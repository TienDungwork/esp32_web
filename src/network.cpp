#include "network.h"

#include <WiFi.h>
#include <WebServer_ESP32_SC_W5500.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

NetworkMode currentNetworkMode = NetworkMode::WIFI_AP_MODE;
bool ethernetConnected = false;
bool wifiConnected = false;

String wifi_ssid = "";
String wifi_password = "";
bool wifi_use_static_ip = false;
IPAddress wifi_static_ip;
IPAddress wifi_gateway;
IPAddress wifi_subnet;
IPAddress wifi_dns1;
IPAddress wifi_dns2;

IPAddress eth_ip(192, 168, 41, 200);
IPAddress eth_gateway(192, 168, 41, 1);
IPAddress eth_subnet(255, 255, 255, 0);

static const char* AP_SSID = "esp32s3";
static const char* AP_PASSWORD = "123456789";

#ifdef INT_GPIO
  #undef INT_GPIO
#endif
#ifdef MISO_GPIO
  #undef MISO_GPIO
#endif
#ifdef MOSI_GPIO
  #undef MOSI_GPIO
#endif
#ifdef SCK_GPIO
  #undef SCK_GPIO
#endif
#ifdef CS_GPIO
  #undef CS_GPIO
#endif

#define INT_GPIO      45
#define MISO_GPIO     37
#define MOSI_GPIO     35
#define SCK_GPIO      36
#define CS_GPIO       48
#define SPI_CLOCK_MHZ 25

static IPAddress eth_dns1(8, 8, 8, 8);
static IPAddress eth_dns2(8, 8, 4, 4);
static byte eth_mac[6];

void loadWiFiConfig() {
  wifi_static_ip = IPAddress(192, 168, 1, 201);
  wifi_gateway = IPAddress(192, 168, 1, 1);
  wifi_subnet = IPAddress(255, 255, 255, 0);
  wifi_dns1 = IPAddress(8, 8, 8, 8);
  wifi_dns2 = IPAddress(8, 8, 4, 4);

  if (LittleFS.exists("/wifi_config.json")) {
    File file = LittleFS.open("/wifi_config.json", "r");
    if (file) {
      String content = file.readString();
      file.close();

      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, content) == DeserializationError::Ok) {
        wifi_ssid = doc["ssid"].as<String>();
        wifi_password = doc["password"].as<String>();
        wifi_use_static_ip = doc["use_static_ip"] | false;

        if (wifi_use_static_ip) {
          String ip_str = doc["static_ip"].as<String>();
          String gw_str = doc["gateway"].as<String>();
          String sn_str = doc["subnet"].as<String>();
          String dns1_str = doc["dns1"].as<String>();
          String dns2_str = doc["dns2"].as<String>();

          if (ip_str.length() > 0) wifi_static_ip.fromString(ip_str);
          if (gw_str.length() > 0) wifi_gateway.fromString(gw_str);
          if (sn_str.length() > 0) wifi_subnet.fromString(sn_str);
          if (dns1_str.length() > 0) wifi_dns1.fromString(dns1_str);
          if (dns2_str.length() > 0) wifi_dns2.fromString(dns2_str);
        }
        Serial.println("WiFi config loaded: " + wifi_ssid);
      }
    }
  } else {
    Serial.println("No WiFi config found, using defaults");
  }

  // Fallback test WiFi only when nothing is configured in storage.
  if (wifi_ssid.length() == 0) {
    wifi_ssid = "Phuc An";
    wifi_password = "88889999";
    Serial.println("WiFi fallback TEST: " + wifi_ssid);
  }

  Serial.println("WiFi runtime config: SSID=" + wifi_ssid +
                 " staticIP=" + String(wifi_use_static_ip ? "true" : "false"));
}

void saveWiFiConfig(const String& ssid,
                    const String& password,
                    bool useStaticIP,
                    const String& staticIP,
                    const String& gateway,
                    const String& subnet,
                    const String& dns1,
                    const String& dns2) {
  DynamicJsonDocument doc(1024);
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["use_static_ip"] = useStaticIP;

  if (useStaticIP) {
    doc["static_ip"] = staticIP;
    doc["gateway"] = gateway;
    doc["subnet"] = subnet;
    doc["dns1"] = dns1;
    doc["dns2"] = dns2;
  }

  File file = LittleFS.open("/wifi_config.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("WiFi config saved");
  }
}

void loadLanConfig() {
  if (!LittleFS.exists("/lan_config.json")) {
    Serial.println("No LAN config, using defaults");
    return;
  }

  File file = LittleFS.open("/lan_config.json", "r");
  if (!file) return;

  String content = file.readString();
  file.close();

  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, content) == DeserializationError::Ok) {
    String ip = doc["ipAddress"].as<String>();
    String gw = doc["gateway"].as<String>();
    String sn = doc["subnet"].as<String>();
    if (ip.length() > 0) eth_ip.fromString(ip);
    if (gw.length() > 0) eth_gateway.fromString(gw);
    if (sn.length() > 0) eth_subnet.fromString(sn);
  }
}

