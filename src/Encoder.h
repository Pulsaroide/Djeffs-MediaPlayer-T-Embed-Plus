#pragma once
#include <Arduino.h>

// ============================================================
//  EncoderInput — Rotary encoder + button handler
//  Encoder rotation → getDelta()
//  Short press     → wasPressed()
//  Long press      → wasLongPressed()
// ============================================================

namespace EncoderInput {

void init();
void update();        // call every loop()

int  getDelta();      // +N or -N steps since last call (clears on read)
bool wasPressed();    // true once per short press
bool wasLongPressed();// true once per long press

} // namespace EncoderInput
