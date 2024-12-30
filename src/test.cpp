#include <fstream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "cpu.h"

using json = nlohmann::json;

constexpr size_t kTestMemSize = 65536;

int run_cpu_tests() {
  CPU cpu;
  cpu.memory = Mem::create_memory(kTestMemSize);

  spdlog::info("Running CPU tests");

  std::ifstream input("../external/sm83/v1/00.json");
  if (input.fail()) {
    spdlog::error("Failed to open test file: {}", strerror(errno));
    return 1;
  }

  json data = json::parse(input);

  spdlog::info("# of tests: {}", data.size());

  for (int i = 0; i < data.size(); i++) {
    auto test = data.at(i);
    auto initial = test.at("initial");
    auto final = test.at("final");
    auto cycles = test.at("cycles");

    spdlog::info("Running test ({} of {}): {}", i, data.size(),  test.at("name").get<std::string>());
    spdlog::info(" > Initial:");
    for (const auto &reg : initial.items()) {
      if (reg.key() == "ram") {
        spdlog::info("    ram: {}", reg.value().dump());
      } else {
        spdlog::info("    {}: {}", reg.key(), reg.value().get<uint16_t>());
      }
    }

    spdlog::info(" > Final:");
    for (const auto &reg : final.items()) {
      if (reg.key() == "ram") {
        spdlog::info("    ram: {}", reg.value().dump());
      } else {
        spdlog::info("    {}: {}", reg.key(), reg.value().get<uint16_t>());
      }
    }
  }

  spdlog::info("Done.");
  return 0;
}
