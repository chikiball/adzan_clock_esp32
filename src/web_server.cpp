// ============================================================
// web_server.cpp — Async Web Server for Configuration
// ============================================================
#include "web_server.h"
#include "config.h"
#include "wifi_manager.h"
#include "audio_player.h"
#include "display_manager.h"
#include "prayer_time.h"
#include "time_sync.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

WebServerManager webServer;

static AsyncWebServer server(80);

void WebServerManager::begin() {
    setupRoutes();
    server.begin();
    _running = true;
    Serial.println("[WEB] Server started on port 80");
}

void WebServerManager::stop() {
    server.end();
    _running = false;
}

bool WebServerManager::isRunning() {
    return _running;
}

void WebServerManager::setupRoutes() {

    // Serve the config page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    // API: Get current status
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["time"] = timeSync.getTimeString();
        doc["date"] = timeSync.getDateString();
        doc["hijri"] = prayerTime.getHijriDate();
        doc["wifi_mode"] = (wifiMgr.getCurrentMode() == CLOCK_MODE_AP) ? "AP" : "STA";
        doc["ip"] = wifiMgr.getIP();
        doc["next_prayer"] = prayerTime.getNextPrayerInfo();

        JsonArray prayers = doc["prayers"].to<JsonArray>();
        for (int i = 0; i < prayerTime.prayerCount; i++) {
            JsonObject p = prayers.add<JsonObject>();
            p["name"] = prayerTime.prayers[i].name;
            char timeStr[6];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
                     prayerTime.prayers[i].hour, prayerTime.prayers[i].minute);
            p["time"] = timeStr;
            p["triggered"] = prayerTime.prayers[i].triggered;
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // API: Save WiFi credentials (POST with JSON body)
    server.on("/api/wifi", HTTP_POST,
        // Request handler — called after body is fully received
        [](AsyncWebServerRequest* request) {
            // Body was parsed in onBody callback, respond here
            // (fallback if body handler didn't fire)
            if (!request->_tempObject) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
            }
        },
        NULL,
        // Body handler — parse JSON and respond
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                // Complete body received in one chunk
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, (char*)data, len);

                if (err) {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                    request->_tempObject = (void*)1;
                    return;
                }

                String ssid = doc["ssid"].as<String>();
                String password = doc["password"].as<String>();

                if (ssid.length() > 0) {
                    wifiMgr.saveCredentials(ssid, password);
                    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Credentials saved. Restart to connect.\"}");
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID required\"}");
                }
                request->_tempObject = (void*)1;  // Mark as handled
            }
        }
    );

    // API: Set volume (POST with JSON body)
    server.on("/api/volume", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            if (!request->_tempObject)
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data\"}");
        },
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                JsonDocument doc;
                deserializeJson(doc, (char*)data, len);
                int vol = doc["volume"] | DEFAULT_VOLUME;
                audioPlayer.setVolume(vol);
                request->send(200, "application/json", "{\"status\":\"ok\"}");
                request->_tempObject = (void*)1;
            }
        }
    );

    // API: Set brightness (POST with JSON body)
    server.on("/api/brightness", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            if (!request->_tempObject)
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data\"}");
        },
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                JsonDocument doc;
                deserializeJson(doc, (char*)data, len);
                int brightness = doc["brightness"] | DEFAULT_BRIGHTNESS;
                display.setBrightness(brightness);
                request->send(200, "application/json", "{\"status\":\"ok\"}");
                request->_tempObject = (void*)1;
            }
        }
    );

    // API: Test adzan
    server.on("/api/test-adzan", HTTP_POST, [](AsyncWebServerRequest* request) {
        audioPlayer.playAdzan(false);
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Playing test adzan\"}");
    });

    // API: Restart ESP
    server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Restarting...\"}");
        delay(500);
        ESP.restart();
    });
}
