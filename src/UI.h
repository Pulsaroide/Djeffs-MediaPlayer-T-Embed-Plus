#pragma once
#include <Arduino.h>
#include <vector>
#include "SDManager.h"

// ============================================================
//  UI — File browser + player controls overlay
// ============================================================

namespace UI {

void init();

// ── File Browser ─────────────────────────────────────────────
void showBrowser(const std::vector<VideoFile>& files);
void scrollBrowser(int delta);
String getSelectedFile();   // returns full path of highlighted item

// ── Player view ──────────────────────────────────────────────
void showPlayer(const String& filename);
void updateProgressBar(uint32_t posMs, uint32_t durMs);
void setPlayIcon(bool playing);
void showVolumeOverlay(int vol);  // fades after 1.5s

// ── Helpers ──────────────────────────────────────────────────
bool isLoopEnabled();
void toggleLoop();

} // namespace UI
