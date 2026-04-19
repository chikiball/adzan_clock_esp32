// ============================================================
// display_manager.h — MAX7219 32×16 Dot Matrix Display
// ============================================================
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

class DisplayManager {
public:
    void begin();
    void update();                              // Call in loop()
    void showTime(int hour, int min, int sec);  // Row 1: HH:MM with blinking colon
    void showPrayerInfo(const String& text);    // Row 2: scrolling prayer info
    void showMessage(const String& line1, const String& line2);  // Static message
    void setBrightness(uint8_t level);          // 0–15
    void showAdzanAlert(const String& prayerName);  // Flash alert during adzan

private:
    bool _colonVisible = true;
    unsigned long _lastColonToggle = 0;
};

extern DisplayManager display;

#endif // DISPLAY_MANAGER_H
