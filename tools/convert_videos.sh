#!/usr/bin/env bash
# ============================================================
#  convert_videos.sh
#  Converts any video to MJPEG AVI for LilyGo T-Embed
#  Screen: 170x270 pixels (video area), 25fps, MJPEG q=5
#  Audio:  PCM 16-bit stereo 44100 Hz
#
#  Usage:
#    chmod +x convert_videos.sh
#    ./convert_videos.sh input.mp4
#    ./convert_videos.sh *.mp4         (batch)
#    ./convert_videos.sh /path/to/dir  (all videos in folder)
# ============================================================

VIDEO_W=170
VIDEO_H=270
FPS=25
QUALITY=5      # MJPEG quality: 1=best, 31=worst  (5 = excellent)
AUDIO_RATE=44100
OUTPUT_DIR="./converted"

# Colors
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

check_ffmpeg() {
    if ! command -v ffmpeg &>/dev/null; then
        echo -e "${RED}ERROR: ffmpeg not found.${NC}"
        echo "Install: https://ffmpeg.org/download.html"
        exit 1
    fi
    echo -e "${GREEN}✓ ffmpeg found: $(ffmpeg -version 2>&1 | head -1)${NC}"
}

convert_file() {
    local input="$1"
    local basename=$(basename "${input%.*}")
    local output="${OUTPUT_DIR}/${basename}.avi"

    echo -e "\n${YELLOW}Converting:${NC} $input"
    echo -e "  → ${output}"

    # Get source info
    local src_info
    src_info=$(ffprobe -v quiet -select_streams v:0 \
               -show_entries stream=width,height,r_frame_rate \
               -of csv=p=0 "$input" 2>/dev/null)
    echo "  Source: $src_info"

    ffmpeg -y -i "$input" \
        -vf "scale=${VIDEO_W}:${VIDEO_H}:force_original_aspect_ratio=decrease,pad=${VIDEO_W}:${VIDEO_H}:(ow-iw)/2:(oh-ih)/2:black,fps=${FPS}" \
        -vcodec mjpeg \
        -q:v ${QUALITY} \
        -acodec pcm_s16le \
        -ar ${AUDIO_RATE} \
        -ac 2 \
        -f avi \
        "${output}" 2>&1 | grep -E "frame=|fps=|size=|time=|error|Error" | tail -5

    if [ $? -eq 0 ]; then
        local size=$(du -h "${output}" | cut -f1)
        echo -e "  ${GREEN}✓ Done — ${size}${NC}"
        generate_thumbnail "$input" "$basename"
    else
        echo -e "  ${RED}✗ Conversion failed${NC}"
    fi
}

generate_thumbnail() {
    local input="$1"
    local basename="$2"
    local thumb="${OUTPUT_DIR}/thumbs/${basename}.jpg"

    mkdir -p "${OUTPUT_DIR}/thumbs"

    ffmpeg -y -i "$input" \
        -ss 00:00:01 \
        -vframes 1 \
        -vf "scale=48:30:force_original_aspect_ratio=decrease,pad=48:30:(ow-iw)/2:(oh-ih)/2:black" \
        "${thumb}" -loglevel quiet

    [ -f "${thumb}" ] && echo "  ${GREEN}✓ Thumbnail generated${NC}"
}

main() {
    echo "============================================"
    echo " Djeff's MediaPlayer For T-Embed Plus — Video Converter"
    echo " Target: ${VIDEO_W}x${VIDEO_H} @ ${FPS}fps MJPEG"
    echo "============================================"

    check_ffmpeg
    mkdir -p "${OUTPUT_DIR}"

    if [ $# -eq 0 ]; then
        echo "Usage: $0 <file.mp4> [file2.mp4 ...]"
        echo "       $0 /path/to/videos/"
        exit 1
    fi

    for arg in "$@"; do
        if [ -d "$arg" ]; then
            # Directory — process all video files
            find "$arg" -type f \( \
                -iname "*.mp4" -o -iname "*.mkv" -o -iname "*.mov" \
                -o -iname "*.avi" -o -iname "*.webm" -o -iname "*.mpg" \
            \) | while read -r f; do
                convert_file "$f"
            done
        elif [ -f "$arg" ]; then
            convert_file "$arg"
        else
            echo -e "${RED}Not found: $arg${NC}"
        fi
    done

    echo -e "\n${GREEN}============================================"
    echo " Conversion complete!"
    echo " Copy files from: ${OUTPUT_DIR}/"
    echo " → /videos/ on your SD card"
    echo " → /thumbs/ folder too (for thumbnails)"
    echo -e "============================================${NC}"
}

main "$@"
