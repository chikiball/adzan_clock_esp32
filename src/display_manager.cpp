// ============================================================
// display_manager.cpp — MAX7219 32×16 Dot Matrix Display
// ============================================================
#include "display_manager.h"
#include "config.h"
#include <SPI.h>

// Hardware type for FC-16 style modules (most common)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// Two virtual zones: top row (zone 1) and bottom row (zone 0)
// MAX7219 daisy chain: modules are wired bottom-row-last → top-row-first
// Zone 0 = devices 0–3 (bottom row), Zone 1 = devices 4–7 (top row)
static MD_Parola parola = MD_Parola(HARDWARE_TYPE, PIN_MAX7219_DIN, PIN_MAX7219_CLK, PIN_MAX7219_CS, MAX_DEVICES);

DisplayManager display;

void DisplayManager::begin() {
    parola.begin(2);  // 2 zones

    // Zone 0: bottom row (devices 0–3) — scrolling prayer info
    parola.setZone(0, 0, 3);
    parola.displayZoneText(0, "", PA_CENTER, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

    // Zone 1: top row (devices 4–7) — static time display
    parola.setZone(1, 4, 7);
    parola.displayZoneText(1, "", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);

    setBrightness(DEFAULT_BRIGHTNESS);

    Serial.println("[DISPLAY] Initialised 32x16 (2 zones)");
}

void DisplayManager::update() {
    parola.displayAnimate();
}

void DisplayManager::showTime(int hour, int min, int sec) {
    // Blink the colon every COLON_BLINK_MS
    unsigned long now = millis();
    if (now - _lastColonToggle >= COLON_BLINK_MS) {
        _colonVisible = !_colonVisible;
        _lastColonToggle = now;
    }

    static char timeBuf[9];
    if (_colonVisible) {
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour, min);
    } else {
        snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", hour, min);
    }

    if (parola.getZoneStatus(1)) {
        parola.displayZoneText(1, timeBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    }
}

void DisplayManager::showPrayerInfo(const String& text) {
    static char prayerBuf[64];
    text.toCharArray(prayerBuf, sizeof(prayerBuf));

    if (parola.getZoneStatus(0)) {
        parola.displayZoneText(0, prayerBuf, PA_LEFT, SCROLL_SPEED, 2000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
}

void DisplayManager::showMessage(const String& line1, const String& line2) {
    static char buf1[32], buf2[32];
    line1.toCharArray(buf1, sizeof(buf1));
    line2.toCharArray(buf2, sizeof(buf2));

    parola.displayZoneText(1, buf1, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    parola.displayZoneText(0, buf2, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    parola.displayAnimate();
}

void DisplayManager::setBrightness(uint8_t level) {
    parola.setIntensity(level);
}

void DisplayManager::showAdzanAlert(const String& prayerName) {
    static char alertBuf[32];
    prayerName.toCharArray(alertBuf, sizeof(alertBuf));

    // Flash the display with the prayer name
    parola.displayZoneText(1, alertBuf, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    parola.displayZoneText(0, "ADZAN", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    parola.displayAnimate();
}
