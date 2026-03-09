#include "Display.h"
#include "Config.h"

TFT_eSPI tft = TFT_eSPI();

// PSRAM framebuffer for MJPEG
static uint16_t* frameBuffer = nullptr;

namespace Display {

void init() {
    tft.init();
    tft.setRotation(0);   // Portrait, USB at bottom
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // Backlight
    ledcAttach(DISPLAY_BL, 5000, 8);
    ledcWrite(DISPLAY_BL, 200);

    // Allocate framebuffer in PSRAM
    frameBuffer = (uint16_t*)ps_malloc(MJPEG_BUFFER_SIZE);
    if (!frameBuffer) {
        Serial.println("[Display] WARNING: PSRAM framebuffer allocation failed, using heap");
        frameBuffer = (uint16_t*)malloc(MJPEG_BUFFER_SIZE);
    }
    Serial.printf("[Display] Init OK — %dx%d, FB=%p\n",
                  DISPLAY_WIDTH, DISPLAY_HEIGHT, frameBuffer);
}

void showSplash() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.drawString("Djeff's MediaPlayer", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 20);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("For T-Embed Plus", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 10);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("Loading SD...", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 35);
    delay(1200);
}

void showError(const String& msg) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.drawString("ERROR", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 20);
    tft.setTextColor(TFT_WHITE);
    int y = DISPLAY_HEIGHT/2;
    // Split msg on \n
    String line;
    for (int i = 0; i <= (int)msg.length(); i++) {
        if (i == (int)msg.length() || msg[i] == '\n') {
            tft.drawString(line, DISPLAY_WIDTH/2, y);
            y += 14;
            line = "";
        } else {
            line += msg[i];
        }
    }
}

void setBrightness(uint8_t level) {
    ledcWrite(DISPLAY_BL, level);
}

void clear(uint16_t color) {
    tft.fillScreen(color);
}

uint16_t* getFrameBuffer() {
    return frameBuffer;
}

void pushFrameBuffer(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* buf) {
    tft.pushImage(x, y, w, h, buf);
}

} // namespace Display
