@echo off
:: ============================================================
::  convert_videos.bat  (Windows)
::  Converts videos to MJPEG AVI for LilyGo T-Embed
::
::  Usage:
::    Drag & drop a video file onto this .bat
::    OR run: convert_videos.bat input.mp4
:: ============================================================

set VIDEO_W=170
set VIDEO_H=270
set FPS=25
set QUALITY=5
set AUDIO_RATE=44100
set OUTPUT_DIR=converted

:: Check FFmpeg
where ffmpeg >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: ffmpeg not found in PATH.
    echo Download from: https://ffmpeg.org/download.html
    echo Add ffmpeg\bin to your PATH environment variable.
    pause
    exit /b 1
)

if "%~1"=="" (
    echo Usage: Drag a video file onto this batch file
    echo        OR: convert_videos.bat input.mp4
    pause
    exit /b 1
)

:: Create output folder
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%OUTPUT_DIR%\thumbs" mkdir "%OUTPUT_DIR%\thumbs"

set INPUT=%~1
set BASENAME=%~n1
set OUTPUT=%OUTPUT_DIR%\%BASENAME%.avi
set THUMB=%OUTPUT_DIR%\thumbs\%BASENAME%.jpg

echo ============================================
echo  Djeff's MediaPlayer For T-Embed Plus - Video Converter
echo  Input:  %INPUT%
echo  Output: %OUTPUT%
echo  Format: %VIDEO_W%x%VIDEO_H% @ %FPS%fps MJPEG
echo ============================================

:: Convert video
ffmpeg -y -i "%INPUT%" ^
    -vf "scale=%VIDEO_W%:%VIDEO_H%:force_original_aspect_ratio=decrease,pad=%VIDEO_W%:%VIDEO_H%:(ow-iw)/2:(oh-ih)/2:black,fps=%FPS%" ^
    -vcodec mjpeg ^
    -q:v %QUALITY% ^
    -acodec pcm_s16le ^
    -ar %AUDIO_RATE% ^
    -ac 2 ^
    -f avi ^
    "%OUTPUT%"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Conversion failed!
    pause
    exit /b 1
)

:: Generate thumbnail
ffmpeg -y -i "%INPUT%" ^
    -ss 00:00:01 ^
    -vframes 1 ^
    -vf "scale=48:30:force_original_aspect_ratio=decrease,pad=48:30:(ow-iw)/2:(oh-ih)/2:black" ^
    "%THUMB%" -loglevel quiet

echo.
echo ============================================
echo  SUCCESS!
echo  Copy to SD card:
echo    %OUTPUT%       ->  /videos/
echo    %THUMB%        ->  /thumbs/
echo ============================================
pause
