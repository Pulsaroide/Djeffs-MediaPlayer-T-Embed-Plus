/*
 * VideoPlayer.cpp
 *
 * AVI/MJPEG container parser + JPEG decoder for ESP32-S3
 *
 * AVI structure:
 *   RIFF 'AVI '
 *     LIST 'hdrl'   (header)
 *       avih         main AVI header
 *       LIST 'strl'
 *         strh       stream header (vids / auds)
 *         strf       stream format
 *     LIST 'movi'   (frame data)
 *       00dc chunk   MJPEG frame
 *       01wb chunk   audio chunk
 *     idx1           index
 */

#include "VideoPlayer.h"
#include "Display.h"
#include "Config.h"
#include <SdFat.h>
#include <esp_heap_caps.h>

// ── JPEG decoder (built-in ESP32 hardware JPEG) ──────────────
#include "esp_jpg_decode.h"

// ── AVI FOURCC helpers ────────────────────────────────────────
#define FOURCC(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
static const uint32_t FOURCC_RIFF = FOURCC('R','I','F','F');
static const uint32_t FOURCC_AVI  = FOURCC('A','V','I',' ');
static const uint32_t FOURCC_LIST = FOURCC('L','I','S','T');
static const uint32_t FOURCC_hdrl = FOURCC('h','d','r','l');
static const uint32_t FOURCC_movi = FOURCC('m','o','v','i');
static const uint32_t FOURCC_avih = FOURCC('a','v','i','h');
static const uint32_t FOURCC_idx1 = FOURCC('i','d','x','1');
static const uint32_t FOURCC_00dc = FOURCC('0','0','d','c');
static const uint32_t FOURCC_01wb = FOURCC('0','1','w','b');

#pragma pack(push,1)
struct AVIMainHeader {
    uint32_t dwMicroSecPerFrame;
    uint32_t dwMaxBytesPerSec;
    uint32_t dwPaddingGranularity;
    uint32_t dwFlags;
    uint32_t dwTotalFrames;
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];
};
#pragma pack(pop)

// ── State ─────────────────────────────────────────────────────
static File     aviFile;
static bool     fileOpen      = false;
static bool     playing       = false;
static bool     paused        = false;
static bool     finished      = false;

static uint32_t moviOffset    = 0;   // byte offset of 'movi' data start
static uint32_t moviSize      = 0;
static uint32_t currentOffset = 0;   // current read position in movi

static uint16_t videoFPS      = 25;
static uint32_t totalFrames   = 0;
static uint32_t currentFrame  = 0;
static uint16_t videoWidth    = 170;
static uint16_t videoHeight   = 270;
static uint32_t durationMs    = 0;

static uint32_t frameIntervalUs = 40000;  // 25fps default
static uint32_t lastFrameTime   = 0;

// JPEG read buffer in PSRAM
static uint8_t* jpegBuf       = nullptr;
static size_t   jpegBufSize   = 0;

// ── RGB565 output buffer (written by JPEG decoder callback) ──
static uint16_t* rgbBuf       = nullptr;
static uint16_t  rgbBufW      = 0;
static uint16_t  rgbBufH      = 0;

// ── Helpers ───────────────────────────────────────────────────
static uint32_t readU32LE(File& f) {
    uint8_t b[4];
    f.read(b, 4);
    return (uint32_t)b[0] | ((uint32_t)b[1]<<8) |
           ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}

// ── JPEG → RGB565 callback ────────────────────────────────────
static bool jpegWriteCB(void* arg, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, uint8_t* data) {
    if (!data) return false;
    // data is RGB888, convert to RGB565 in rgbBuf
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint32_t px = (y + row) * rgbBufW + (x + col);
            if (px >= (uint32_t)rgbBufW * rgbBufH) continue;
            uint8_t r = data[(row * w + col) * 3 + 0];
            uint8_t g = data[(row * w + col) * 3 + 1];
            uint8_t b = data[(row * w + col) * 3 + 2];
            rgbBuf[px] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
    }
    return true;
}

// ── AVI header parser ─────────────────────────────────────────
static bool parseAVIHeader() {
    aviFile.seek(0);
    uint32_t riff  = readU32LE(aviFile);
    uint32_t fileSize = readU32LE(aviFile);
    uint32_t aviTag = readU32LE(aviFile);

    if (riff != FOURCC_RIFF || aviTag != FOURCC_AVI) {
        Serial.println("[Video] Not an AVI file");
        return false;
    }

    // Scan chunks until we find 'movi'
    bool foundAvih = false;
    while (aviFile.available()) {
        uint32_t chunkId   = readU32LE(aviFile);
        uint32_t chunkSize = readU32LE(aviFile);
        uint32_t chunkStart = aviFile.position();

        if (chunkId == FOURCC_LIST) {
            uint32_t listType = readU32LE(aviFile);
            if (listType == FOURCC_movi) {
                moviOffset  = aviFile.position();
                moviSize    = chunkSize - 4;
                currentOffset = 0;
                Serial.printf("[Video] movi @ 0x%08X, size=%u\n", moviOffset, moviSize);
                break;
            }
            // continue scanning inside hdrl
            continue;
        }

        if (chunkId == FOURCC_avih && !foundAvih) {
            AVIMainHeader hdr;
            aviFile.read((uint8_t*)&hdr, sizeof(hdr));
            videoFPS        = hdr.dwMicroSecPerFrame ? (1000000 / hdr.dwMicroSecPerFrame) : 25;
            totalFrames     = hdr.dwTotalFrames;
            videoWidth      = hdr.dwWidth;
            videoHeight     = hdr.dwHeight;
            frameIntervalUs = hdr.dwMicroSecPerFrame;
            durationMs      = (uint32_t)((uint64_t)totalFrames * hdr.dwMicroSecPerFrame / 1000);
            foundAvih       = true;
            Serial.printf("[Video] %dx%d @ %dfps, %u frames, %ums\n",
                          videoWidth, videoHeight, videoFPS, totalFrames, durationMs);
        }

        // Skip to next chunk (aligned to 2 bytes)
        uint32_t skip = chunkSize;
        if (skip & 1) skip++;
        aviFile.seek(chunkStart + skip);
    }

    return (moviOffset > 0);
}

