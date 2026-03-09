#include "UI.h"
#include "Display.h"
#include "Config.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// ── Colours (RGB565) ─────────────────────────────────────────
#define COL_BG          0x0841   // very dark blue-grey
#define COL_ITEM        0x10A2   // dark item row
#define COL_SELECTED    0x0473   // accent blue
#define COL_TEXT        TFT_WHITE
#define COL_SUBTEXT     0x8410   // grey
#define COL_ACCENT      0x07FF   // cyan
#define COL_BAR_BG      0x2945
#define COL_BAR_FG      0x07FF
#define COL_VOL_BG      0x2008
#define COL_VOL_FG      0x07E0   // green

// ── Layout constants ─────────────────────────────────────────
#define ITEM_H          36
#define ITEM_MARGIN     4
#define THUMB_W         48
#define THUMB_H         30
#define HEADER_H        28
#define BROWSER_Y       HEADER_H
#define VISIBLE_ITEMS   ((DISPLAY_HEIGHT - HEADER_H) / ITEM_H)

// ── Controls bar (bottom of screen in player mode) ───────────
#define CTRL_H          UI_CONTROLS_HEIGHT
#define CTRL_Y          (DISPLAY_HEIGHT - CTRL_H)
#define PROG_BAR_Y      (CTRL_Y + 6)
#define PROG_BAR_H      6
#define PROG_BAR_X      8
#define PROG_BAR_W      (DISPLAY_WIDTH - 16)

// ── State ─────────────────────────────────────────────────────
static std::vector<VideoFile> fileList;
static int selectedIdx = 0;
static int scrollOffset = 0;
static bool loopEnabled = false;
static bool isPlaying_  = false;

static uint32_t volOverlayTime = 0;
static int      lastVol        = -1;
static int      lastProg       = -1;

// ── Helpers ───────────────────────────────────────────────────
static String formatDuration(uint32_t ms) {
    uint32_t s = ms / 1000;
    char buf[12];
    snprintf(buf, sizeof(buf), "%u:%02u", s / 60, s % 60);
    return String(buf);
}

static void drawBrowserHeader() {
    tft.fillRect(0, 0, DISPLAY_WIDTH, HEADER_H, COL_SELECTED);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.drawString("  Media Player", 0, HEADER_H / 2);
    // Loop icon
    tft.setTextDatum(MR_DATUM);
    tft.drawString(loopEnabled ? "[L]" : "   ", DISPLAY_WIDTH - 2, HEADER_H / 2);
}

static void drawItem(int idx, bool selected) {
    if (idx < 0 || idx >= (int)fileList.size()) return;
    const VideoFile& vf = fileList[idx];

    int visIdx = idx - scrollOffset;
    int y = BROWSER_Y + visIdx * ITEM_H;

    // Background
    uint16_t bgCol = selected ? COL_SELECTED : (idx % 2 == 0 ? COL_ITEM : COL_BG);
    tft.fillRect(0, y, DISPLAY_WIDTH, ITEM_H, bgCol);

    // Thumbnail placeholder or icon
    uint16_t iconCol = vf.hasThumb ? 0x07E0 : 0x4208;
    tft.fillRect(ITEM_MARGIN, y + (ITEM_H - THUMB_H) / 2,
                 THUMB_W, THUMB_H, iconCol);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString(vf.hasThumb ? "IMG" : "AVI",
                   ITEM_MARGIN + THUMB_W / 2,
                   y + ITEM_H / 2);

    // Filename
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_WHITE);
    String name = vf.name;
    if (name.length() > 16) name = name.substring(0, 15) + "~";
    tft.drawString(name, ITEM_MARGIN + THUMB_W + 6, y + 10);

    // Size
    tft.setTextColor(COL_SUBTEXT);
    char sizeBuf[12];
    if (vf.sizeKB > 1024)
        snprintf(sizeBuf, sizeof(sizeBuf), "%u MB", vf.sizeKB / 1024);
    else
        snprintf(sizeBuf, sizeof(sizeBuf), "%u KB", vf.sizeKB);
    tft.drawString(String(sizeBuf), ITEM_MARGIN + THUMB_W + 6, y + 22);
}

// ── Public API ────────────────────────────────────────────────

namespace UI {

void init() {
    tft.setTextFont(1);
}

void showBrowser(const std::vector<VideoFile>& files) {
    fileList     = files;
    selectedIdx  = 0;
    scrollOffset = 0;
    lastProg     = -1;

    tft.fillScreen(COL_BG);
    drawBrowserHeader();

    if (files.empty()) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COL_SUBTEXT);
        tft.drawString("No .avi files found", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("Put videos in /videos/", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 10);
        return;
    }

    int shown = min((int)files.size(), VISIBLE_ITEMS);
    for (int i = 0; i < shown; i++) {
        drawItem(i, i == selectedIdx);
    }
}

