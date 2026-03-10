#pragma once
#include <Arduino.h>

// ============================================================
//  VideoPlayer — MJPEG/AVI decoder pour T-Embed CC1101
//  Décode frames MJPEG → RGB565 → TFT via TJpgDec + TFT_eSPI
//  Lib requise : Bodmer/TJpg_Decoder
// ============================================================

namespace VideoPlayer {

bool open(const String& path);
void play();
void pause();
void resume();
void stop();
void tick();          // Appeler à chaque loop() pendant la lecture

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
