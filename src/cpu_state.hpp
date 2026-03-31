#pragma once


struct CpuState {
  bool ime = false;
  bool halt = false;
  bool halt_bug = false;
  bool stop = false;
  bool hard_lock = false;
  bool double_speed = false;

  inline void Reset() {
    ime = false;
    halt = false;
    halt_bug = false;
    stop = false;
    hard_lock = false;
    double_speed = false;
  }
};
