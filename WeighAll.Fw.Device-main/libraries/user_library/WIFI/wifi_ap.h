#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "driver_config.h"

void startCaptivePortal();
void stopCaptivePortal();
void handleCaptiveTasks();

#endif  // WIFI_CONNECT_H