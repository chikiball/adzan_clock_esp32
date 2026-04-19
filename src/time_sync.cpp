// ============================================================
// time_sync.cpp — NTP Time Synchronisation (SGT UTC+8)
// ============================================================
#include "time_sync.h"
#include "config.h"
#include <time.h>

TimeSync timeSync;

void TimeSync::begin() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
}

bool TimeSync::syncNTP() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
        _synced = true;
        return true;
    }
    _synced = false;
    return false;
}

bool TimeSync::isTimeSynced() {
    return _synced;
}

void TimeSync::getTime(int& hour, int& min, int& sec) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        hour = timeinfo.tm_hour;
        min = timeinfo.tm_min;
        sec = timeinfo.tm_sec;
    }
}

String TimeSync::getTimeString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "??:??:??";

    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

String TimeSync::getDateString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";

    char buf[32];
    strftime(buf, sizeof(buf), "%d %B %Y", &timeinfo);
    return String(buf);
}

int TimeSync::getHour() {
    struct tm t;
    getLocalTime(&t);
    return t.tm_hour;
}

int TimeSync::getMinute() {
    struct tm t;
    getLocalTime(&t);
    return t.tm_min;
}

int TimeSync::getSecond() {
    struct tm t;
    getLocalTime(&t);
    return t.tm_sec;
}
