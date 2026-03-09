#pragma once
#include <Arduino.h>

// ============================================================
//  VideoPlayer — MJPEG/AVI decoder for T-Embed
//  Reads MJPEG frames from AVI container on SD card
//  Decodes JPEG → RGB565 → pushes to TFT via DMA
// ============================================================

namespace VideoPlayer {

bool open(const String& path);
void play();
void pause();
void resume();
void stop();
void tick();         // Call every loop() iteration while playing

void seekToStart();

bool isPlaying();
bool isPaused();
bool isFinished();

uint32_t getPositionMs();
uint32_t getDurationMs();
uint16_t getFPS();
uint16_t getWidth();
uint16_t getHeight();
uint32_t getFrameCount();
uint32_t getCurrentFrame();

} // namespace VideoPlayer
