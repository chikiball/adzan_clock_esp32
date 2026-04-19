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
    Serial.println("  time    — Toggle printing current time every 5s");
    Serial.println("  mode    — Toggle WiFi mode (AP <-> STA)");
    Serial.println("  status  — Print current status");
    Serial.println("  help    — Show this help");
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
    // Process serial commands
    processSerial();

    // Debug: print time every 5 seconds if enabled
    if (debugPrintTime && timeSync.isTimeSynced()) {
        unsigned long now = millis();
        if (now - lastTimePrint >= TIME_PRINT_INTERVAL) {
            lastTimePrint = now;
            Serial.println("[TIME] " + timeSync.getTimeString() + " | Next: " + prayerTime.getNextPrayerInfo());
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

    // Process DNS captive portal requests
    webServer.loop();

    // Update display animations
    display.update();
}
