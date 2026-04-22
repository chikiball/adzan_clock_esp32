// ============================================================
// time_sync.cpp — NTP Time Synchronisation (SGT UTC+8)
// ============================================================
#include "time_sync.h"
#include "config.h"
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

TimeSync timeSync;

void TimeSync::begin() {
    // Try multiple NTP servers
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET,
               "pool.ntp.org", "time.google.com", "time.nist.gov");
    Serial.println("[NTP] Configured NTP servers");
}

bool TimeSync::syncNTP() {
    // Try NTP first (10 second timeout)
    struct tm timeinfo;
    Serial.println("[NTP] Attempting NTP sync...");
    if (getLocalTime(&timeinfo, 10000)) {
        _synced = true;
        Serial.println("[NTP] NTP sync successful");
        return true;
    }

    // NTP failed — fallback: get time from WorldTimeAPI via HTTP
    Serial.println("[NTP] NTP failed, trying HTTP time API fallback...");
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://worldtimeapi.org/api/timezone/Asia/Singapore");
        int httpCode = http.GET();
        if (httpCode == 200) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            long unixtime = doc["unixtime"] | 0;
            if (unixtime > 0) {
                // Set system time from unix timestamp
                struct timeval tv;
                tv.tv_sec = unixtime;
                tv.tv_usec = 0;
                settimeofday(&tv, NULL);
                // Re-configure timezone
                setenv("TZ", "SGT-8", 1);
                tzset();
                _synced = true;
                Serial.println("[NTP] HTTP time sync successful");
                http.end();
                return true;
            }
        }
        http.end();
    }

    _synced = false;
    Serial.println("[NTP] All sync methods failed");
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
