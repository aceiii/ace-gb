#include <fstream>
#include <memory>
#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

#include "cpu.h"
#include "io.h"
#include "mmu.h"
#include "registers.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr size_t kTestMemSize = 65536;

using TestMemory = std::array<uint8_t, kTestMemSize>;

class TestMemoryDevice : public IMMUDevice {
public:
  explicit TestMemoryDevice(TestMemory &mem_):mem{mem_} {}

  bool valid_for(uint16_t addr) const override {
    return true;
  }

  void write8(uint16_t addr, uint8_t byte) override {
    mem[addr] = byte;
  }

  [[nodiscard]] uint8_t read8(uint16_t addr) const override {
    return mem[addr];
  }

  void reset() override {
    mem.fill(0);
  }

private:
  TestMemory& mem;
};

struct TestConfig {
  fs::path path {};
  std::vector<size_t> only_cases {};
  bool list_fails = false;
  bool fail_details = false;
};

template <typename TSuccess, typename TFailed>
struct TestResult {
  size_t total = 0;
  std::vector<TSuccess> succeeded {};
  std::vector<TFailed> failed {};
};

void load_registers(const json &data, Registers &regs) {
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

void load_memory(const json& data, TestMemory& mem) {
  for (const auto &ram : data.items()) {
    auto addr = ram.value().at(0).get<uint16_t>();
    auto val = ram.value().at(1).get<uint16_t>();
    spdlog::debug("load_memory: {:02x} => {:02x}", addr, val);
    mem[addr] = val;
  }
}

bool check_memory(const json &data, const TestMemory& mem) {
  for (const auto &ram : data.items()) {
    auto addr = ram.value().at(0).get<uint16_t>();
    auto val = ram.value().at(1).get<uint8_t>();
    if (mem[addr] != val) {
      return false;
    }
  }
  return true;
}

std::vector<std::tuple<std::string, uint8_t, uint8_t>> mismatched_registers(const Registers &regs, const Registers &target) {
  std::vector<std::tuple<std::string, uint8_t, uint8_t>> failed;
  for (int i = 0; i < std::to_underlying(Reg8::Count); i += 1) {
    Reg8 r {i};
    uint8_t a = regs.get(r);
    uint8_t b = target.get(r);
    if (a != b) {
      failed.emplace_back(magic_enum::enum_name(r), a, b);
    }
  }

  if (regs.sp != target.sp) {
    failed.emplace_back("SP", regs.sp, target.sp);
  }

  if (regs.pc != target.pc) {
    failed.emplace_back("PC", regs.pc, target.pc);
  }

  return failed;
}

std::vector<std::tuple<uint16_t, uint8_t, uint8_t>> mismatched_memory(const json& data, const TestMemory& mem) {
  std::vector<std::tuple<uint16_t, uint8_t, uint8_t>> failed;
  for (const auto &ram : data.items()) {
    auto addr = ram.value().at(0).get<uint16_t>();
    auto val = ram.value().at(1).get<uint8_t>();
    auto mem_val = mem[addr];
    if (mem_val != val) {
      failed.emplace_back(addr, mem_val, val);
    }
  }
  return failed;
}

tl::expected<TestResult<int, int>, std::string> run_test(const TestConfig &config) {
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

  spdlog::debug("Running test '{}', {} cases.", config.path.string(), data.size());

  TestResult<int, int> result {};

  std::vector<size_t> tests_to_run;
  if (!config.only_cases.empty()) {
    tests_to_run = config.only_cases;
  } else {
    tests_to_run.resize(data.size());
    std::iota(begin(tests_to_run), end(tests_to_run), 0);
  }

  result.total = tests_to_run.size();

  CPU cpu;
  MMU mmu;

  TestMemory mem;
  mmu.add_device(std::make_shared<TestMemoryDevice>(mem));

  for (const auto i : tests_to_run) {
    auto test = data.at(i);
    auto initial = test.at("initial");
    auto final = test.at("final");
    auto cycles = test.at("cycles");
    auto name = test.at("name").get<std::string>();

    spdlog::debug("Running case #{} of {}: {}", i, result.total, name);

    cpu.reset();
    mmu.reset_devices();

    Registers &regs = cpu.regs;
    load_registers(initial, regs);

    Registers final_regs;
    load_registers(final, final_regs);
    load_memory(initial.at("ram"), mem);

    cpu.execute(mmu);

    auto reg_match = regs == final_regs;
    auto ram_match = check_memory(final.at("ram"), mem);
    auto is_success = reg_match && ram_match;
    if (is_success) {
      result.succeeded.emplace_back(i);
      spdlog::debug("Test #{} succeeded.", i);
    } else {
      result.failed.emplace_back(i);
      spdlog::error("Test #{} failed.", i);
    }

    if (!is_success && config.fail_details) {
      if (!reg_match) {
        for (const auto &[reg, a, b] : mismatched_registers(regs, final_regs)) {
          spdlog::error("  reg[{}]: {} != {}", reg , a, b);
        }
      }

      if (!ram_match) {
        for (const auto &[addr, a, b] : mismatched_memory(final.at("ram"), mem)) {
          spdlog::error("  mem[{}]: {} != {}", addr , a, b);
        }
      }
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
      test_files.emplace_back(entry.path());
    }
  }

  spdlog::info("Found {} test files.", test_files.size());

  TestResult<fs::path, std::tuple<fs::path, size_t, size_t>> total_results {};
  total_results.total = test_files.size();

  for (const auto &path : test_files) {
    TestConfig test_config = config;
    test_config.path = path;
    auto res = run_test(test_config);
    if (!res.has_value()) {
      total_results.failed.emplace_back(path, 0, 0);
      continue;
    }

    auto result = res.value();
    if (!result.failed.empty()) {
      total_results.failed.emplace_back(path, result.failed.size(), result.total);
    } else {
      total_results.succeeded.emplace_back(path);
    }
  }

  spdlog::info("Test Summary:");
  spdlog::info("{} / {} tests were successful.", total_results.succeeded.size(), total_results.total);

  if (config.list_fails && !total_results.failed.empty()) {
    spdlog::error("Failed:");
    for (const auto& [path, failed, total] : total_results.failed) {
      spdlog::error("  {}: {}/{}", path.string(), total-failed, total);
    }
  }

  return total_results.failed.empty() ? 0 : 1;
}

int run_single_test(const TestConfig &config) {
  if (auto res = run_test(config); res.has_value()) {
    auto result = res.value();
    if (config.list_fails && !result.failed.empty()) {
      spdlog::error("Failed:");
      for (const auto &item : result.failed) {
        spdlog::error("  #{}", item);
      }
    } else if (result.succeeded.size() == result.total) {
      spdlog::info("All Tests successful");
    } else {
      spdlog::error("{} / {} tests succeeded.", result.succeeded.size(), result.total);
    }
    return result.failed.empty() ? 0 : 1;
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

  program.add_argument("--list-fails")
    .help("List failed tests")
    .default_value(false)
    .implicit_value(true);

  program.add_argument("--fail-details")
    .help("Display mismatched registers or ram that caused the test to fail")
    .default_value(false)
    .implicit_value(true);

  program.add_argument("--only-cases")
    .help("Only run these cases matching specified index")
    .scan<'d', size_t>();

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
  config.list_fails = program.get<bool>("--list-fails");
  config.fail_details = program.get<bool>("--fail-details");
  config.only_cases = program.get<std::vector<size_t>>("--only-cases");

  return run_cpu_tests(config);
}
