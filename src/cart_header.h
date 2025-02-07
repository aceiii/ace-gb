#pragma once

#include <array>

struct cart_header {
  std::array<uint8_t, 4> entry_point;
  std::array<uint8_t, 48> logo;
  std::array<uint8_t, 16> title;
  std::array<uint8_t, 4> manufacturer_code;
  uint8_t cgb_flag;
  std::array<uint8_t, 2> new_licensee_code;
  uint8_t sgb_flag;
  uint8_t cartridge_type;
  uint8_t rom_size;
  uint8_t ram_size;
  uint8_t destination_code;
  uint8_t licensee_code;
  uint8_t mask_rom_version_number;
  uint8_t header_checksum;
  std::array<uint8_t, 2> global_checksum;
};
