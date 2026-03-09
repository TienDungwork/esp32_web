#include "web_server.h"

#include <WiFi.h>
#include <WebServer_ESP32_SC_W5500.hpp>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "network.h"
#include "led_matrix.h"
#include "device_control.h"

#include <HTTPUpdate.h>

WebServer server(80);
static const char* LED_CONFIG_FILE = "/led_config.json";
static const char* APP_SERVER_CONFIG_FILE = "/app_server_config.json";

static WiFiClient appServerClient;
static String appServerIp = "";
static uint16_t appServerPort = 0;
static uint8_t appServerIdType = 1;
static bool appServerAutoReconnect = true;
static bool appServerEnabled = false;
static unsigned long appServerLastConnectAttemptMs = 0;
static unsigned long appServerLastConnectedAtMs = 0;
static unsigned long appServerLastRxAtMs = 0;
static String appServerLastError = "";

enum class DeviceControlMode : uint8_t {
  NONE = 0,
  BARRIER = 1,
  TRAFFIC = 2
};

static DeviceControlMode activeDeviceControlMode = DeviceControlMode::NONE;

static const char* controlModeToStr(DeviceControlMode mode) {
  switch (mode) {
    case DeviceControlMode::BARRIER: return "barrier";
    case DeviceControlMode::TRAFFIC: return "traffic";
    case DeviceControlMode::NONE:
    default: return "none";
  }
}

static void switchDeviceControlMode(DeviceControlMode mode) {
  if (mode == activeDeviceControlMode) return;

  if (mode == DeviceControlMode::BARRIER) {
    trafficLightDeactivate();
  } else if (mode == DeviceControlMode::TRAFFIC) {
    barrierDeactivate();
  } else {
    barrierDeactivate();
    trafficLightDeactivate();
  }

  activeDeviceControlMode = mode;
  Serial.print("[DevCtrl] Active mode switched to: ");
  Serial.println(controlModeToStr(activeDeviceControlMode));
}

#ifndef APP_FIRMWARE_VERSION
  #define APP_FIRMWARE_VERSION "dev"
#endif

static String getFirmwareBuildStamp() {
  return String(__DATE__) + " " + String(__TIME__);
}

static bool hasNetworkUplink() {
  return ethernetConnected || wifiConnected;
}

static void loadAppServerConfig() {
  appServerIp = "";
  appServerPort = 0;
  appServerIdType = 1;
  appServerAutoReconnect = true;
  appServerEnabled = false;

  if (!LittleFS.exists(APP_SERVER_CONFIG_FILE)) return;

  File file = LittleFS.open(APP_SERVER_CONFIG_FILE, "r");
  if (!file) return;

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err != DeserializationError::Ok) return;

  appServerIp = doc["ip"].as<String>();
  appServerPort = static_cast<uint16_t>(doc["port"] | 0);
  appServerIdType = static_cast<uint8_t>(doc["id_type"] | 1);
  appServerAutoReconnect = doc["auto_reconnect"] | true;
  appServerEnabled = doc["enabled"] | false;

  if (appServerIdType < 1 || appServerIdType > 255) appServerIdType = 1;
}

