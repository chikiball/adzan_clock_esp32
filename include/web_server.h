// ============================================================
// web_server.h — Async Web Server for Configuration
// ============================================================
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

class WebServerManager {
public:
    void begin();
    void stop();
    void loop();           // Must call in main loop() for DNS processing
    void startDNS();       // Start captive portal DNS (call in AP mode)
    void stopDNS();        // Stop DNS (call when switching to STA)
    bool isRunning();

private:
    bool _running = false;
    void setupRoutes();
};

extern WebServerManager webServer;

#endif // WEB_SERVER_H
