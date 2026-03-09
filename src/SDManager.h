#pragma once
#include <Arduino.h>
#include <vector>

struct VideoFile {
    String path;        // Full path: /videos/myvideo.avi
    String name;        // Display name: myvideo.avi
    String thumbPath;   // /thumbs/myvideo.jpg  (may not exist)
    uint32_t sizeKB;
    bool hasThumb;
};

namespace SDManager {

bool init();
std::vector<VideoFile> getFileList();
bool fileExists(const String& path);
File openFile(const String& path);

// Thumbnail management
bool hasThumbnail(const String& videoPath);
String getThumbnailPath(const String& videoPath);

} // namespace SDManager
