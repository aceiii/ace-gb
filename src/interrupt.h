#pragma once

enum class Interrupt {
  VBlank = 0,
  LCD,
  Timer,
  Serial,
  Joypad,
  Count,
};
