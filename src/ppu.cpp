#include "ppu.h"

const int kLCDWidth = 160;
const int kLCDHeight = 144;

void PPU::init() {
  target = LoadRenderTexture(kLCDWidth, kLCDHeight);

  BeginTextureMode(target);
  ClearBackground(Color(75, 0, 1, 255));
  EndTextureMode();
}

void PPU::cleanup() {
  UnloadRenderTexture(target);
}

void PPU::update() {
}

void PPU::render(int x, int y, Color tint) {
  DrawTexture(target.texture, x, y, tint);
}
