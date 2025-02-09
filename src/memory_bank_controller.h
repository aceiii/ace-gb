#pragma once

constexpr size_t kRomBank00Start = 0x0000;
constexpr size_t kRomBank00End = 0x3FFF;
constexpr size_t kRomBank01Start = 0x4000;
constexpr size_t kRomBank01End = 0x7FFF;
constexpr size_t kExtRamStart = 0xA000;
constexpr size_t kExtRamEnd = 0xBFFF;

class MemoryBankController {
public:
  virtual ~MemoryBankController() = default;

  [[nodiscard]] virtual uint8_t read_rom0(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint8_t read_rom1(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint8_t read_ram(uint16_t addr) const = 0;

  virtual void write_reg(uint16_t addr, uint8_t byte) = 0;
  virtual void write_ram(uint16_t addr, uint8_t byte) = 0;
};
