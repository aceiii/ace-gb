#pragma once

#include "decoder.hpp"
#include "registers.h"
#include "memory.h"
#include "mmu.h"
#include "io.h"
#include "interrupt.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"

#include <memory>
#include <valarray>
#include <vector>


constexpr size_t kClockSpeed = 4194304;
constexpr float kFrameRate = 59.73;

struct State {
  bool ime = false;
  bool halt = false;
  bool stop = false;
  bool hard_lock = false;

  inline void Reset() {
    ime = false;
    halt = false;
    stop = false;
    hard_lock = false;
  }
};

class Cpu {
public:
  Cpu() = delete;
  explicit Cpu(Mmu &mmu, InterruptDevice &interrupts);

  void Reset();
  uint8_t Execute();
  uint8_t ReadNext8();
  uint16_t ReadNext16();

  uint8_t Read8(uint16_t addr);
  void Write8(uint16_t addr, uint8_t val);
  uint16_t Read16(uint16_t addr);
  void Write16(uint16_t addr, uint16_t val);
  void Push16(uint16_t val);
  uint16_t Pop16();

  void AddSyncedDevice(SyncedDevice *device);
  void Tick();
  uint64_t Ticks() const;

public:
  Mmu &mmu;
  InterruptDevice &interrupts;
  Registers regs;
  State state;

  std::vector<SyncedDevice*> synced_devices;
  uint64_t tick_counter = 0;

private:
  uint8_t ExecuteInterrupts();
};
