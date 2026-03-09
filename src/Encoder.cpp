#include "Encoder.h"
#include "Config.h"
#include <ESP32Encoder.h>

static ESP32Encoder enc;
static int32_t  lastCount  = 0;
static int      deltaAccum = 0;

// Button state machine
static bool    btnLastState   = HIGH;
static uint32_t btnPressTime  = 0;
static bool    shortPressFlag = false;
static bool    longPressFlag  = false;
static bool    longFired      = false;

namespace EncoderInput {

void init() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    enc.attachHalfQuad(ENCODER_INA_PIN, ENCODER_INB_PIN);
    enc.clearCount();
    lastCount = 0;

    pinMode(ENCODER_KEY_PIN, INPUT_PULLUP);
    Serial.println("[Encoder] Init OK");
}

void update() {
    // ── Rotation ────────────────────────────────────────────
    int32_t count = enc.getCount();
    int32_t diff  = count - lastCount;
    if (diff != 0) {
        deltaAccum += (int)(diff / 2);  // half-quad → divide by 2
        lastCount = count;
    }

    // ── Button ──────────────────────────────────────────────
    bool btnState = digitalRead(ENCODER_KEY_PIN);

    if (btnState == LOW && btnLastState == HIGH) {
        // Press start
        btnPressTime = millis();
        longFired    = false;
    }

    if (btnState == LOW && !longFired) {
        if (millis() - btnPressTime >= LONG_PRESS_MS) {
            longPressFlag = true;
            longFired     = true;
        }
    }

    if (btnState == HIGH && btnLastState == LOW) {
        // Release
        if (!longFired && (millis() - btnPressTime < LONG_PRESS_MS)) {
            shortPressFlag = true;
        }
    }

    btnLastState = btnState;
}

int getDelta() {
    int d = deltaAccum;
    deltaAccum = 0;
    return d;
}

bool wasPressed() {
    if (shortPressFlag) { shortPressFlag = false; return true; }
    return false;
}

bool wasLongPressed() {
    if (longPressFlag) { longPressFlag = false; return true; }
    return false;
}

} // namespace EncoderInput
