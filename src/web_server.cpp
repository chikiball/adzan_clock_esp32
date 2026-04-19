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
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

WebServerManager webServer;

static AsyncWebServer server(80);
static DNSServer dnsServer;
static bool dnsRunning = false;

void WebServerManager::begin() {
    setupRoutes();
    server.begin();
    _running = true;
    Serial.println("[WEB] Server started on port 80");
}

void WebServerManager::startDNS() {
    // Redirect all DNS queries to our IP (captive portal)
    dnsServer.start(53, "*", AP_IP);
    dnsRunning = true;
    Serial.println("[WEB] DNS captive portal started");
}

void WebServerManager::stopDNS() {
    if (dnsRunning) {
        dnsServer.stop();
        dnsRunning = false;
    }
}

void WebServerManager::loop() {
    if (dnsRunning) {
        dnsServer.processNextRequest();
    }
}

void WebServerManager::stop() {
    server.end();
    stopDNS();
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

    // API: Save WiFi credentials (plain HTML form POST)
    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("ssid", true)) {
            String ssid = request->getParam("ssid", true)->value();
            String password = request->hasParam("password", true) ?
                              request->getParam("password", true)->value() : "";

            if (ssid.length() > 0) {
                wifiMgr.saveCredentials(ssid, password);
                Serial.println("[WEB] WiFi credentials saved: " + ssid);

                // Return a simple HTML page confirming save + auto-restart
                String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<style>body{background:#0a0a0a;color:#e0e0e0;font-family:sans-serif;"
                    "display:flex;justify-content:center;align-items:center;height:100vh;"
                    "text-align:center;}</style></head><body>"
                    "<div><h2>&#9989; WiFi Saved!</h2>"
                    "<p>SSID: <b>" + ssid + "</b></p>"
                    "<p>Restarting in 3 seconds...</p>"
                    "</div></body></html>";
                request->send(200, "text/html", html);

                // Schedule restart
                delay(2000);
                ESP.restart();
            } else {
                request->send(400, "text/html", "<h2>Error: SSID is empty</h2><a href='/'>Back</a>");
            }
        } else {
            request->send(400, "text/html", "<h2>Error: No SSID provided</h2><a href='/'>Back</a>");
        }
    });

    // API: Set volume (POST form-encoded)
    server.on("/api/volume", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("volume", true)) {
            int vol = request->getParam("volume", true)->value().toInt();
            audioPlayer.setVolume(vol);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"volume param missing\"}");
        }
    });

    // API: Set brightness (POST form-encoded)
    server.on("/api/brightness", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("brightness", true)) {
            int brightness = request->getParam("brightness", true)->value().toInt();
            display.setBrightness(brightness);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"brightness param missing\"}");
        }
    });

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

    // Captive portal: redirect any unknown requests to our config page
    // This handles OS connectivity checks (Apple CNA, Android, Windows)
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.1/");
    });
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.1/");
    });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.1/");
    });
    server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.1/");
    });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.1/");
    });

    // Catch-all: redirect unknown paths to root
    server.onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_GET) {
            request->redirect("http://192.168.4.1/");
        } else {
            request->send(404, "text/plain", "Not found");
        }
    });
}
