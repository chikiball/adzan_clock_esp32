// ============================================================
// main.cpp — Adzan Clock Entry Point
// ============================================================
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
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

// Serial debug flags
bool debugPrintTime = false;
unsigned long lastTimePrint = 0;
#define TIME_PRINT_INTERVAL 5000

// Serial command buffer
String serialBuffer = "";

void IRAM_ATTR onButtonPress() {
    unsigned long now = millis();
    if (now - lastButtonPress > BUTTON_DEBOUNCE_MS) {
        buttonPressed = true;
        lastButtonPress = now;
    }
}

// ============================================================
// Serial Command Handler
// ============================================================
void printSerialHelp() {
    Serial.println("\n=== Serial Commands ===");
    Serial.println("  time          — Toggle printing current time every 5s");
    Serial.println("  mode          — Toggle WiFi mode (AP <-> STA)");
    Serial.println("  status        — Print current status");
    Serial.println("  setnext HH:MM — Set next prayer time (temporary, for testing)");
    Serial.println("  fetch         — Re-fetch prayer times from API (resets setnext)");
    Serial.println("  help          — Show this help");
    Serial.println("========================\n");
}

void handleSerialCommand(const String& cmd) {
    String command = cmd;
    command.trim();
    command.toLowerCase();

    if (command == "time") {
        debugPrintTime = !debugPrintTime;
        Serial.printf("[DEBUG] Time printing: %s\n", debugPrintTime ? "ON (every 5s)" : "OFF");

    } else if (command == "mode") {
        Serial.println("[DEBUG] Toggling WiFi mode...");
        wifiMgr.toggleMode();

        if (wifiMgr.getCurrentMode() == CLOCK_MODE_AP) {
            Serial.println("[DEBUG] Switched to AP mode: " + String(AP_SSID));
            webServer.startDNS();
            display.showMessage("AP Mode", AP_SSID);
        } else {
            webServer.stopDNS();
            if (wifiMgr.connectSTA()) {
                Serial.println("[DEBUG] Switched to STA mode: " + wifiMgr.getIP());
                display.showMessage("WiFi OK", wifiMgr.getIP().c_str());
                timeSync.syncNTP();
                prayerTime.fetchToday();
            } else {
                Serial.println("[DEBUG] STA connection failed, reverting to AP");
                wifiMgr.startAP();
                webServer.startDNS();
                display.showMessage("AP Mode", AP_SSID);
            }
        }

    } else if (command == "status") {
        Serial.println("\n=== Status ===");
        Serial.println("  Mode:   " + String(wifiMgr.getCurrentMode() == CLOCK_MODE_AP ? "AP" : "STA"));
        Serial.println("  IP:     " + wifiMgr.getIP());
        Serial.println("  Time:   " + timeSync.getTimeString());
        Serial.println("  Synced: " + String(timeSync.isTimeSynced() ? "Yes" : "No"));
        Serial.println("  Next:   " + prayerTime.getNextPrayerInfo());
        Serial.println("  Debug:  Time print " + String(debugPrintTime ? "ON" : "OFF"));
        Serial.println("==============\n");

    } else if (command.startsWith("setnext")) {
        // setnext HH:MM — override the next upcoming prayer time for testing
        String timeArg = command.substring(7);
        timeArg.trim();
        int colonIdx = timeArg.indexOf(':');
        if (colonIdx > 0) {
            int h = timeArg.substring(0, colonIdx).toInt();
            int m = timeArg.substring(colonIdx + 1).toInt();
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                // Find the next untriggered prayer and override it
                bool found = false;
                struct tm timeinfo;
                int nowMinutes = 0;
                if (getLocalTime(&timeinfo)) {
                    nowMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
                }
                for (int i = 0; i < prayerTime.prayerCount; i++) {
                    int pMin = prayerTime.prayers[i].hour * 60 + prayerTime.prayers[i].minute;
                    if (pMin > nowMinutes && !prayerTime.prayers[i].triggered) {
                        Serial.printf("[DEBUG] Changed %s from %02d:%02d to %02d:%02d (temporary)\n",
                            prayerTime.prayers[i].name.c_str(),
                            prayerTime.prayers[i].hour, prayerTime.prayers[i].minute,
                            h, m);
                        prayerTime.prayers[i].hour = h;
                        prayerTime.prayers[i].minute = m;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Serial.println("[DEBUG] No upcoming prayer to override. Overriding first prayer.");
                    if (prayerTime.prayerCount > 0) {
                        prayerTime.prayers[0].hour = h;
                        prayerTime.prayers[0].minute = m;
                        prayerTime.prayers[0].triggered = false;
                        Serial.printf("[DEBUG] Set %s to %02d:%02d\n",
                            prayerTime.prayers[0].name.c_str(), h, m);
                    }
                }
            } else {
                Serial.println("[DEBUG] Invalid time. Use: setnext HH:MM (e.g. setnext 14:30)");
            }
        } else {
            Serial.println("[DEBUG] Usage: setnext HH:MM (e.g. setnext 14:30)");
        }

    } else if (command == "fetch") {
        Serial.println("[DEBUG] Re-fetching prayer times from API...");
        if (WiFi.status() == WL_CONNECTED) {
            if (prayerTime.fetchToday()) {
                Serial.println("[DEBUG] Prayer times updated:");
                for (int i = 0; i < prayerTime.prayerCount; i++) {
                    Serial.printf("  %s: %02d:%02d%s\n",
                        prayerTime.prayers[i].name.c_str(),
                        prayerTime.prayers[i].hour, prayerTime.prayers[i].minute,
                        prayerTime.prayers[i].triggered ? " (done)" : "");
                }
            } else {
                Serial.println("[DEBUG] Fetch failed!");
            }
        } else {
            Serial.println("[DEBUG] No WiFi connection. Switch to STA mode first.");
        }

    } else if (command == "help") {
        printSerialHelp();

    } else if (command.length() > 0) {
        Serial.println("[DEBUG] Unknown command: '" + command + "'. Type 'help' for commands.");
    }
}

void processSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                handleSerialCommand(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
}

void setup() {
    Serial.begin(115200);
    disableCore0WDT();  // Prevent async_tcp watchdog timeout (known ESPAsyncWebServer issue)
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
        webServer.startDNS();  // Captive portal DNS in AP mode
        display.showMessage("AP Mode", AP_SSID);
    } else {
        Serial.println("[WIFI] Connected: " + wifiMgr.getIP());
        display.showMessage("WiFi OK", wifiMgr.getIP().c_str());
    }

    // Start web server (available in both AP and STA modes)
    webServer.begin();

    // If in STA mode, sync time and fetch prayer times
    if (wifiMgr.getCurrentMode() == CLOCK_MODE_STA) {
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
    printSerialHelp();
}

void loop() {
    static bool firstLoop = true;
    if (firstLoop) { Serial.println("[LOOP] Entered main loop"); firstLoop = false; }
    // Process serial commands
    processSerial();

    // Debug: print time every 5 seconds if enabled
    if (debugPrintTime) {
        unsigned long now = millis();
        if (now - lastTimePrint >= TIME_PRINT_INTERVAL) {
            lastTimePrint = now;
            struct tm ti;
            if (getLocalTime(&ti, 0)) {
                Serial.printf("[TIME] %02d:%02d:%02d | Next: %s\n",
                    ti.tm_hour, ti.tm_min, ti.tm_sec,
                    prayerTime.getNextPrayerInfo().c_str());
            } else {
                Serial.println("[TIME] Not synced yet");
            }
        }
    }

    // Handle button press — toggle WiFi mode
    if (buttonPressed) {
        buttonPressed = false;
        wifiMgr.toggleMode();

        if (wifiMgr.getCurrentMode() == CLOCK_MODE_AP) {
            webServer.startDNS();
            display.showMessage("AP Mode", AP_SSID);
        } else {
            webServer.stopDNS();
            if (wifiMgr.connectSTA()) {
                display.showMessage("WiFi OK", wifiMgr.getIP().c_str());
                timeSync.syncNTP();
                prayerTime.fetchToday();
            } else {
                display.showMessage("WiFi", "FAILED");
                delay(1000);
                wifiMgr.startAP();
                webServer.startDNS();
                display.showMessage("AP Mode", AP_SSID);
            }
        }
    }

    // Update display
    if (wifiMgr.getCurrentMode() == CLOCK_MODE_STA && timeSync.isTimeSynced()) {
        int h, m, s;
        timeSync.getTime(h, m, s);

        // Show time on row 1
        display.showTime(h, m, s);

        // Show next prayer info on row 2
        display.showPrayerInfo(prayerTime.getNextPrayerInfo());

        // Midnight reset — re-fetch prayer times for new day
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        if (timeinfo.tm_mday != lastDay) {
            lastDay = timeinfo.tm_mday;
            prayerTime.resetTriggers();
            prayerTime.fetchToday();
        }
    }

    // Check for adzan time (works even if display isn't updating)
    if (prayerTime.prayerCount > 0) {
        struct tm ti;
        if (getLocalTime(&ti, 0)) {
            int h = ti.tm_hour;
            int m = ti.tm_min;
            int s = ti.tm_sec;
            if (prayerTime.isAdzanTime(h, m)) {
                String prayerName = prayerTime.getCurrentAdzanName();
                Serial.println("\n=============================");
                Serial.println("[ADZAN] >>> TIME FOR " + prayerName + " <<<");
                Serial.printf("[ADZAN] Triggered at %02d:%02d:%02d\n", h, m, s);
                Serial.println("=============================\n");
                display.showAdzanAlert(prayerName);
                audioPlayer.playAdzan(false);  // Short version; set true for full
            }
        }
    }

    // Feed audio buffer if playing
    audioPlayer.loop();

    // Process DNS captive portal requests
    webServer.loop();

    // Update display animations
    display.update();

    // Yield to prevent watchdog reset
    delay(1);
}
