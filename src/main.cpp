// ============================================================
// main.cpp — Adzan Clock Entry Point
// ============================================================
#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "wifi_manager.h"
#include "time_sync.h"
#include "prayer_time.h"
#include "display_manager.h"
#include "audio_player.h"
#include "web_server.h"

// Button state
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;

// Midnight reset tracking
int lastDay = -1;

void IRAM_ATTR onButtonPress() {
    unsigned long now = millis();
    if (now - lastButtonPress > BUTTON_DEBOUNCE_MS) {
        buttonPressed = true;
        lastButtonPress = now;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Adzan Clock ESP32 ===");

    // Initialise LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[ERROR] LittleFS mount failed");
    }

    // Initialise button with interrupt
    pinMode(PIN_MODE_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_MODE_BUTTON), onButtonPress, FALLING);

    // Initialise modules
    display.begin();
    audioPlayer.begin();
    wifiMgr.begin();

    // Show startup message
    display.showMessage("Adzan", "Clock");
    delay(1500);

    // Try connecting to saved WiFi, fallback to AP
    if (!wifiMgr.connectSTA()) {
        Serial.println("[WIFI] STA failed, starting AP mode");
        wifiMgr.startAP();
        display.showMessage("AP Mode", AP_SSID);
    } else {
        Serial.println("[WIFI] Connected: " + wifiMgr.getIP());
        display.showMessage("WiFi OK", wifiMgr.getIP().c_str());
    }

    // Start web server (available in both AP and STA modes)
    webServer.begin();

    // If in STA mode, sync time and fetch prayer times
    if (wifiMgr.getCurrentMode() == MODE_STA) {
        timeSync.begin();

        if (timeSync.syncNTP()) {
            Serial.println("[NTP] Time synced: " + timeSync.getTimeString());
        }

        if (prayerTime.fetchToday()) {
            Serial.println("[PRAYER] Fetched today's prayer times");
            for (int i = 0; i < prayerTime.prayerCount; i++) {
                Serial.printf("  %s: %02d:%02d\n",
                    prayerTime.prayers[i].name.c_str(),
                    prayerTime.prayers[i].hour,
                    prayerTime.prayers[i].minute);
            }
        }
    }

    delay(1000);
}

void loop() {
    // Handle button press — toggle WiFi mode
    if (buttonPressed) {
        buttonPressed = false;
        wifiMgr.toggleMode();

        if (wifiMgr.getCurrentMode() == MODE_AP) {
            display.showMessage("AP Mode", AP_SSID);
        } else {
            if (wifiMgr.connectSTA()) {
                display.showMessage("WiFi OK", wifiMgr.getIP().c_str());
                timeSync.syncNTP();
                prayerTime.fetchToday();
            } else {
                display.showMessage("WiFi", "FAILED");
                delay(1000);
                wifiMgr.startAP();
                display.showMessage("AP Mode", AP_SSID);
            }
        }
    }

    // Update display
    if (wifiMgr.getCurrentMode() == MODE_STA && timeSync.isTimeSynced()) {
        int h, m, s;
        timeSync.getTime(h, m, s);

        // Show time on row 1
        display.showTime(h, m, s);

        // Show next prayer info on row 2
        display.showPrayerInfo(prayerTime.getNextPrayerInfo());

        // Check for adzan time
        if (prayerTime.isAdzanTime(h, m)) {
            String prayerName = prayerTime.getCurrentAdzanName();
            Serial.println("[ADZAN] Time for " + prayerName);
            display.showAdzanAlert(prayerName);
            audioPlayer.playAdzan(false);  // Short version; set true for full
        }

        // Midnight reset — re-fetch prayer times for new day
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        if (timeinfo.tm_mday != lastDay) {
            lastDay = timeinfo.tm_mday;
            prayerTime.resetTriggers();
            prayerTime.fetchToday();
        }
    }

    // Feed audio buffer if playing
    audioPlayer.loop();

    // Update display animations
    display.update();
}
