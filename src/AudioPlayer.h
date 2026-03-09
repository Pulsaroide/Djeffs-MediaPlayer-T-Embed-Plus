#pragma once
#include <Arduino.h>

// ============================================================
//  AudioPlayer — I2S audio from AVI audio stream
//  Uses ESP32-S3 I2S peripheral to drive onboard speaker
// ============================================================

namespace AudioPlayer {

bool init();
bool open(const String& aviPath);   // opens AVI, extracts audio stream
void play();
void pause();
void resume();
void stop();
void tick();                         // fill I2S DMA buffer

void setVolume(int vol);             // 0-100
int  getVolume();

bool isReady();

} // namespace AudioPlayer
