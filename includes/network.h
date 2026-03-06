#pragma once

#include <Arduino.h>

enum class NetworkMode {
  ETHERNET,
  WIFI_STA_MODE,
  WIFI_AP_MODE
};

extern NetworkMode currentNetworkMode;
extern bool ethernetConnected;
extern bool wifiConnected;

extern String wifi_ssid;
extern String wifi_password;
extern bool wifi_use_static_ip;
extern IPAddress wifi_static_ip;
extern IPAddress wifi_gateway;
extern IPAddress wifi_subnet;
extern IPAddress wifi_dns1;
extern IPAddress wifi_dns2;

extern IPAddress eth_ip;
extern IPAddress eth_gateway;
extern IPAddress eth_subnet;

void networkInit();

IPAddress networkGetCurrentIp();
String networkGetModeString();
String networkGetApSsid();

void loadWiFiConfig();
void saveWiFiConfig(const String& ssid,
                    const String& password,
                    bool useStaticIP,
                    const String& staticIP,
                    const String& gateway,
                    const String& subnet,
                    const String& dns1,
                    const String& dns2);

void loadLanConfig();
void saveLanConfig(const String& ipAddress,
                   const String& gateway,
                   const String& subnet);
