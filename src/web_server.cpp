#include "web_server.h"

#include <WiFi.h>
#include <WebServer_ESP32_SC_W5500.hpp>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "network.h"
#include "led_matrix.h"
#include "device_control.h"
#include "printer.h"
#include "speech.h"

#include <HTTPUpdate.h>

WebServer server(80);
static const char* LED_CONFIG_FILE = "/led_config.json";

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

  String pathNoSlash = path.substring(1);
  String usePath = LittleFS.exists(path) ? path : (LittleFS.exists(pathNoSlash) ? pathNoSlash : "");
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

static void handleRoot() {
  if (serveStaticFile("/index.html")) return;
  server.send(200, "text/plain", "index.html not found");
}

static void handleNotFound() {
  Serial.print("HTTP 404: method=");
  Serial.print(server.method());
  Serial.print(" uri=");
  Serial.println(server.uri());

  if (serveStaticFile(server.uri())) return;
  server.send(404, "text/plain", "Not found");
}

static void handleNetworkStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  String mode = networkGetModeString();
  IPAddress ip = networkGetCurrentIp();

  String resp = "{";
  resp += "\"current_mode\":\"" + mode + "\",";
  resp += "\"ip\":\"" + ip.toString() + "\",";
  resp += "\"ethernet_connected\":" + String(ethernetConnected ? "true" : "false") + ",";
  resp += "\"wifi_connected\":" + String(wifiConnected ? "true" : "false") + ",";
  resp += "\"ap_ssid\":\"" + networkGetApSsid() + "\"";
  resp += "}";

  server.send(200, "application/json", resp);
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

  // Keep AP alive so client can continue using web UI after sending config.
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
  else if (action == "pause") state = BARRIER_PAUSE;
  else {
    server.send(400, "application/json", "{\"error\":\"Invalid action. Use: open, close, pause\"}");
    return;
  }

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

  trafficLightControl(state);

  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["state"] = stateStr;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

// ══════════════════════════════════════════════════
//  Speech Play
// ══════════════════════════════════════════════════
static void handleSpeechPlay() {
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

  String tracks = doc["tracks"].as<String>();
  if (tracks.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No tracks specified\"}");
    return;
  }

  // Gửi response trước vì playTracks() là blocking
  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["tracks"] = tracks;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);

  playTracks(tracks);
}

// ══════════════════════════════════════════════════
//  Printer
// ══════════════════════════════════════════════════
static void handlePrinterPrint() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data provided\"}");
    return;
  }

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String type = doc["type"] | "text";
  String text = doc["text"].as<String>();
  bool bold = doc["bold"] | false;
  int align = doc["align"] | 0;

  printer_reset();
  printer_set_bold(bold);
  printer_set_alignment((uint8_t)align);

  if (type == "text") {
    printer_println(text.c_str());
  } else if (type == "barcode") {
    printer_set_barcode_height(80);
    printer_set_barcode_width(2);
    printer_print_barcode(text.c_str(), 73);
  } else if (type == "qrcode") {
    uint8_t zoom = doc["zoom"] | 6;
    printer_print_qrcode(text.c_str(), 0, 0, zoom);
  }

  printer_new_line(3);

  DynamicJsonDocument resp(128);
  resp["success"] = true;
  resp["type"] = type;
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
    default:            return "pause";
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

  DynamicJsonDocument doc(512);
  doc["barrier"] = barrierStateToStr(getBarrierState());
  doc["traffic_light"] = trafficStateToStr(getTrafficLightState());
  doc["beam"] = readBeam();
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
  server.on("/", HTTP_GET, handleRoot);

  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/index.html", LittleFS, "/index.html");

  // ── Network ──
  server.on("/api/network/status", HTTP_GET, handleNetworkStatus);
  server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);
  server.on("/api/lan", HTTP_GET, handleLanGet);
  server.on("/api/lan", HTTP_POST, handleLanPost);

  // ── LED ──
  server.on("/api/led/config", HTTP_OPTIONS, handleLedConfigOptions);
  server.on("/api/led/config", HTTP_GET, handleLedGetConfig);
  server.on("/api/led/config", HTTP_POST, handleLedConfig);

  // ── Device Control ──
  server.on("/api/barrier/control",       HTTP_OPTIONS, handleApiOptions);
  server.on("/api/barrier/control",       HTTP_POST,    handleBarrierControl);
  server.on("/api/traffic-light/control", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/traffic-light/control", HTTP_POST,    handleTrafficLight);
  server.on("/api/speech/play",           HTTP_OPTIONS, handleApiOptions);
  server.on("/api/speech/play",           HTTP_POST,    handleSpeechPlay);
  server.on("/api/printer/print",         HTTP_OPTIONS, handleApiOptions);
  server.on("/api/printer/print",         HTTP_POST,    handlePrinterPrint);
  server.on("/api/ota/update",            HTTP_OPTIONS, handleApiOptions);
  server.on("/api/ota/update",            HTTP_POST,    handleOtaUpdate);
  server.on("/api/device/status",         HTTP_GET,     handleDeviceStatus);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("WebServer started");
  Serial.println("API endpoints:");
  Serial.println("  GET  /api/led/config");
  Serial.println("  POST /api/led/config");
  Serial.println("  POST /api/barrier/control");
  Serial.println("  POST /api/traffic-light/control");
  Serial.println("  POST /api/speech/play");
  Serial.println("  POST /api/printer/print");
  Serial.println("  POST /api/ota/update");
  Serial.println("  GET  /api/device/status");
}

void webServerHandleClient() {
  server.handleClient();
}
