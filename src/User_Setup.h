// ============================================================
//  TFT_eSPI User Setup — LilyGO T-Embed CC1101
//  ST7789 170x320
// ============================================================

#define USER_SETUP_ID 214

#define ST7789_DRIVER
#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define TFT_CS   41
#define TFT_DC   16
#define TFT_RST  40
#define TFT_MOSI  9
#define TFT_SCLK 11
#define TFT_BL   21

#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
