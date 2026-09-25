#include <utility>
#include <spdlog/spdlog.h>

#include "target_lcd.hpp"
#include "ppu.hpp"


void TargetLcd::DrawPixel(int x, int y, const Colour &colour) {
  ImageDrawPixel(&target_lcd_back_, x, y, Color{ .r = colour.red, .g = colour.green, .b = colour.blue, .a = colour.alpha });
}

void TargetLcd::VSync() {
  UpdateTexture(target_lcd_front_, target_lcd_back_.data);
}

void TargetLcd::Reset() {
  BeginTextureMode(target_tiles_);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_tilemap1_);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_tilemap2_);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_sprites_);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_palettes_);
  ClearBackground(BLANK);
  EndTextureMode();

  ClearTargetBuffers();
}

void TargetLcd::Init() {
  spdlog::info("Initializing TargetLcd");

  target_lcd_back_ = GenImageColor(Ppu::kLCDWidth, Ppu::kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  target_lcd_front_ = LoadTextureFromImage(target_lcd_back_);

  constexpr int tiles_width = 16 * 8;
  constexpr int tiles_height = 48 * 8;
  target_tiles_ = LoadRenderTexture(tiles_width, tiles_height);

  constexpr int palettes_width = 136;
  constexpr int palettes_height = 128;
  target_palettes_ = LoadRenderTexture(palettes_width, palettes_height);

  constexpr int tilemap_width = 256;
  constexpr int tilemap_height = 256;
  target_tilemap1_ = LoadRenderTexture(tilemap_width, tilemap_height);
  target_tilemap2_ = LoadRenderTexture(tilemap_width, tilemap_height);

  constexpr int sprites_width = 8 * 9;
  constexpr int sprites_height = 5 * 16;
  target_sprites_ = LoadRenderTexture(sprites_width, sprites_height);

  target_lcd_back_ = GenImageColor(Ppu::kLCDWidth, Ppu::kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

void TargetLcd::Cleanup() {
  UnloadRenderTexture(target_tiles_);
  UnloadRenderTexture(target_tilemap1_);
  UnloadRenderTexture(target_tilemap2_);
  UnloadRenderTexture(target_sprites_);
  UnloadRenderTexture(target_palettes_);

  UnloadImage(target_lcd_back_);
  UnloadTexture(target_lcd_front_);
}

void TargetLcd::ClearTargetBuffers() {
  ImageClearBackground(&target_lcd_back_, BLANK);
  UpdateTexture(target_lcd_front_, target_lcd_back_.data);
}

const Texture2D& TargetLcd::GetTargetLCD() const {
  return target_lcd_front_;
}

const RenderTexture2D& TargetLcd::GetTargetTiles() const {
  return target_tiles_;
}

const RenderTexture2D& TargetLcd::GetTargetTilemap(u8 idx) const {
  if (idx == 0) {
    return target_tilemap1_;
  } else if (idx == 1) {
    return target_tilemap2_;
  }
  std::unreachable();
}

const RenderTexture2D& TargetLcd::GetTargetSprites() const {
  return target_sprites_;
}

const RenderTexture2D& TargetLcd::GetTargetPalettes() const {
  return target_palettes_;
}
