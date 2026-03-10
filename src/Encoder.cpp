/*
 * Encoder.cpp
 * Lib officielle LilyGO T-Embed : mathertel/RotaryEncoder@^1.5.3
 * FIX: remplace ESP32Encoder (incompatible) par RotaryEncoder
 */
#include "Encoder.h"
#include "Config.h"
#include <RotaryEncoder.h>

// LATCH mode adapté à l'encodeur du T-Embed
static RotaryEncoder enc(ENCODER_INA_PIN, ENCODER_INB_PIN,
                         RotaryEncoder::LatchMode::TWO03);

static int      deltaAccum    = 0;
static long     lastPos       = 0;

// Button state machine
static bool     btnLastState  = HIGH;
static uint32_t btnPressTime  = 0;
static bool     shortPressFlag= false;
static bool     longPressFlag = false;
static bool     longFired     = false;

namespace EncoderInput {

void init() {
    pinMode(ENCODER_KEY_PIN, INPUT_PULLUP);
    // Les pins de l'encodeur sont configurées par RotaryEncoder
    Serial.println("[Encoder] Init OK (mathertel/RotaryEncoder)");
}

void update() {
    // ── Rotation ────────────────────────────────────────────
    enc.tick();   // OBLIGATOIRE : doit être appelé à chaque loop()
    long newPos = enc.getPosition();
    long diff   = newPos - lastPos;
    if (diff != 0) {
        deltaAccum += (int)diff;
        lastPos = newPos;
    }

    // ── Button ──────────────────────────────────────────────
    bool btnState = digitalRead(ENCODER_KEY_PIN);

    if (btnState == LOW && btnLastState == HIGH) {
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
