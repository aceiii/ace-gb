#include <fstream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "cpu.h"

using json = nlohmann::json;

constexpr size_t kTestMemSize = 65536;

int run_cpu_tests() {
  spdlog::info("Running CPU tests");

  std::ifstream input("../external/sm83/v1/5b.json");
  if (input.fail()) {
    spdlog::error("Failed to open test file: {}", strerror(errno));
    return 1;
  }

  json data = json::parse(input);

  spdlog::info("# of tests: {}", data.size());

  CPU cpu(kTestMemSize);
  int success = 0;
  int fail = 0;

  for (int i = 0; i < data.size(); i++) {
    auto test = data.at(i);
    auto initial = test.at("initial");
    auto final = test.at("final");
    auto cycles = test.at("cycles");

    spdlog::info("Running test #{} of {}: {}", i, data.size(),  test.at("name").get<std::string>());

    Registers &regs = cpu.regs;
    regs.reset();
    regs.pc = initial.at("pc").get<uint16_t>();
    regs.sp = initial.at("sp").get<uint16_t>();
    regs.set(Reg8::A, initial.at("a").get<uint8_t>());
    regs.set(Reg8::B, initial.at("b").get<uint8_t>());
    regs.set(Reg8::C, initial.at("c").get<uint8_t>());
    regs.set(Reg8::D, initial.at("d").get<uint8_t>());
    regs.set(Reg8::E, initial.at("e").get<uint8_t>());
    regs.set(Reg8::F, initial.at("f").get<uint8_t>());
    regs.set(Reg8::H, initial.at("h").get<uint8_t>());
    regs.set(Reg8::L, initial.at("l").get<uint8_t>());

    auto memory = Mem::create_memory(65536);
    for (const auto &ram : initial.at("ram").items()) {
      auto addr = ram.value().at(0).get<uint16_t>();
      auto val = ram.value().at(1).get<uint16_t>();
      memory[addr] = val;
    }
    cpu.memory = std::move(memory);
    cpu.execute();

    auto a_match = final.at("a").get<uint8_t>() == cpu.regs.get(Reg8::A);
    auto b_match = final.at("b").get<uint8_t>() == cpu.regs.get(Reg8::B);
    auto c_match = final.at("c").get<uint8_t>() == cpu.regs.get(Reg8::C);
    auto d_match = final.at("d").get<uint8_t>() == cpu.regs.get(Reg8::D);
    auto e_match = final.at("e").get<uint8_t>() == cpu.regs.get(Reg8::E);
    auto f_match = final.at("f").get<uint8_t>() == cpu.regs.get(Reg8::F);
    auto h_match = final.at("h").get<uint8_t>() == cpu.regs.get(Reg8::H);
    auto l_match = final.at("l").get<uint8_t>() == cpu.regs.get(Reg8::L);
    auto sp_match = final.at("sp").get<uint16_t>() == cpu.regs.sp;
    auto pc_match = final.at("pc").get<uint16_t>() == cpu.regs.pc;

    bool ram_match = true;
    for (const auto &ram : final.at("ram").items()) {
      auto addr = ram.value().at(0).get<uint16_t>();
      auto val = ram.value().at(1).get<uint8_t>();

      if (cpu.memory[addr] != val) {
        ram_match = false;
        break;
      }
    }

    auto is_success = a_match && b_match && c_match && d_match && e_match && f_match && h_match && l_match && sp_match && pc_match && ram_match;
    if (is_success) {
      success += 1;
      spdlog::info("Test #{} succeeded.", i);
    } else {
      fail += 1;
      spdlog::error("Test #{} failed.", i);
    }
  }

  spdlog::info("Done.");
  spdlog::info("{} / {} Tests were successful.", success, data.size());

  return success == data.size() ? 0 : 1;
}
