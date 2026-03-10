#pragma once

#include <memory>
#include <valarray>
#include <vector>

#include "types.hpp"
#include "decoder.hpp"
#include "registers.hpp"
#include "mmu.hpp"
#include "io.hpp"
#include "interrupt.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"
#include "cpu_state.hpp"

struct CpuConfig {
  Mmu* mmu;
  InterruptDevice* interrupts;
};

class Cpu {
public:
  void Init(CpuConfig cfg);

  void Reset();
  u8 Execute();
  u8 ReadNext8();
  u16 ReadNext16();

  u8 Read8(u16 addr);
  void Write8(u16 addr, u8 val);
  u16 Read16(u16 addr);
  void Write16(u16 addr, u16 val);
  void Push16(u16 val);
  u16 Pop16();

  void AddSyncedDevice(SyncedDevice* device);
  void Tick();
  uint64_t Ticks() const;

  Registers& GetRegisters();
  const Registers& GetRegisters() const;
  CpuState& GetState();
  const CpuState& GetState() const;

private:
  Mmu* mmu_ = nullptr;
  InterruptDevice* interrupts_ = nullptr;
  Registers regs_ {};
  CpuState state_ {};

  std::vector<SyncedDevice*> synced_devices {};
  uint64_t tick_counter = 0;

private:
  u8 ExecuteInterrupts();
};
