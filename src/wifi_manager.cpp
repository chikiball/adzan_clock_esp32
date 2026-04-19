// ============================================================
// wifi_manager.cpp — AP/STA WiFi Mode Manager
// ============================================================
#include "wifi_manager.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

WifiManager wifiMgr;

void WifiManager::begin() {
    WiFi.mode(WIFI_OFF);
    delay(100);
}

void WifiManager::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    _currentMode = MODE_AP;
    Serial.println("[WIFI] AP started: " + String(AP_SSID));
    Serial.println("[WIFI] AP IP: " + WiFi.softAPIP().toString());
}

bool WifiManager::connectSTA() {
    String ssid, password;
    if (!loadCredentials(ssid, password)) {
        Serial.println("[WIFI] No saved credentials");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.print("[WIFI] Connecting to " + ssid);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _currentMode = MODE_STA;
        Serial.println("[WIFI] Connected! IP: " + WiFi.localIP().toString());
        return true;
    }

    Serial.println("[WIFI] Connection failed");
    return false;
}

void WifiManager::toggleMode() {
    if (_currentMode == MODE_AP) {
        _currentMode = MODE_STA;
    } else {
        _currentMode = MODE_AP;
        startAP();
    }
}

WiFiMode_t WifiManager::getCurrentMode() {
    return _currentMode;
}

String WifiManager::getIP() {
    if (_currentMode == MODE_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

void WifiManager::saveCredentials(const String& ssid, const String& password) {
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File file = LittleFS.open(CONFIG_FILE, "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("[WIFI] Credentials saved");
    }
}

bool WifiManager::loadCredentials(String& ssid, String& password) {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) return false;

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    return ssid.length() > 0;
}
