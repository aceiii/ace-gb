#pragma once

#include <string>

enum class CartType : uint8_t {
  ROM_ONLY = 0x00,
  MBC1 = 0x01,
  MBC1_RAM = 0x02,
  MBC1_RAM_BATTERY = 0x03,
  MBC2 = 0x05,
  MBC2_BATTERY = 0x06,
  ROM_RAM = 0x08,
  ROM_RAM_BATTERY = 0x09,
  MMM01 = 0x0B,
  MMM01_RAM = 0x0C,
  MMM01_RAM_BATTERY = 0x0D,
  MBC3_TIMER_BATTERY = 0X0F,
  MBC3_TIMER_RAM_BATTERY = 0x10,
  MBC3 = 0x11,
  MBC3_RAM = 0x12,
  MBC3_RAM_BATTERY = 0x13,
  MBC5 = 0x19,
  MBC5_RAM = 0X1A,
  MBC5_RAM_BATTERY = 0X1B,
  MBC5_RUMBLE = 0x1C,
  MBC5_RUMBLE_RAM = 0x1D,
  MBC5_RUMBLE_RAM_BATTERY = 0x1E,
  MBC6 = 0x20,
  MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
  POCKET_CAMERA = 0XFC,
  BANDAI_TAMA5 = 0XFD,
  HUC3 = 0xFE,
  HUC1_RAM_BATTERY = 0xFF,
};

enum class RamType : uint8_t {
  NO_RAM = 0x00,
  UNUSED = 0x01,
  BANKS_1 = 0X02,
  BANKS_4 = 0X03,
  BANKS_16 = 0X04,
  BANKS_8 = 0X05,
};

struct cart_info {
  std::string title;
  CartType type;
  size_t rom_size_bytes;
  size_t rom_num_banks;
  RamType ram_type;

  inline void reset() {
    title = "";
    type = CartType::ROM_ONLY;
    rom_size_bytes = 0;
    rom_num_banks = 0;
    ram_type = RamType::NO_RAM;
  }
};
