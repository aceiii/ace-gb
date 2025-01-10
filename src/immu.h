#pragma once

#include <cstdint>
#include <functional>

enum class IO {
  P1 = 0xff00,
  SB = 0xff01,
  SC = 0xff02,
  DIV = 0xff04,
  TIMA = 0xff05,
  TMA = 0xff06,
  TAC = 0xff07,
  IF = 0xff0f,
  NR10 = 0xff10,
  NR11 = 0xff11,
  NR12 = 0xff12,
  NR13 = 0xff13,
  NR14 = 0xff14,
  NR21 = 0xff16,
  NR22 = 0xff17,
  NR23 = 0xff18,
  NR24 = 0xff19,
  NR30 = 0xff1a,
  NR31 = 0xff1b,
  NR32 = 0xff1c,
  NR33 = 0xff1d,
  NR34 = 0xff1e,
  NR41 = 0xff20,
  NR42 = 0xff21,
  NR43 = 0xff22,
  NR44 = 0xff23,
  NR50 = 0xff24,
  NR51 = 0xff25,
  NR52 = 0xff26,
  WAVE = 0xff30, // to 0xff3f
  LCDC = 0xff40,
  STAT = 0xff41,
  SCY = 0xff42,
  SCX = 0xff43,
  LY = 0xff44,
  LYC = 0xff45,
  DMA = 0xff46,
  BGP = 0xff47,
  OBP0 = 0xff48,
  OBP1 = 0xff49,
  WY = 0xff4a,
  WX = 0xff4b,
  KEY1 = 0xff4d,
  VBK = 0xff4f,
  BOOT = 0xff50,
  HDMA1 = 0xff51,
  HDMA2 = 0xff52,
  HDMA3 = 0xff53,
  HDMA4 = 0xff54,
  HDMA5 = 0xff55,
  RP = 0xff56,
  BCPS = 0xff68, BGPI = 0xff68,
  BCPD = 0xff69, BGPD = 0xff69,
  OCPS = 0xff6a, OBPI = 0xff6a,
  OCPD = 0xff6b, OBPD = 0xff6b,
  OPRI = 0xff6c,
  SVBK = 0xff70,
  PCM12 = 0xff76,
  PCM34 = 0xff77,
  IE = 0xfff,
};

typedef std::function<uint8_t(uint16_t, uint8_t)> mmu_callback;

class IMMU {
public:
  virtual ~IMMU() = 0;

  virtual void load_boot_rom(const uint8_t *rom) = 0;
  virtual void load_cartridge(const std::vector<uint8_t> &cart) = 0;

  virtual void write(uint16_t addr, uint8_t byte) = 0;
  virtual void write(uint16_t addr, uint16_t word) = 0;

  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint16_t read16(uint16_t addr) const = 0;

  virtual void inc(uint16_t addr) = 0;

  virtual void on_write8(uint16_t addr, mmu_callback callback) = 0;

  virtual void reset() = 0;
};