static void saveAppServerConfig() {
  DynamicJsonDocument doc(512);
  doc["ip"] = appServerIp;
  doc["port"] = appServerPort;
  doc["id_type"] = appServerIdType;
  doc["auto_reconnect"] = appServerAutoReconnect;
  doc["enabled"] = appServerEnabled;

  File file = LittleFS.open(APP_SERVER_CONFIG_FILE, "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

static bool appServerConnectNow() {
  appServerLastConnectAttemptMs = millis();

  if (!hasNetworkUplink()) {
    appServerLastError = "No WiFi STA/Ethernet uplink";
    return false;
  }

  appServerIp.trim();
  if (appServerIp.length() == 0 || appServerPort == 0) {
    appServerLastError = "IP/Port is empty or invalid";
    return false;
  }

  if (appServerClient.connected()) {
    appServerLastError = "";
    return true;
  }

  appServerClient.stop();
  appServerClient.setTimeout(1500);

  bool ok = appServerClient.connect(appServerIp.c_str(), appServerPort);
  if (!ok) {
    appServerLastError = "Connect failed";
    return false;
  }

  appServerLastConnectedAtMs = millis();
  appServerLastError = "";
  return true;
}

static void appServerDisconnect() {
  if (appServerClient.connected()) {
    appServerClient.stop();
  }
}

static void appServerLoop() {
  if (appServerClient.connected()) {
    while (appServerClient.available() > 0) {
      appServerClient.read();
      appServerLastRxAtMs = millis();
    }
    return;
  }

  if (!appServerEnabled || !appServerAutoReconnect) return;

  const unsigned long retryIntervalMs = 5000;
  if (millis() - appServerLastConnectAttemptMs >= retryIntervalMs) {
    appServerConnectNow();
  }
}

static void handleAppServerConfigGet() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(512);
  doc["ip"] = appServerIp;
  doc["port"] = appServerPort;
  doc["id_type"] = appServerIdType;
  doc["auto_reconnect"] = appServerAutoReconnect;
  doc["enabled"] = appServerEnabled;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleAppServerConfigPost() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, body);
  if (err != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String ip = doc["ip"].as<String>();
  int port = doc["port"] | 0;
  int idType = doc["id_type"] | 1;
  bool autoReconnect = doc["auto_reconnect"] | true;
  bool enabled = doc["enabled"] | false;

  ip.trim();
  if (ip.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"IP server is required\"}");
    return;
  }
  if (port < 1 || port > 65535) {
    server.send(400, "application/json", "{\"error\":\"Port must be 1..65535\"}");
    return;
  }
  if (idType < 1 || idType > 255) {
    server.send(400, "application/json", "{\"error\":\"id_type must be 1..255\"}");
    return;
  }

  appServerIp = ip;
  appServerPort = static_cast<uint16_t>(port);
  appServerIdType = static_cast<uint8_t>(idType);
  appServerAutoReconnect = autoReconnect;
  appServerEnabled = enabled;
  saveAppServerConfig();

  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["message"] = "App server config saved";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

static void handleAppServerConnect() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // Optional payload to save and connect in one action.
  String body = server.arg("plain");
  if (body.length() > 0) {
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, body);
    if (err == DeserializationError::Ok) {
      String ip = doc["ip"].as<String>();
      int port = doc["port"] | 0;
      int idType = doc["id_type"] | 1;
      bool autoReconnect = doc["auto_reconnect"] | true;

      ip.trim();
      if (ip.length() > 0) appServerIp = ip;
      if (port >= 1 && port <= 65535) appServerPort = static_cast<uint16_t>(port);
      if (idType >= 1 && idType <= 255) appServerIdType = static_cast<uint8_t>(idType);
      appServerAutoReconnect = autoReconnect;
    }
  }

  appServerEnabled = true;
  saveAppServerConfig();
  bool ok = appServerConnectNow();

  DynamicJsonDocument resp(384);
  resp["success"] = ok;
  resp["connected"] = appServerClient.connected();
  resp["message"] = ok ? "Connected to app server" : "Connect failed";
  resp["error"] = appServerLastError;
  String out;
  serializeJson(resp, out);
  server.send(ok ? 200 : 500, "application/json", out);
}

static void handleAppServerDisconnect() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  appServerEnabled = false;
  appServerDisconnect();
  saveAppServerConfig();

  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["connected"] = false;
  resp["message"] = "Disconnected";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

