#pragma once

// ============================================================
//  LilyGo T-Embed CC1101 Plus — Hardware Configuration
// ============================================================

// ── Power ────────────────────────────────────────────────────
#define BOARD_PWR_EN        15

// ── Display (ST7789V 1.9" 170x320) ──────────────────────────
#define DISPLAY_WIDTH       170
#define DISPLAY_HEIGHT      320
#define DISPLAY_BL          21
#define DISPLAY_CS          41
#define DISPLAY_MOSI        9
#define DISPLAY_SCLK        11
#define DISPLAY_DC          16
#define DISPLAY_RST         40

// ── SD Card (shared SPI bus with display) ───────────────────
#define SD_CS               10
#define SD_MOSI             11
#define SD_MISO             13
#define SD_SCLK             12

// ── I2S Audio (Speaker) ──────────────────────────────────────
#define I2S_BCLK_PIN        46
#define I2S_LRCLK_PIN       40
#define I2S_DIN_PIN         7
#define I2S_PORT            I2S_NUM_0

// ── Encoder ──────────────────────────────────────────────────
#define ENCODER_INA_PIN     4
#define ENCODER_INB_PIN     5
#define ENCODER_KEY_PIN     0
#define USER_KEY_PIN        6
#define LONG_PRESS_MS       700

// ── WS2812 RGB LEDs ─────────────────────────────────────────
#define WS2812_DATA_PIN     14
#define WS2812_NUM_LEDS     8

// ── Microphone (I2S) ─────────────────────────────────────────
#define MIC_DATA_PIN        42
#define MIC_CLK_PIN         39

// ── Video settings ───────────────────────────────────────────
#define VIDEO_WIDTH         170
#define VIDEO_HEIGHT        (320 - UI_CONTROLS_HEIGHT)   // leave room for UI
#define UI_CONTROLS_HEIGHT  50
#define MJPEG_BUFFER_SIZE   (170 * 320 * 2 + 2048)       // RGB565 + headroom

// ── SD file paths ────────────────────────────────────────────
#define VIDEO_FOLDER        "/videos"
#define THUMB_FOLDER        "/thumbs"
#define VIDEO_EXTENSIONS    {".avi", ".AVI"}

// ── Audio ────────────────────────────────────────────────────
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_BUFFER_SIZE   4096
#define DEFAULT_VOLUME      70    // 0-100
