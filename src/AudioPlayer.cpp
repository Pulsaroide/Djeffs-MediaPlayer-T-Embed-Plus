/*
 * AudioPlayer.cpp
 *
 * Reads PCM audio chunks (01wb) from the AVI file and
 * feeds them to the I2S peripheral.
 *
 * The AVI MJPEG files created by our FFmpeg script embed
 * PCM 16-bit stereo @ 44100 Hz in the 01wb chunks.
 *
 * We use a separate SdFat file handle so video and audio
 * can seek independently.
 */

#include "AudioPlayer.h"
#include "Config.h"
#include <driver/i2s.h>
#include <SdFat.h>

// ── AVI FOURCC ────────────────────────────────────────────────
#define FOURCC(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
static const uint32_t FOURCC_RIFF = FOURCC('R','I','F','F');
static const uint32_t FOURCC_AVI  = FOURCC('A','V','I',' ');
static const uint32_t FOURCC_LIST = FOURCC('L','I','S','T');
static const uint32_t FOURCC_movi = FOURCC('m','o','v','i');
static const uint32_t FOURCC_01wb = FOURCC('0','1','w','b');
static const uint32_t FOURCC_00dc = FOURCC('0','0','d','c');
static const uint32_t FOURCC_idx1 = FOURCC('i','d','x','1');

static FsFile   audioFile;
static bool     fileOpen   = false;

// Forward declare the shared SD object from SDManager
extern SdFs SD;
static bool     playing    = false;
static bool     paused     = false;
static int      volume     = DEFAULT_VOLUME;   // 0-100

static uint32_t moviOffset = 0;
static uint32_t moviSize   = 0;
static uint32_t audioPos   = 0;

// PCM buffer
static int16_t* pcmBuf     = nullptr;
static const size_t PCM_BUF_SAMPLES = AUDIO_BUFFER_SIZE / 2;

static uint32_t readU32LE(FsFile& f) {
    uint8_t b[4];
    f.read(b, 4);
    return (uint32_t)b[0] | ((uint32_t)b[1]<<8) |
           ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}

// Apply software volume scaling
static void applyVolume(int16_t* buf, size_t samples) {
    if (volume >= 100) return;
    float scale = volume / 100.0f;
    for (size_t i = 0; i < samples; i++) {
        buf[i] = (int16_t)(buf[i] * scale);
    }
}

namespace AudioPlayer {

bool init() {
    i2s_config_t i2sCfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = AUDIO_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 512,
        .use_apll             = true,
        .tx_desc_auto_clear   = true,
    };

    i2s_pin_config_t pinCfg = {
        .mck_io_num   = I2S_PIN_NO_CHANGE,
        .bck_io_num   = I2S_BCLK_PIN,
        .ws_io_num    = I2S_LRCLK_PIN,
        .data_out_num = I2S_DIN_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2sCfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[Audio] i2s_driver_install failed: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_PORT, &pinCfg);
    if (err != ESP_OK) {
        Serial.printf("[Audio] i2s_set_pin failed: %d\n", err);
        return false;
    }

    i2s_zero_dma_buffer(I2S_PORT);

    pcmBuf = (int16_t*)heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcmBuf) pcmBuf = (int16_t*)malloc(AUDIO_BUFFER_SIZE);

    Serial.println("[Audio] I2S init OK");
    return true;
}

bool open(const String& aviPath) {
    if (fileOpen) audioFile.close();

    audioFile = SD.open(aviPath.c_str(), O_RDONLY);
    if (!audioFile) return false;

    fileOpen = true;
    moviOffset = 0;
    moviSize   = 0;
    audioPos   = 0;

    // Quick scan for movi offset
    audioFile.seek(12);  // skip RIFF header
    while (audioFile.available()) {
        uint32_t chunkId   = readU32LE(audioFile);
        uint32_t chunkSize = readU32LE(audioFile);
        uint32_t chunkStart = audioFile.position();

        if (chunkId == FOURCC_LIST) {
            uint32_t listType = readU32LE(audioFile);
            if (listType == FOURCC_movi) {
                moviOffset = audioFile.position();
                moviSize   = chunkSize - 4;
                break;
            }
            continue;
        }
        uint32_t skip = chunkSize + (chunkSize & 1);
        audioFile.seek(chunkStart + skip);
    }

    if (moviOffset == 0) {
        Serial.println("[Audio] movi not found");
        audioFile.close();
        fileOpen = false;
        return false;
    }

    Serial.printf("[Audio] Ready — movi @ 0x%08X\n", moviOffset);
    return true;
}

void play() {
    if (!fileOpen) return;
    playing  = true;
    paused   = false;
    audioPos = 0;
    audioFile.seek(moviOffset);
    i2s_zero_dma_buffer(I2S_PORT);
}

void pause()  { paused = true; i2s_zero_dma_buffer(I2S_PORT); }
void resume() { paused = false; }

void stop() {
    playing = false;
    paused  = false;
    if (fileOpen) { audioFile.close(); fileOpen = false; }
    i2s_zero_dma_buffer(I2S_PORT);
}

void tick() {
    if (!playing || paused || !fileOpen) return;
    if (audioPos + 8 > moviSize) return;

    // Find and consume next audio chunk (01wb)
    // We may encounter video chunks first — skip those
    for (int guard = 0; guard < 32; guard++) {
        if (audioPos + 8 > moviSize) return;

        uint32_t chunkId   = readU32LE(audioFile);
        uint32_t chunkSize = readU32LE(audioFile);
        audioPos += 8;

        if (chunkId == FOURCC_01wb && chunkSize > 0) {
            size_t toRead = min((size_t)chunkSize, (size_t)AUDIO_BUFFER_SIZE);
            audioFile.read((uint8_t*)pcmBuf, toRead);
            applyVolume(pcmBuf, toRead / 2);

            size_t written = 0;
            i2s_write(I2S_PORT, pcmBuf, toRead, &written, pdMS_TO_TICKS(10));

            uint32_t skip = chunkSize - toRead + (chunkSize & 1);
            if (skip > 0) audioFile.seek(audioFile.position() + skip);
            audioPos += (chunkSize + (chunkSize & 1));
            return;

        } else if (chunkId == FOURCC_00dc || chunkId == FOURCC_idx1) {
            // Skip video frame or index
            uint32_t skip = chunkSize + (chunkSize & 1);
            audioFile.seek(audioFile.position() + skip);
            audioPos += skip;

        } else {
            // Unknown — skip
            uint32_t skip = chunkSize + (chunkSize & 1);
            audioFile.seek(audioFile.position() + skip);
            audioPos += skip;
        }
    }
}

void setVolume(int vol) {
    volume = constrain(vol, 0, 100);
}
int getVolume() { return volume; }

bool isReady() { return fileOpen; }

} // namespace AudioPlayer
