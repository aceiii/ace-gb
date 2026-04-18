#pragma once


struct CpuState {
  u8 ime_counter = 0;

  bool ime = false;
  bool halt = false;
  bool stop = false;
  bool hard_lock = false;
  bool double_speed = false;

  inline void Reset() {
    ime = false;
    halt = false;
    stop = false;
    hard_lock = false;
    double_speed = true;
  }
};
