#include "square_channel.h"

SquareChannel::SquareChannel() {
}

void SquareChannel::reset() {
  nrx0.val = 0;
  nrx1.val = 0;
  nrx2.val = 0;
  nrx3 = 0;
  nrx4.val = 0;
}

void SquareChannel::write(AudioRegister reg, uint8_t value) {
  switch (reg) {
    case AudioRegister::NRx0:
      nrx0.val = value;
      break;
    case AudioRegister::NRx1:
      nrx1.val = value;
      break;
    case AudioRegister::NRx2:
      nrx2.val = value;
      break;
    case AudioRegister::NRx3:
      nrx3 = value;
      break;
    case AudioRegister::NRx4:
      nrx4.val = value;
      break;
  }
}

uint8_t SquareChannel::read(AudioRegister reg) const {
  switch (reg) {
    case AudioRegister::NRx0: return nrx0.val;
    case AudioRegister::NRx1: return nrx1.val;
    case AudioRegister::NRx2: return nrx2.val;
    case AudioRegister::NRx3: return nrx3;
    case AudioRegister::NRx4: return nrx4.val;
  }
}

uint8_t SquareChannel::sample() const {
  return 0;
}

void SquareChannel::tick(uint8_t cycles) {
}