static void handleAppServerStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(512);
  doc["connected"] = appServerClient.connected();
  doc["ip"] = appServerIp;
  doc["port"] = appServerPort;
  doc["id_type"] = appServerIdType;
  doc["auto_reconnect"] = appServerAutoReconnect;
  doc["enabled"] = appServerEnabled;
  doc["last_error"] = appServerLastError;
  doc["last_connect_attempt_ms"] = appServerLastConnectAttemptMs;
  doc["last_connected_at_ms"] = appServerLastConnectedAtMs;
  doc["last_rx_at_ms"] = appServerLastRxAtMs;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void sendDefaultLedConfig() {
  DynamicJsonDocument doc(1024);
  doc["boardCount"] = 1;
  doc["lineSpacing"] = -1;

  JsonArray lines = doc.createNestedArray("lines");
  for (int i = 1; i <= 5; i++) {
    JsonObject line = lines.createNestedObject();
    line["line"] = i;
    line["fixed"] = "";
    line["text"] = "";
    line["fontSize"] = 1.6f;
    line["color"] = "#00ff00";
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static String getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

static bool serveStaticFile(const String& rawPath) {
  String path = rawPath;
  if (path.length() == 0 || path == "/") path = "/index.html";
  if (!path.startsWith("/")) path = "/" + path;

  String usePath = LittleFS.exists(path) ? path : "";
  if (usePath.length() == 0) return false;

  File f = LittleFS.open(usePath, "r");
  if (!f) return false;

  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.streamFile(f, getContentType(usePath));
  f.close();
  return true;
}

static bool shouldTryStaticFallback(HTTPMethod method, const String& uri) {
  if (method != HTTP_GET) return false;
  if (uri.startsWith("/api/")) return false;
  if (uri == "/" || uri == "/index.html") return true;
  return uri.indexOf('.') >= 0;
}

static void handleRoot() {
  if (serveStaticFile("/index.html")) return;
  server.send(200, "text/plain", "index.html not found");
}

static void handleNotFound() {
  Serial.print("HTTP 404: method=");
  Serial.print(server.method());
  Serial.print(" uri=");
  Serial.println(server.uri());

  if (shouldTryStaticFallback(server.method(), server.uri()) && serveStaticFile(server.uri())) return;
  server.send(404, "text/plain", "Not found");
}

static void handleWifiConfigGet() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(512);
  doc["ssid"] = wifi_ssid;
  doc["password"] = wifi_password;
  doc["use_static_ip"] = wifi_use_static_ip;
  doc["static_ip"] = wifi_static_ip.toString();
  doc["gateway"] = wifi_gateway.toString();
  doc["subnet"] = wifi_subnet.toString();
  doc["dns1"] = wifi_dns1.toString();
  doc["dns2"] = wifi_dns2.toString();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleNetworkStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(512);
  doc["current_mode"] = networkGetModeString();
  doc["ip"] = networkGetCurrentIp().toString();
  doc["ethernet_connected"] = ethernetConnected;
  doc["wifi_connected"] = wifiConnected;
  doc["ap_ssid"] = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
                   ? networkGetApSsid()
                   : "";

  // Firmware metadata for showing current version before OTA.
  doc["firmware_version"] = APP_FIRMWARE_VERSION;
  doc["firmware_build"] = getFirmwareBuildStamp();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleWifiScan() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Connection", "close");

  Serial.println("WiFi scan requested");

  wifi_mode_t currentMode = WiFi.getMode();
  if (currentMode == WIFI_OFF) {
    Serial.println("WiFi scan: switching to STA mode");
    WiFi.mode(WIFI_STA);
    delay(120);
  } else if (currentMode == WIFI_AP) {
    Serial.println("WiFi scan: switching AP -> AP+STA");
    WiFi.mode(WIFI_AP_STA);
    delay(120);
  }

  unsigned long scanStart = millis();
  int n = WiFi.scanNetworks(false, true, false, 100);
  unsigned long scanDuration = millis() - scanStart;
  Serial.println("Scan completed in " + String(scanDuration) + "ms");

  if (n == WIFI_SCAN_FAILED) {
    server.send(500, "application/json", "{\"error\":\"WiFi scan failed\",\"networks\":[]}");
    WiFi.scanDelete();
    return;
  }

  if (n <= 0) {
    server.send(200, "application/json", "{\"networks\":[]}");
    WiFi.scanDelete();
    return;
  }

  String response = "{\"networks\":[";
  int maxNetworks = min(n, 6);
  for (int i = 0; i < maxNetworks; i++) {
    if (i > 0) response += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "");
    ssid.replace("\\", "");
    ssid.replace("\n", "");
    ssid.replace("\r", "");

    response += "{";
    response += "\"ssid\":\"" + ssid + "\",";
    response += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    response += "\"encrypted\":" + String((WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false");
    response += "}";
  }
  response += "]}";

  server.send(200, "application/json", response);
  WiFi.scanDelete();
}

static void handleWifiConnect() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();
  bool useStatic = doc["use_static_ip"] | false;
  String staticIP = doc["static_ip"].as<String>();
  String gw = doc["gateway"].as<String>();
  String sn = doc["subnet"].as<String>();
  String dns1 = doc["dns1"].as<String>();
  String dns2 = doc["dns2"].as<String>();

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID required\"}");
    return;
  }

  saveWiFiConfig(ssid, password, useStatic, staticIP, gw, sn, dns1, dns2);

  wifi_ssid = ssid;
  wifi_password = password;
  wifi_use_static_ip = useStatic;
  if (useStatic && staticIP.length() > 0) {
    wifi_static_ip.fromString(staticIP);
    if (gw.length() > 0) wifi_gateway.fromString(gw);
    if (sn.length() > 0) wifi_subnet.fromString(sn);
    if (dns1.length() > 0) wifi_dns1.fromString(dns1);
    if (dns2.length() > 0) wifi_dns2.fromString(dns2);
  }

  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["message"] = "WiFi config saved. Attempting connection...";
  resp["status"] = "connecting";

  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);

  delay(100);
  Serial.println("Attempting WiFi connection to: " + ssid);

  // Start from AP+STA for provisioning connect flow.
  WiFi.mode(WIFI_AP_STA);
  if (wifi_use_static_ip) {
    if (!WiFi.config(wifi_static_ip, wifi_gateway, wifi_subnet, wifi_dns1, wifi_dns2)) {
      Serial.println("Failed to configure static IP");
    }
  }
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 15000) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(500);
  }

  if (connected) {
    wifiConnected = true;
    currentNetworkMode = NetworkMode::WIFI_STA_MODE;

    // Connection succeeded: stop AP and keep STA only.
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    Serial.println("WiFi connected successfully!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed, keeping AP active");
  }
}

