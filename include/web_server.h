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
    bool isRunning();

private:
    bool _running = false;
    void setupRoutes();
};

extern WebServerManager webServer;

#endif // WEB_SERVER_H
