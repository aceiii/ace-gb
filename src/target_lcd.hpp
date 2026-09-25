#pragma once

#include <raylib.h>

#include "lcd.hpp"


class TargetLcd : public ILcd {
public:
  ~TargetLcd() = default;

  void DrawPixel(int x, int y, const Colour& colour) override;
  void VSync() override;
  void Reset() override;

  void Init();
  void Cleanup();
  void ClearTargetBuffers();

  const Texture2D& GetTargetLCD() const;
  const RenderTexture2D& GetTargetTiles() const;
  const RenderTexture2D& GetTargetTilemap(u8 id) const;
  const RenderTexture2D& GetTargetSprites() const;
  const RenderTexture2D& GetTargetPalettes() const;

private:
  Texture2D target_lcd_front_ {};
  Image target_lcd_back_ {};
  RenderTexture2D target_tilemap1_ {};
  RenderTexture2D target_tilemap2_ {};
  RenderTexture2D target_sprites_ {};
  RenderTexture2D target_tiles_ {};
  RenderTexture2D target_palettes_ {};
};