void scrollBrowser(int delta) {
    if (fileList.empty()) return;
    int prev = selectedIdx;
    selectedIdx = constrain(selectedIdx + delta, 0, (int)fileList.size() - 1);

    // Auto-scroll
    if (selectedIdx < scrollOffset) scrollOffset = selectedIdx;
    if (selectedIdx >= scrollOffset + VISIBLE_ITEMS)
        scrollOffset = selectedIdx - VISIBLE_ITEMS + 1;

    if (prev != selectedIdx) {
        // Redraw only changed items
        tft.fillRect(0, BROWSER_Y, DISPLAY_WIDTH,
                     VISIBLE_ITEMS * ITEM_H, COL_BG);
        int shown = min((int)fileList.size() - scrollOffset, VISIBLE_ITEMS);
        for (int i = 0; i < shown; i++) {
            drawItem(scrollOffset + i, (scrollOffset + i) == selectedIdx);
        }
    }
}

String getSelectedFile() {
    if (fileList.empty() || selectedIdx < 0 ||
        selectedIdx >= (int)fileList.size()) return "";
    return fileList[selectedIdx].path;
}

void showPlayer(const String& filename) {
    tft.fillScreen(TFT_BLACK);

    // Bottom control bar
    tft.fillRect(0, CTRL_Y, DISPLAY_WIDTH, CTRL_H, COL_BG);
    tft.fillRect(PROG_BAR_X, PROG_BAR_Y, PROG_BAR_W, PROG_BAR_H, COL_BAR_BG);

    // Filename
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    String name = filename.substring(filename.lastIndexOf('/') + 1);
    if (name.length() > 20) name = name.substring(0, 19) + "~";
    tft.drawString(name, 6, CTRL_Y + 20);

    // Play icon
    tft.fillTriangle(DISPLAY_WIDTH - 18, CTRL_Y + 14,
                     DISPLAY_WIDTH - 18, CTRL_Y + 30,
                     DISPLAY_WIDTH - 6,  CTRL_Y + 22, COL_ACCENT);

    lastProg = -1;
    isPlaying_ = true;
}

void updateProgressBar(uint32_t posMs, uint32_t durMs) {
    if (durMs == 0) return;
    int prog = (int)((long)posMs * PROG_BAR_W / durMs);
    prog = constrain(prog, 0, PROG_BAR_W);
    if (prog == lastProg) return;
    lastProg = prog;

    tft.fillRect(PROG_BAR_X, PROG_BAR_Y, PROG_BAR_W, PROG_BAR_H, COL_BAR_BG);
    if (prog > 0)
        tft.fillRect(PROG_BAR_X, PROG_BAR_Y, prog, PROG_BAR_H, COL_BAR_FG);

    // Time label
    String pos = formatDuration(posMs);
    String dur = formatDuration(durMs);
    tft.fillRect(PROG_BAR_X, PROG_BAR_Y + PROG_BAR_H + 2,
                 PROG_BAR_W, 10, COL_BG);
    tft.setTextColor(COL_SUBTEXT);
    tft.setTextSize(1);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(pos, PROG_BAR_X, PROG_BAR_Y + PROG_BAR_H + 7);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(dur, PROG_BAR_X + PROG_BAR_W, PROG_BAR_Y + PROG_BAR_H + 7);
}

void setPlayIcon(bool playing) {
    isPlaying_ = playing;
    // Erase icon area
    tft.fillRect(DISPLAY_WIDTH - 22, CTRL_Y + 10, 20, 24, COL_BG);
    if (playing) {
        tft.fillTriangle(DISPLAY_WIDTH - 18, CTRL_Y + 14,
                         DISPLAY_WIDTH - 18, CTRL_Y + 30,
                         DISPLAY_WIDTH - 6,  CTRL_Y + 22, COL_ACCENT);
    } else {
        // Pause bars
        tft.fillRect(DISPLAY_WIDTH - 18, CTRL_Y + 14, 4, 16, COL_ACCENT);
        tft.fillRect(DISPLAY_WIDTH - 11, CTRL_Y + 14, 4, 16, COL_ACCENT);
    }
}

void showVolumeOverlay(int vol) {
    lastVol = vol;
    volOverlayTime = millis();

    int overlayW = 90, overlayH = 22;
    int ox = (DISPLAY_WIDTH - overlayW) / 2;
    int oy = CTRL_Y - overlayH - 4;

    tft.fillRoundRect(ox, oy, overlayW, overlayH, 4, COL_VOL_BG);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    char buf[16];
    snprintf(buf, sizeof(buf), "Vol: %d%%", vol);
    tft.drawString(buf, ox + 6, oy + overlayH / 2);

    // Volume bar
    int barX = ox + 46, barW = overlayW - 52, barH = 8;
    int barY = oy + (overlayH - barH) / 2;
    tft.fillRect(barX, barY, barW, barH, COL_BAR_BG);
    int filled = barW * vol / 100;
    if (filled > 0)
        tft.fillRect(barX, barY, filled, barH, COL_VOL_FG);
}

bool isLoopEnabled() { return loopEnabled; }

void toggleLoop() {
    loopEnabled = !loopEnabled;
    drawBrowserHeader();
}

} // namespace UI
