#pragma once

#include "lcd.hpp"


class NullLcd : public ILcd {
public:
  virtual ~NullLcd() = default;

  void DrawPixel(int x, int y, const Colour& colour) override;
  void VSync() override;
  void Reset() override;
};

