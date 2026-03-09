#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// ============================================================
//  Display — wrapper around TFT_eSPI
// ============================================================

extern TFT_eSPI tft;

namespace Display {

void init();
void showSplash();
void showError(const String& msg);
void setBrightness(uint8_t level);   // 0-255
void clear(uint16_t color = TFT_BLACK);

// Direct framebuffer access for MJPEG decoder
uint16_t* getFrameBuffer();
void pushFrameBuffer(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* buf);

} // namespace Display
