#pragma once
// DeskPet Web Server — async HTTP on port 80 + mDNS deskpet.local
// Serves REST API and static web UI from LittleFS /web/

#include <Arduino.h>

void webServerInit();         // start server, mDNS, register all routes
bool webServerRunning();      // true when WiFi connected + server up

// Circular log buffer for /api/log
void   webLog(const char* fmt, ...);
String webLogGet();           // returns JSON array of last N lines
