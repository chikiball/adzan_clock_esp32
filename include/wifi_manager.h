// ============================================================
// wifi_manager.h — AP/STA WiFi Mode Manager
// ============================================================
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

enum ClockWiFiMode {
    CLOCK_MODE_AP,       // Access Point — config portal
    CLOCK_MODE_STA       // Station — connected to home WiFi
};

class WifiManager {
public:
    void begin();
    void toggleMode();
    void startAP();
    bool connectSTA();
    ClockWiFiMode getCurrentMode();
    String getIP();

    // Stored credentials
    void saveCredentials(const String& ssid, const String& password);
    bool loadCredentials(String& ssid, String& password);

private:
    ClockWiFiMode _currentMode = CLOCK_MODE_STA;
    String _ssid;
    String _password;
};

extern WifiManager wifiMgr;

#endif // WIFI_MANAGER_H
