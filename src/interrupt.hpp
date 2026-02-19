#pragma once

enum class Interrupt {
  VBlank = 0,
  Stat,
  Timer,
  Serial,
  Joypad,
  Count,
};
