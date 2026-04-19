// ============================================================
// prayer_time.h — MUIS Prayer Times API Client & Scheduler
// ============================================================
#ifndef PRAYER_TIME_H
#define PRAYER_TIME_H

#include <Arduino.h>

struct PrayerSchedule {
    String name;      // "Subuh", "Zohor", "Asar", "Maghrib", "Isyak"
    int hour;
    int minute;
    bool triggered;   // Already played adzan today
};

class PrayerTimeManager {
public:
    void begin();
    bool fetchToday();                           // Fetch from MUIS API
    String getNextPrayerInfo();                  // "Zohor 13:05" for display
    bool isAdzanTime(int hour, int min);         // Check if any prayer matches
    String getCurrentAdzanName();                // Name of prayer currently triggering
    void resetTriggers();                        // Reset at midnight
    String getHijriDate();                       // From API response
    String getFriendlyDate();                    // From API response

    PrayerSchedule prayers[5];                   // 5 prayer times
    int prayerCount = 0;

private:
    String _hijriDate;
    String _friendlyDate;
    unsigned long _lastFetch = 0;
    bool _fetched = false;
    bool parsePrayerTime(const String& timeStr, int& hour, int& minute);
};

extern PrayerTimeManager prayerTime;

#endif // PRAYER_TIME_H
