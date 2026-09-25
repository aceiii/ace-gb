#pragma once

#include "colour.hpp"


class ILcd {
public:
  virtual ~ILcd() = default;

  virtual void DrawPixel(int x, int y, const Colour& colour) = 0;
  virtual void VSync() = 0;
  virtual void Reset() = 0;

};
