#include "SDManager.h"
#include "Config.h"
#include <SdFat.h>

static SdFs SD;
static bool sdReady = false;

namespace SDManager {

bool init() {
    SdSpiConfig spiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(25));
    // Try up to 3 times
    for (int attempt = 0; attempt < 3; attempt++) {
        if (SD.begin(spiConfig)) {
            sdReady = true;
            // Create folders if missing
            if (!SD.exists(VIDEO_FOLDER)) SD.mkdir(VIDEO_FOLDER);
            if (!SD.exists(THUMB_FOLDER)) SD.mkdir(THUMB_FOLDER);
            Serial.printf("[SD] Init OK (attempt %d)\n", attempt + 1);
            return true;
        }
        delay(300);
    }
    Serial.println("[SD] Init FAILED");
    return false;
}

std::vector<VideoFile> getFileList() {
    std::vector<VideoFile> list;
    if (!sdReady) return list;

    FsFile dir = SD.open(VIDEO_FOLDER);
    if (!dir) return list;

    FsFile entry;
    while (entry.openNext(&dir, O_RDONLY)) {
        char nameBuf[64];
        entry.getName(nameBuf, sizeof(nameBuf));
        String name = String(nameBuf);

        if (!entry.isDir() &&
            (name.endsWith(".avi") || name.endsWith(".AVI"))) {
            VideoFile vf;
            vf.name = name;
            vf.path = String(VIDEO_FOLDER) + "/" + name;
            vf.sizeKB = (uint32_t)(entry.fileSize() / 1024);

            // Check thumbnail
            String baseName = name.substring(0, name.lastIndexOf('.'));
            vf.thumbPath = String(THUMB_FOLDER) + "/" + baseName + ".jpg";
            vf.hasThumb = SD.exists(vf.thumbPath.c_str());

            list.push_back(vf);
        }
        entry.close();
    }
    dir.close();

    Serial.printf("[SD] Found %d video(s)\n", (int)list.size());
    return list;
}

bool fileExists(const String& path) {
    return sdReady && SD.exists(path.c_str());
}

File openFile(const String& path) {
    return SD.open(path.c_str(), FILE_READ);
}

bool hasThumbnail(const String& videoPath) {
    String base = videoPath.substring(videoPath.lastIndexOf('/') + 1);
    base = base.substring(0, base.lastIndexOf('.'));
    String thumbPath = String(THUMB_FOLDER) + "/" + base + ".jpg";
    return SD.exists(thumbPath.c_str());
}

String getThumbnailPath(const String& videoPath) {
    String base = videoPath.substring(videoPath.lastIndexOf('/') + 1);
    base = base.substring(0, base.lastIndexOf('.'));
    return String(THUMB_FOLDER) + "/" + base + ".jpg";
}

} // namespace SDManager
