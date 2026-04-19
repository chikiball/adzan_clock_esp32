#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// Pin Definitions
// ============================================================

// MAX7219 Dot Matrix (SPI)
#define PIN_MAX7219_DIN   23   // MOSI
#define PIN_MAX7219_CLK   18   // SCK
#define PIN_MAX7219_CS     5   // Chip Select

// MAX7219 Display Configuration
#define MAX_DEVICES        8   // 2 modules × 4 units each = 8 total MAX7219 chips
#define DISPLAY_COLS      32   // Pixels wide per row
#define DISPLAY_ROWS       2   // 2 rows stacked

// I2S Audio → UDA1334A DAC
#define PIN_I2S_BCLK      27   // Bit Clock
#define PIN_I2S_LRC        25   // Word Select (Left/Right Clock)
#define PIN_I2S_DOUT       26   // Serial Data Out

// Mode Switch Button
#define PIN_MODE_BUTTON     4   // Active LOW with internal pull-up
#define BUTTON_DEBOUNCE_MS 50

// ============================================================
// WiFi Defaults
// ============================================================
#define AP_SSID          "AdzanClock"
#define AP_PASSWORD      "12345678"
#define AP_IP            IPAddress(192, 168, 4, 1)

// ============================================================
// NTP Configuration
// ============================================================
#define NTP_SERVER       "pool.ntp.org"
#define GMT_OFFSET_SEC   28800    // UTC+8 (Singapore)
#define DAYLIGHT_OFFSET  0        // No DST in Singapore

// ============================================================
// MUIS Prayer Times API
// ============================================================
#define PRAYER_API_URL   "https://api.prayertimes.sg/api/v1/prayer-times/today"
#define PRAYER_FETCH_INTERVAL_MS  3600000  // Re-fetch every 1 hour as fallback
#define PRAYER_COUNT     5         // Subuh, Zohor, Asar, Maghrib, Isyak (skip Syuruk)

// ============================================================
// Audio
// ============================================================
#define ADZAN_SHORT_PATH "/adzan_short.mp3"
#define ADZAN_FULL_PATH  "/adzan_full.mp3"
#define DEFAULT_VOLUME   80        // 0-100

// ============================================================
// Display
// ============================================================
#define DEFAULT_BRIGHTNESS  5      // 0-15 for MAX7219
#define SCROLL_SPEED       50      // ms per frame for scrolling text
#define COLON_BLINK_MS    500      // Blink colon every 500ms

// ============================================================
// Config Storage (LittleFS)
// ============================================================
#define CONFIG_FILE      "/config.json"

#endif // CONFIG_H
