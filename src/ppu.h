#pragma once

#include <raylib.h>

class PPU {
public:
  void init();
  void cleanup();
  void update();
  void render(int x, int y, Color tint);

  RenderTexture2D target;
};
