#pragma once

// ============================================================
//  LilyGo T-Embed CC1101 — Hardware Configuration
//  Source officielle : https://github.com/Xinyuan-LilyGO/T-Embed-CC1101
//  Hardware : v1.0-240729
// ============================================================

// ── Power ────────────────────────────────────────────────────
#define BOARD_PWR_EN        15
#define BOARD_USER_KEY      6

// ── Display (ST7789 1.9" 170x320) ───────────────────────────
#define DISPLAY_WIDTH       170
#define DISPLAY_HEIGHT      320
#define DISPLAY_BL          21
#define DISPLAY_CS          41
#define DISPLAY_MOSI        9
#define DISPLAY_SCLK        11
#define DISPLAY_DC          16
#define DISPLAY_RST         40
// NOTE: pin 40 partagée DISPLAY_RST + BOARD_VOICE_LRCLK
// Voulu par LilyGO : RST actif au boot seulement, I2S ensuite.

// ── SPI Bus (partagé Display + SD) ───────────────────────────
#define BOARD_SPI_SCK       11
#define BOARD_SPI_MOSI      9
#define BOARD_SPI_MISO      10

// ── SD Card ──────────────────────────────────────────────────
// Corrections vs version originale : CS=13, MISO=10, SCK=11
#define SD_CS               13     // BOARD_SD_CS
#define SD_MOSI             BOARD_SPI_MOSI
#define SD_MISO             BOARD_SPI_MISO
#define SD_SCLK             BOARD_SPI_SCK

// ── I2S Audio (Speaker) ──────────────────────────────────────
#define I2S_BCLK_PIN        46     // BOARD_VOICE_BCLK
#define I2S_LRCLK_PIN       40     // BOARD_VOICE_LRCLK
#define I2S_DIN_PIN         7      // BOARD_VOICE_DIN
#define I2S_PORT            I2S_NUM_0

// ── Encoder (mathertel/RotaryEncoder) ────────────────────────
#define ENCODER_INA_PIN     4
#define ENCODER_INB_PIN     5
#define ENCODER_KEY_PIN     0
#define USER_KEY_PIN        6
#define LONG_PRESS_MS       700

// ── WS2812 RGB LEDs ──────────────────────────────────────────
#define WS2812_DATA_PIN     14
#define WS2812_NUM_LEDS     8

// ── Microphone (I2S) ─────────────────────────────────────────
#define MIC_DATA_PIN        42
#define MIC_CLK_PIN         39

// ── I2C ──────────────────────────────────────────────────────
#define BOARD_I2C_SDA       8
#define BOARD_I2C_SCL       18

// ── Video / MJPEG ────────────────────────────────────────────
#define VIDEO_WIDTH         170
#define VIDEO_HEIGHT        (320 - UI_CONTROLS_HEIGHT)
#define UI_CONTROLS_HEIGHT  50
// PSRAM 8MB OPI — RGB565 framebuffer + marge JPEG
#define MJPEG_BUFFER_SIZE   (170 * 320 * 2 + 4096)

// ── SD file paths ────────────────────────────────────────────
#define VIDEO_FOLDER        "/videos"
#define THUMB_FOLDER        "/thumbs"

// ── Audio ────────────────────────────────────────────────────
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_BUFFER_SIZE   8192   // profite de la PSRAM 8MB
#define DEFAULT_VOLUME      70     // 0-100
