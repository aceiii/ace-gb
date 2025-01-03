#include <fstream>
#include <memory>
#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

#include "cpu.h"
#include "registers.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr size_t kTestMemSize = 65536;

struct TestConfig {
  fs::path path;
};

template <typename T>
struct TestResult {
  size_t total = 0;
  std::vector<T> succeeded {};
  std::vector<T> failed {};
};

void load_registers(json &data, Registers &regs) {
  regs.set(Reg8::A, data.at("a").get<uint8_t>());
  regs.set(Reg8::B, data.at("b").get<uint8_t>());
  regs.set(Reg8::C, data.at("c").get<uint8_t>());
  regs.set(Reg8::D, data.at("d").get<uint8_t>());
  regs.set(Reg8::E, data.at("e").get<uint8_t>());
  regs.set(Reg8::F, data.at("f").get<uint8_t>());
  regs.set(Reg8::H, data.at("h").get<uint8_t>());
  regs.set(Reg8::L, data.at("l").get<uint8_t>());
  regs.pc = data.at("pc").get<uint16_t>();
  regs.sp = data.at("sp").get<uint16_t>();
}

std::unique_ptr<uint8_t[]> load_memory(json &data) {
  auto memory = Mem::create_memory(kTestMemSize);
  for (const auto &ram : data.items()) {
    auto addr = ram.value().at(0).get<uint16_t>();
    auto val = ram.value().at(1).get<uint16_t>();
    memory[addr] = val;
  }
  return memory;
}

bool check_memory(json &data, const uint8_t *mem) {
  for (const auto &ram : data.items()) {
    auto addr = ram.value().at(0).get<uint16_t>();
    auto val = ram.value().at(1).get<uint8_t>();

    if (mem[addr] != val) {
      return false;
    }
  }
  return true;
}

tl::expected<TestResult<int>, std::string> run_test(const TestConfig &config) {
  std::ifstream input(config.path);
  if (input.fail()) {
    return tl::unexpected { fmt::format("Failed to open '{}': {}", config.path.string(), strerror(errno)) };
  }

  json data;
  try {
    data = json::parse(input);
  } catch (const json::parse_error &e) {
    return tl::unexpected { e.what() };
  }

  spdlog::info("# of test cases: {}", data.size());

  TestResult<int> result {};
  result.total = data.size();

  CPU cpu(kTestMemSize);

  for (int i = 0; i < data.size(); i++) {
    auto test = data.at(i);
    auto initial = test.at("initial");
    auto final = test.at("final");
    auto cycles = test.at("cycles");
    auto name = test.at("name").get<std::string>();

    spdlog::debug("Running case #{} of {}: {}", i, data.size(), name);

    Registers &regs = cpu.regs;
    load_registers(initial, regs);

    Registers final_regs;
    load_registers(final, final_regs);

    cpu.memory = load_memory(initial.at("ram"));
    cpu.execute();

    auto is_success = regs == final_regs && check_memory(final.at("ram"), cpu.memory.get());
    if (is_success) {
      result.succeeded.push_back(i);
      spdlog::debug("Test #{} succeeded.", i);
    } else {
      result.failed.push_back(i);
      spdlog::debug("Test #{} failed.", i);
    }
  }

  spdlog::debug("Done. {} out of {} succeeded.", result.succeeded.size(), result.total);

  return result;
}

int run_all_tests(const TestConfig &config) {
  std::vector<fs::path> test_files;
  test_files.reserve(128);

  for (const auto &entry : fs::directory_iterator(config.path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      test_files.push_back(entry.path());
    }
  }

  spdlog::info("Found {} test files.", test_files.size());

  TestResult<fs::path> total_results {};
  total_results.total = test_files.size();

  for (const auto &path : test_files) {
    TestConfig test_config = config;
    test_config.path = path;
    auto res = run_test(test_config);
    if (!res.has_value()) {
      total_results.failed.emplace_back(path);
      continue;
    }

    auto result = res.value();
    if (!result.failed.empty()) {
      total_results.failed.emplace_back(path);
    } else {
      total_results.succeeded.emplace_back(path);
    }
  }

  spdlog::info("Test Summary:");
  spdlog::info("{} / {} tests were successful.", total_results.succeeded.size(), total_results.total);
  return total_results.failed.empty() ? 0 : 1;
}

int run_single_test(const TestConfig &config) {
  if (auto res = run_test(config); res.has_value()) {
    return 0;
  } else {
    spdlog::error("Failed: {}", res.error());
    return 1;
  }
}

int run_cpu_tests(const TestConfig &config) {
  if (fs::is_directory(config.path)) {
    spdlog::info("Running all .json test files at {}.", config.path.string());
    return run_all_tests(config);
  }

  spdlog::info("Running single test at {}.", config.path.string());
  return run_single_test(config);
}

static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

auto main(int argc, char *argv[]) -> int {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program("ace-gb", "0.0.1");

  program.add_argument("--log-level")
    .help("Set the verbosity for logging")
    .default_value(std::string("info"))
    .nargs(1);

  program.add_argument("path")
    .help("Path to json test file or directory containing json test files.");

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  const std::string level = program.get("--log-level");
  if (!set_logging_level(level)) {
    std::cerr << fmt::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}",
                             level)
              << std::endl;
    std::cerr << program;
    return 1;
  }

  TestConfig config;
  config.path = program.get("path");

  return run_cpu_tests(config);
}
