// ============================================================
// prayer_time.cpp — MUIS Prayer Times API Client & Scheduler
// ============================================================
#include "prayer_time.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

PrayerTimeManager prayerTime;

// Prayer names matching MUIS API keys (skip "syuruk" — it's sunrise, not a prayer)
static const char* PRAYER_KEYS[] = {"subuh", "zohor", "asar", "maghrib", "isyak"};
static const char* PRAYER_NAMES[] = {"Subuh", "Zohor", "Asar", "Maghrib", "Isyak"};

void PrayerTimeManager::begin() {
    prayerCount = 0;
    _fetched = false;
}

bool PrayerTimeManager::fetchToday() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[PRAYER] No WiFi connection");
        return false;
    }

    HTTPClient http;
    http.begin(PRAYER_API_URL);
    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("[PRAYER] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse JSON response
    // Expected: {"date":"...","hijri_date":"...","times":{"subuh":"05:41",...}}
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.println("[PRAYER] JSON parse error: " + String(err.c_str()));
        return false;
    }

    _hijriDate = doc["hijri_date"].as<String>();
    _friendlyDate = doc["friendly_date"].as<String>();

    JsonObject times = doc["times"];
    prayerCount = 0;

    for (int i = 0; i < PRAYER_COUNT; i++) {
        String timeStr = times[PRAYER_KEYS[i]].as<String>();
        int h, m;
        if (parsePrayerTime(timeStr, h, m)) {
            prayers[prayerCount].name = PRAYER_NAMES[i];
            prayers[prayerCount].hour = h;
            prayers[prayerCount].minute = m;
            prayers[prayerCount].triggered = false;
            prayerCount++;
        }
    }

    _fetched = true;
    _lastFetch = millis();
    Serial.printf("[PRAYER] Loaded %d prayer times for %s\n",
                  prayerCount, _friendlyDate.c_str());
    return true;
}

bool PrayerTimeManager::parsePrayerTime(const String& timeStr, int& hour, int& minute) {
    // Parse "HH:MM" format
    int colonIdx = timeStr.indexOf(':');
    if (colonIdx < 0) return false;

    hour = timeStr.substring(0, colonIdx).toInt();
    minute = timeStr.substring(colonIdx + 1).toInt();
    return true;
}

String PrayerTimeManager::getNextPrayerInfo() {
    if (!_fetched || prayerCount == 0) return "No data";

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "No time";

    int nowMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    for (int i = 0; i < prayerCount; i++) {
        int prayerMinutes = prayers[i].hour * 60 + prayers[i].minute;
        if (prayerMinutes > nowMinutes) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s %02d:%02d",
                     prayers[i].name.c_str(),
                     prayers[i].hour, prayers[i].minute);
            return String(buf);
        }
    }

    // All prayers passed — show first prayer of tomorrow
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %02d:%02d*",
             prayers[0].name.c_str(),
             prayers[0].hour, prayers[0].minute);
    return String(buf);
}

bool PrayerTimeManager::isAdzanTime(int hour, int min) {
    for (int i = 0; i < prayerCount; i++) {
        if (prayers[i].hour == hour &&
            prayers[i].minute == min &&
            !prayers[i].triggered) {
            prayers[i].triggered = true;
            return true;
        }
    }
    return false;
}

String PrayerTimeManager::getCurrentAdzanName() {
    // Return the last triggered prayer name
    for (int i = prayerCount - 1; i >= 0; i--) {
        if (prayers[i].triggered) {
            return prayers[i].name;
        }
    }
    return "";
}

void PrayerTimeManager::resetTriggers() {
    for (int i = 0; i < prayerCount; i++) {
        prayers[i].triggered = false;
    }
    Serial.println("[PRAYER] Triggers reset for new day");
}

String PrayerTimeManager::getHijriDate() {
    return _hijriDate;
}

String PrayerTimeManager::getFriendlyDate() {
    return _friendlyDate;
}