static void handleLanGet() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  DynamicJsonDocument doc(256);
  doc["ipAddress"] = eth_ip.toString();
  doc["gateway"] = eth_gateway.toString();
  doc["subnet"] = eth_subnet.toString();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleLanPost() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String ip = doc["ipAddress"].as<String>();
  String gw = doc["gateway"].as<String>();
  String sn = doc["subnet"].as<String>();

  if (ip.length() == 0 || gw.length() == 0 || sn.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Missing IP/gateway/subnet\"}");
    return;
  }

  saveLanConfig(ip, gw, sn);

  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["message"] = "LAN config saved. Please reboot device if IP changed.";

  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

static void handleLedConfigOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

static void handleLedGetConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  if (!LittleFS.exists(LED_CONFIG_FILE)) {
    sendDefaultLedConfig();
    return;
  }

  File file = LittleFS.open(LED_CONFIG_FILE, "r");
  if (!file) {
    server.send(500, "application/json", "{\"error\":\"Cannot open LED config file\"}");
    return;
  }

  String content = file.readString();
  file.close();

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, content) != DeserializationError::Ok) {
    Serial.println("LED config file invalid JSON, fallback to defaults");
    sendDefaultLedConfig();
    return;
  }

  if (!doc.containsKey("boardCount")) doc["boardCount"] = 1;
  if (!doc.containsKey("lineSpacing")) doc["lineSpacing"] = -1;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleLedConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0 && server.args() > 0) {
    body = server.arg(0);
  }

  if (body.length() == 0) {
    Serial.println("LED config: empty request body");
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, body);
  if (err != DeserializationError::Ok) {
    Serial.println("LED config JSON parse failed: " + String(err.c_str()));
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int boardCount = doc["boardCount"] | 1;
  int lineSpacing = doc["lineSpacing"] | -1;

  String lines[5] = {"", "", "", "", ""};
  float fontSizes[5] = {1.6f, 1.6f, 1.6f, 1.6f, 1.6f};
  uint16_t colors[5] = {
    LED_COLOR_GREEN,
    LED_COLOR_GREEN,
    LED_COLOR_GREEN,
    LED_COLOR_GREEN,
    LED_COLOR_GREEN
  };

  auto normalizeFontSize = [](float value) {
    if (value <= 0.0f) return 1.6f;
    if (value > 8.0f) return value / 10.0f;
    return value;
  };

  JsonArray inputLines = doc["lines"].as<JsonArray>();
  for (JsonVariant item : inputLines) {
    int lineNumber = item["line"] | 0;
    if (lineNumber < 1 || lineNumber > 5) continue;

    String fixed = item["fixed"] | "";
    String text = item["text"] | "";
    lines[lineNumber - 1] = fixed + text;
    float receivedSize = item["fontSize"] | 1.6f;
    fontSizes[lineNumber - 1] = normalizeFontSize(receivedSize);
    String colorHex = item["color"] | "";
    colors[lineNumber - 1] = ledMatrixParseColor(colorHex, LED_COLOR_GREEN);
  }

  ledMatrixShowMultiLine(lines, fontSizes, colors, 5, boardCount, lineSpacing);

  File file = LittleFS.open(LED_CONFIG_FILE, "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
  } else {
    Serial.println("LED config save failed: cannot open file");
  }

  DynamicJsonDocument resp(256);
  resp["success"] = true;
  resp["message"] = "LED config applied";
  resp["maxBoardCount"] = ledMatrixGetMaxBoardCount();

  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

// ══════════════════════════════════════════════════
//  Barrier Control
// ══════════════════════════════════════════════════
static void handleBarrierControl() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String action = doc["action"].as<String>();
  BarrierState state;

  if (action == "open")       state = BARRIER_OPEN;
  else if (action == "close") state = BARRIER_CLOSE;
  else if (action == "stop" || action == "pause") state = BARRIER_PAUSE;
  else {
    server.send(400, "application/json", "{\"error\":\"Invalid action. Use: open, close, stop\"}");
    return;
  }

  switchDeviceControlMode(DeviceControlMode::BARRIER);
  barrierControl(state);

  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["state"] = action;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

// ══════════════════════════════════════════════════
//  Traffic Light Control
// ══════════════════════════════════════════════════
static void handleTrafficLight() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String stateStr = doc["state"].as<String>();
  TrafficLightState state;

  if (stateStr == "off")           state = TRAFFIC_OFF;
  else if (stateStr == "green")    state = TRAFFIC_GREEN;
  else if (stateStr == "red")      state = TRAFFIC_RED;
  else if (stateStr == "yellow")   state = TRAFFIC_YELLOW;
  else if (stateStr == "red_flash") state = TRAFFIC_RED_FLASH;
  else {
    server.send(400, "application/json", "{\"error\":\"Invalid state. Use: off, green, red, yellow, red_flash\"}");
    return;
  }

  switchDeviceControlMode(DeviceControlMode::TRAFFIC);
  trafficLightControl(state);

  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["state"] = stateStr;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

static void handleControlModePost() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String modeStr = doc["mode"].as<String>();
  modeStr.toLowerCase();

  if (modeStr == "barrier") {
    switchDeviceControlMode(DeviceControlMode::BARRIER);
  } else if (modeStr == "traffic") {
    switchDeviceControlMode(DeviceControlMode::TRAFFIC);
  } else if (modeStr == "none") {
    switchDeviceControlMode(DeviceControlMode::NONE);
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid mode. Use: barrier, traffic, none\"}");
    return;
  }

  DynamicJsonDocument resp(192);
  resp["success"] = true;
  resp["active_control_mode"] = controlModeToStr(activeDeviceControlMode);
  resp["message"] = "Control mode switched";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

