#pragma once

#include <array>

#include "types.hpp"


struct CartHeader {
  union {
    struct {
      std::array<u8, 4> entry_point;
      std::array<u8, 48> logo;
      union {
        struct {
          std::array<u8, 16> title;
        };
        struct {
          std::array<u8, 11> title_short;
          std::array<u8, 4> manufacturer_code;
          u8 cgb_flag;
        };
      };
      std::array<u8, 2> new_licensee_code;
      u8 sgb_flag;
      u8 cartridge_type;
      u8 rom_size;
      u8 ram_size;
      u8 destination_code;
      u8 licensee_code;
      u8 mask_rom_version_number;
      u8 header_checksum;
      std::array<u8, 2> global_checksum;
    };
    std::array<u8, 80> bytes;
  };
};