// ── Public API ────────────────────────────────────────────────

namespace VideoPlayer {

bool open(const String& path) {
    if (fileOpen) aviFile.close();

    aviFile = SD.open(path.c_str(), FILE_READ);
    if (!aviFile) {
        Serial.printf("[Video] Cannot open: %s\n", path.c_str());
        return false;
    }

    fileOpen  = true;
    playing   = false;
    paused    = false;
    finished  = false;
    currentFrame = 0;

    if (!parseAVIHeader()) {
        aviFile.close();
        fileOpen = false;
        return false;
    }

    // Allocate JPEG buffer (PSRAM preferred)
    jpegBufSize = 64 * 1024;  // 64KB should cover any MJPEG frame
    if (!jpegBuf) {
        jpegBuf = (uint8_t*)heap_caps_malloc(jpegBufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!jpegBuf) jpegBuf = (uint8_t*)malloc(jpegBufSize);
    }

    rgbBuf  = Display::getFrameBuffer();
    rgbBufW = videoWidth;
    rgbBufH = videoHeight;

    return true;
}

void play() {
    if (!fileOpen) return;
    playing  = false;
    paused   = false;
    finished = false;
    currentFrame  = 0;
    currentOffset = 0;
    aviFile.seek(moviOffset);
    playing = true;
    lastFrameTime = micros();
}

void pause()  { paused = true;  playing = false; }
void resume() { paused = false; playing = true;  lastFrameTime = micros(); }

void stop() {
    playing  = false;
    paused   = false;
    finished = false;
    if (fileOpen) { aviFile.close(); fileOpen = false; }
    currentFrame  = 0;
    currentOffset = 0;
}

void seekToStart() {
    currentFrame  = 0;
    currentOffset = 0;
    aviFile.seek(moviOffset);
    finished = false;
    lastFrameTime = micros();
}

void tick() {
    if (!playing || paused || finished) return;

    // Frame pacing
    uint32_t now = micros();
    if ((now - lastFrameTime) < frameIntervalUs) return;
    lastFrameTime = now;

    // Read next chunk from movi
    while (true) {
        if (currentOffset + 8 > moviSize) {
            finished = true;
            playing  = false;
            return;
        }

        uint32_t chunkId   = readU32LE(aviFile);
        uint32_t chunkSize = readU32LE(aviFile);
        currentOffset += 8;

        if (chunkSize == 0) continue;

        if (chunkId == FOURCC_00dc) {
            // Video frame — read JPEG data
            if (chunkSize > jpegBufSize) {
                Serial.printf("[Video] Frame too large: %u > %u\n", chunkSize, jpegBufSize);
                aviFile.seek(aviFile.position() + chunkSize);
            } else {
                aviFile.read(jpegBuf, chunkSize);
                // Decode JPEG → RGB565 via callback
                esp_jpg_decode(jpegBuf, chunkSize, JPG_SCALE_NONE,
                               jpegWriteCB, nullptr, nullptr);
                // Push to display (center video in screen)
                int16_t yOff = 0;  // top of screen for video
                Display::pushFrameBuffer(0, yOff, videoWidth, videoHeight, rgbBuf);
                currentFrame++;
            }
            currentOffset += (chunkSize + (chunkSize & 1));
            return;  // one frame per tick

        } else if (chunkId == FOURCC_01wb) {
            // Audio — skip here, handled by AudioPlayer
            aviFile.seek(aviFile.position() + chunkSize + (chunkSize & 1));
            currentOffset += (chunkSize + (chunkSize & 1));

        } else if (chunkId == FOURCC_idx1) {
            finished = true;
            playing  = false;
            return;

        } else {
            // Unknown chunk, skip
            uint32_t skip = chunkSize + (chunkSize & 1);
            aviFile.seek(aviFile.position() + skip);
            currentOffset += skip;
        }
    }
}

bool isPlaying()  { return playing && !paused; }
bool isPaused()   { return paused; }
bool isFinished() { return finished; }

uint32_t getPositionMs() {
    if (videoFPS == 0) return 0;
    return (uint32_t)((uint64_t)currentFrame * 1000 / videoFPS);
}
uint32_t getDurationMs()  { return durationMs; }
uint16_t getFPS()          { return videoFPS; }
uint16_t getWidth()        { return videoWidth; }
uint16_t getHeight()       { return videoHeight; }
uint32_t getFrameCount()   { return totalFrames; }
uint32_t getCurrentFrame() { return currentFrame; }

} // namespace VideoPlayer
