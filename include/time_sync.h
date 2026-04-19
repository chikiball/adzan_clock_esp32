// ============================================================
// time_sync.h — NTP Time Synchronisation (SGT UTC+8)
// ============================================================
#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>

class TimeSync {
public:
    void begin();
    bool syncNTP();                  // Force NTP sync
    bool isTimeSynced();
    void getTime(int& hour, int& min, int& sec);
    String getTimeString();          // "HH:MM:SS"
    String getDateString();          // "19 April 2026"
    int getHour();
    int getMinute();
    int getSecond();

private:
    bool _synced = false;
};

extern TimeSync timeSync;

#endif // TIME_SYNC_H
