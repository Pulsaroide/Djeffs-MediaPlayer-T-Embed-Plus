#pragma once
#include <Arduino.h>

// ============================================================
//  EncoderInput — Rotary encoder + button handler
//  Lib : mathertel/RotaryEncoder@^1.5.3  (officielle LilyGO)
//  Encoder rotation → getDelta()
//  Short press      → wasPressed()
//  Long press       → wasLongPressed()
// ============================================================

namespace EncoderInput {

void init();
void update();         // call every loop()

int  getDelta();       // +N ou -N steps depuis dernier appel
bool wasPressed();     // true une fois par appui court
bool wasLongPressed(); // true une fois par appui long

} // namespace EncoderInput
