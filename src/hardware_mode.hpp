#pragma once

#include <utility>

#include "emulation_mode.hpp"


enum class HardwareMode {
  kDmgMode = std::to_underlying(EmulationMode::kDmgMode),
  kCgbMode,
};