// ══════════════════════════════════════════════════
//  OTA Update
// ══════════════════════════════════════════════════
static void handleOtaUpdate() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String url = doc["url"].as<String>();
  if (url.length() == 0 || !url.startsWith("http")) {
    server.send(400, "application/json", "{\"error\":\"Invalid URL\"}");
    return;
  }

  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["message"] = "Starting OTA update...";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
  delay(200);

  Serial.println("[OTA] Starting update from: " + url);

  WiFiClient client;
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] FAILED (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] No updates available");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] Update success, restarting...");
      ESP.restart();
      break;
  }
}

// ══════════════════════════════════════════════════
//  Device Status
// ══════════════════════════════════════════════════
static const char* barrierStateToStr(BarrierState s) {
  switch (s) {
    case BARRIER_OPEN:  return "open";
    case BARRIER_CLOSE: return "close";
    case BARRIER_PAUSE:
    default:            return "stop";
  }
}

static const char* trafficStateToStr(TrafficLightState s) {
  switch (s) {
    case TRAFFIC_GREEN:     return "green";
    case TRAFFIC_RED:       return "red";
    case TRAFFIC_YELLOW:    return "yellow";
    case TRAFFIC_RED_FLASH: return "red_flash";
    case TRAFFIC_OFF:
    default:                return "off";
  }
}

