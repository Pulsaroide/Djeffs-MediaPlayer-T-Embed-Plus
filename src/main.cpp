/*
 * ============================================================
 *  Djeff's MediaPlayer For T-Embed Plus
 *  Main entry point
 *
 *  Features:
 *   - MJPEG video playback (AVI container) from SD card
 *   - MP3 / AAC audio via I2S speaker
 *   - File browser with thumbnails
 *   - Play / Pause / Stop
 *   - Progress bar
 *   - Volume control via rotary encoder
 *   - Loop playback
 * ============================================================
 */

#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "SDManager.h"
#include "AudioPlayer.h"
#include "VideoPlayer.h"
#include "UI.h"
#include "Encoder.h"

// ── App state machine ─────────────────────────────────────────
enum AppState {
    STATE_BROWSER,
    STATE_PLAYING,
    STATE_PAUSED
};

static AppState appState = STATE_BROWSER;

// ── Forward declarations ──────────────────────────────────────
void handleBrowserInput();
void handlePlayerInput();

// =============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[MediaPlayer] Booting...");

    // Power enable
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);
    delay(100);

    Display::init();
    Display::showSplash();

    EncoderInput::init();

    if (!SDManager::init()) {
        Display::showError("SD Card not found!\nInsert SD and reboot.");
        while (true) delay(1000);
    }

    AudioPlayer::init();
    UI::init();

    Serial.println("[MediaPlayer] Ready.");
    UI::showBrowser(SDManager::getFileList());
}

// =============================================================
void loop() {
    EncoderInput::update();

    switch (appState) {
        case STATE_BROWSER:
            handleBrowserInput();
            break;

        case STATE_PLAYING:
            VideoPlayer::tick();          // decode & push next frame
            AudioPlayer::tick();          // refill I2S buffer
            UI::updateProgressBar(
                VideoPlayer::getPositionMs(),
                VideoPlayer::getDurationMs()
            );
            handlePlayerInput();

            if (VideoPlayer::isFinished()) {
                if (UI::isLoopEnabled()) {
                    VideoPlayer::seekToStart();
                } else {
                    VideoPlayer::stop();
                    AudioPlayer::stop();
                    appState = STATE_BROWSER;
                    UI::showBrowser(SDManager::getFileList());
                }
            }
            break;

        case STATE_PAUSED:
            handlePlayerInput();
            break;
    }
}

// =============================================================
void handleBrowserInput() {
    int delta = EncoderInput::getDelta();
    if (delta != 0) {
        UI::scrollBrowser(delta);
    }

    if (EncoderInput::wasPressed()) {
        String selected = UI::getSelectedFile();
        if (selected.length() > 0) {
            Serial.printf("[MediaPlayer] Opening: %s\n", selected.c_str());
            if (VideoPlayer::open(selected) && AudioPlayer::open(selected)) {
                UI::showPlayer(selected);
                VideoPlayer::play();
                AudioPlayer::play();
                appState = STATE_PLAYING;
            } else {
                Display::showError("Cannot open file:\n" + selected);
                delay(2000);
                UI::showBrowser(SDManager::getFileList());
            }
        }
    }
}

// =============================================================
void handlePlayerInput() {
    int delta = EncoderInput::getDelta();
    if (delta != 0) {
        // Encoder rotation = volume
        int vol = AudioPlayer::getVolume() + delta * 5;
        vol = constrain(vol, 0, 100);
        AudioPlayer::setVolume(vol);
        UI::showVolumeOverlay(vol);
    }

    if (EncoderInput::wasPressed()) {
        if (appState == STATE_PLAYING) {
            VideoPlayer::pause();
            AudioPlayer::pause();
            appState = STATE_PAUSED;
            UI::setPlayIcon(false);
        } else {
            VideoPlayer::resume();
            AudioPlayer::resume();
            appState = STATE_PLAYING;
            UI::setPlayIcon(true);
        }
    }

    if (EncoderInput::wasLongPressed()) {
        // Long press = back to browser
        VideoPlayer::stop();
        AudioPlayer::stop();
        appState = STATE_BROWSER;
        UI::showBrowser(SDManager::getFileList());
    }
}