void saveLanConfig(const String& ipAddress,
                   const String& gateway,
                   const String& subnet) {
  DynamicJsonDocument doc(512);
  doc["ipAddress"] = ipAddress;
  doc["gateway"] = gateway;
  doc["subnet"] = subnet;

  File file = LittleFS.open("/lan_config.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("LAN config saved");
  }

  if (ipAddress.length() > 0) eth_ip.fromString(ipAddress);
  if (gateway.length() > 0) eth_gateway.fromString(gateway);
  if (subnet.length() > 0) eth_subnet.fromString(subnet);
}

static void generateEthernetMAC() {
  uint8_t wifiMac[6];
  WiFi.macAddress(wifiMac);

  eth_mac[0] = wifiMac[0];
  eth_mac[1] = wifiMac[1];
  eth_mac[2] = wifiMac[2];
  eth_mac[3] = wifiMac[3] ^ 0x02;
  eth_mac[4] = wifiMac[4] ^ 0x02;
  eth_mac[5] = wifiMac[5] ^ 0x02;
}

bool g_apEnabled = false;

static bool startAp() {
  Serial.println("Starting WiFi AP mode...");
  // Start in AP+STA to allow initial provisioning.
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Failed to start AP");
    g_apEnabled = false;
    return false;
  }
  g_apEnabled = true;
  currentNetworkMode = NetworkMode::WIFI_AP_MODE;
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP  : ");
  Serial.println(WiFi.softAPIP());
  return true;
}

static bool startEthernet() {
  Serial.println("Trying Ethernet (W5500)...");
  try {
    generateEthernetMAC();

    ESP32_W5500_onEvent();

    ETH.begin(MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO,
              SPI_CLOCK_MHZ, ETH_SPI_HOST, eth_mac);
    ETH.config(eth_ip, eth_gateway, eth_subnet, eth_dns1, eth_dns2);

    unsigned long start = millis();
    while (!ESP32_W5500_isConnected() && millis() - start < 10000) {
      delay(100);
    }

    if (ESP32_W5500_isConnected()) {
      ethernetConnected = true;
      currentNetworkMode = NetworkMode::ETHERNET;
      Serial.println("Ethernet connected");
      Serial.print("IP: ");
      Serial.println(ETH.localIP());
      return true;
    }
  } catch (...) {
    Serial.println("Ethernet init failed");
  }

  ethernetConnected = false;
  return false;
}

static bool startWifiStaFromConfig() {
  if (wifi_ssid.length() == 0) {
    Serial.println("WiFi STA: no SSID configured");
    return false;
  }

  Serial.print("WiFi STA: connecting to ");
  Serial.println(wifi_ssid);

  // Do not disable AP while connecting STA.
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Clean previous STA session before applying new credentials/IP mode.
  WiFi.disconnect(false, true);
  delay(120);

  if (wifi_use_static_ip) {
    Serial.print("WiFi STA: static IP ");
    Serial.println(wifi_static_ip);
    if (!WiFi.config(wifi_static_ip, wifi_gateway, wifi_subnet, wifi_dns1, wifi_dns2)) {
      Serial.println("Failed to configure WiFi static IP");
    }
  } else {
    const IPAddress zeroIp(0, 0, 0, 0);
    if (!WiFi.config(zeroIp, zeroIp, zeroIp, zeroIp, zeroIp)) {
      Serial.println("Failed to set DHCP mode for WiFi STA");
    }
  }

  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    currentNetworkMode = NetworkMode::WIFI_STA_MODE;

    // STA is stable now: disable AP so device no longer broadcasts AP SSID.
    if (g_apEnabled) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      g_apEnabled = false;
      Serial.println("WiFi AP disabled after STA connected");
    }

    Serial.println("WiFi STA connected");
    Serial.println("Web: http://" + WiFi.localIP().toString());
    return true;
  }

  wifiConnected = false;
  if (!ethernetConnected) {
    currentNetworkMode = NetworkMode::WIFI_AP_MODE;
  }
  Serial.println("WiFi STA connect failed");
  return false;
}

void networkInit() {
  loadWiFiConfig();
  loadLanConfig();

  startAp();
  startEthernet();
  startWifiStaFromConfig();
}

IPAddress networkGetCurrentIp() {
  if (currentNetworkMode == NetworkMode::ETHERNET && ethernetConnected) {
    return ETH.localIP();
  }
  if (currentNetworkMode == NetworkMode::WIFI_STA_MODE && wifiConnected) {
    return WiFi.localIP();
  }
  return WiFi.softAPIP();
}

String networkGetModeString() {
  switch (currentNetworkMode) {
    case NetworkMode::ETHERNET:
      return "ethernet";
    case NetworkMode::WIFI_STA_MODE:
      return "wifi_sta";
    case NetworkMode::WIFI_AP_MODE:
    default:
      return "wifi_ap";
  }
}

String networkGetApSsid() {
  return String(AP_SSID);
}
