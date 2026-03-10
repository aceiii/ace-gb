#pragma once


struct CpuState {
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
