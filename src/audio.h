#pragma once

#include <array>
#include <utility>

#include "io.h"
#include "mmu_device.h"

constexpr int kAudioStart = std::to_underlying(IO::NR10);
constexpr int kAudioEnd = std::to_underlying(IO::LCDC) - 1;
constexpr int kAudioSize = kAudioEnd - kAudioStart + 1;

class Audio : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

private:
  std::array<uint8_t, kAudioSize> ram;
};