static void handleDeviceStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(1024);
  doc["barrier"] = barrierStateToStr(getBarrierState());
  doc["traffic_light"] = trafficStateToStr(getTrafficLightState());
  doc["active_control_mode"] = controlModeToStr(activeDeviceControlMode);
  // Keep legacy key for existing UI compatibility
  doc["beam"] = readBeamPwm1();

  doc["beam_pwm1"] = readBeamPwm1();
  doc["beam_pwm2"] = readBeamPwm2();
  doc["beam_a0"] = readBeamA0();

  doc["btn_open"] = readButtonOpen();
  doc["btn_close"] = readButtonClose();
  doc["btn_stop"] = readButtonStop();
  doc["button_open"] = readButtonOpen();
  doc["button_close"] = readButtonClose();
  doc["button_stop"] = readButtonStop();
  doc["btn_open_pressed"] = (readButtonOpen() == LOW);
  doc["btn_close_pressed"] = (readButtonClose() == LOW);
  doc["btn_stop_pressed"] = (readButtonStop() == LOW);
  doc["uptime_ms"] = millis();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// CORS preflight handler
static void handleApiOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void webServerInit() {
  loadAppServerConfig();

  server.on("/", HTTP_GET, handleRoot);

  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/index.html", LittleFS, "/index.html");

  // ── Network ──
  server.on("/api/network/status", HTTP_GET, handleNetworkStatus);
  server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/api/wifi/config", HTTP_GET, handleWifiConfigGet);
  server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);
  server.on("/api/lan", HTTP_GET, handleLanGet);
  server.on("/api/lan", HTTP_POST, handleLanPost);

  // ── App Server TCP ──
  server.on("/api/app-server/config", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/app-server/config", HTTP_GET, handleAppServerConfigGet);
  server.on("/api/app-server/config", HTTP_POST, handleAppServerConfigPost);
  server.on("/api/app-server/connect", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/app-server/connect", HTTP_POST, handleAppServerConnect);
  server.on("/api/app-server/disconnect", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/app-server/disconnect", HTTP_POST, handleAppServerDisconnect);
  server.on("/api/app-server/status", HTTP_GET, handleAppServerStatus);

  // Ignore stray probing requests (e.g. browser extensions / tooling)
  server.on("/chat", HTTP_ANY, []() {
    server.send(204, "text/plain", "");
  });

  // ── LED ──
  server.on("/api/led/config", HTTP_OPTIONS, handleLedConfigOptions);
  server.on("/api/led/config", HTTP_GET, handleLedGetConfig);
  server.on("/api/led/config", HTTP_POST, handleLedConfig);

  // ── Device Control ──
  server.on("/api/barrier/control",       HTTP_OPTIONS, handleApiOptions);
  server.on("/api/barrier/control",       HTTP_POST,    handleBarrierControl);
  server.on("/api/traffic-light/control", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/traffic-light/control", HTTP_POST,    handleTrafficLight);
  server.on("/api/device/control-mode",   HTTP_OPTIONS, handleApiOptions);
  server.on("/api/device/control-mode",   HTTP_POST,    handleControlModePost);
  server.on("/api/ota/update",            HTTP_OPTIONS, handleApiOptions);
  server.on("/api/ota/update",            HTTP_POST,    handleOtaUpdate);
  server.on("/api/device/status",         HTTP_GET,     handleDeviceStatus);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("WebServer started");
  Serial.println("API endpoints:");
  Serial.println("  GET  /api/wifi/config");
  Serial.println("  GET  /api/app-server/config");
  Serial.println("  POST /api/app-server/config");
  Serial.println("  POST /api/app-server/connect");
  Serial.println("  POST /api/app-server/disconnect");
  Serial.println("  GET  /api/app-server/status");
  Serial.println("  GET  /api/led/config");
  Serial.println("  POST /api/led/config");
  Serial.println("  POST /api/barrier/control");
  Serial.println("  POST /api/traffic-light/control");
  Serial.println("  POST /api/device/control-mode");
  Serial.println("  POST /api/ota/update");
  Serial.println("  GET  /api/device/status");
}

void webServerHandleClient() {
  server.handleClient();
  appServerLoop();
}
