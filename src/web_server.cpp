#include "web_server.h"

#include <WiFi.h>
#include <WebServer_ESP32_SC_W5500.hpp>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

#include "network.h"
#include "led_matrix.h"
#include "device_control.h"

#include <HTTPUpdate.h>

WebServer server(80);
static const char* LED_CONFIG_FILE = "/led_config.json";
static const char* APP_SERVER_CONFIG_FILE = "/app_server_config.json";
static const char* APP_PACKET_DELIM = "<EOF>";

static WiFiClient appServerClient;
static String appServerIp = "";
static uint16_t appServerPort = 0;
static uint8_t appServerIdType = 1;
static int appServerSelectedDeviceCode = 1;
static bool appServerAutoReconnect = true;
static bool appServerEnabled = false;
static unsigned long appServerLastConnectAttemptMs = 0;
static unsigned long appServerLastConnectedAtMs = 0;
static unsigned long appServerLastRxAtMs = 0;
static unsigned long appServerLastTxAtMs = 0;
static bool appServerWaitingForResponse = false;
static bool appServerConnectSent = false;  // chỉ true sau khi user bấm gửi gói yêu cầu kết nối
static String appServerLastError = "";
static String appServerRxBuffer = "";
static bool appServerConnectionConfirmed = false;
static int appServerLastResponseStatus = -1;
static int appServerLastResponseCode = 0;
static bool ntpConfigured = false;

static void logAppServer(const String& msg) {
  Serial.println("[AppServer] " + msg);
}

static String escapeJsonString(String input) {
  input.replace("\\", "\\\\");
  input.replace("\"", "\\\"");
  return input;
}

static bool sendConnectPacketForDeviceType(int deviceType) {
  // Packet connect tối giản theo yêu cầu:
  // {"Code":1,"Message":"{\"DeviceType\":<int>}","DeviceType":<int>,"CreatedAt":"0001-01-01T00:00:00","IndexInPacket":0}<EOF>
  if (!appServerClient.connected()) return false;

  DynamicJsonDocument info(64);
  info["DeviceType"] = deviceType;
  String infoJson;
  serializeJson(info, infoJson);

  DynamicJsonDocument doc(256);
  doc["Code"] = 1;
  doc["Message"] = infoJson;           // ArduinoJson tự escape đúng 1 lần
  doc["DeviceType"] = deviceType;
  doc["CreatedAt"] = "0001-01-01T00:00:00";
  doc["IndexInPacket"] = 0;

  String out;
  serializeJson(doc, out);
  out += APP_PACKET_DELIM;

  size_t written = appServerClient.print(out);
  appServerLastTxAtMs = millis();
  appServerWaitingForResponse = true;

  if (written != out.length()) {
    appServerLastError = "Write packet failed";
    logAppServer("TX connect failed (wrote " + String(written) + "/" + String(out.length()) + " bytes), DeviceType=" + String(deviceType));
    return false;
  }

  logAppServer("TX connect ok DeviceType=" + String(deviceType) + " bytes=" + String(out.length()));
  return true;
}

// Gửi nhiều gói Code=1 cho danh sách thiết bị, giống luồng tconnect cũ.
static void sendAllDeviceConnectStatesLikeTconnect() {
  int selected = appServerSelectedDeviceCode;

  // Nhóm thiết bị vào
  static const int enterList[] = {1, 3, 4, 5, 6, 7, 10, 11, 102};
  // Nhóm thiết bị ra
  static const int exitList[]  = {51, 53, 54, 55, 56, 57, 102};
  // Nhóm loa
  static const int speakerList[] = {102};

  const int* list = enterList;
  int count = (int)(sizeof(enterList) / sizeof(enterList[0]));
  if (selected >= 51 && selected <= 61) {
    list = exitList;
    count = (int)(sizeof(exitList) / sizeof(exitList[0]));
  } else if (selected == 102) {
    list = speakerList;
    count = (int)(sizeof(speakerList) / sizeof(speakerList[0]));
  }

  logAppServer("Send connect list count=" + String(count) +
               " selected_device_code=" + String(selected));
  for (int i = 0; i < count; i++) {
    sendConnectPacketForDeviceType(list[i]);
    delay(30);
  }
}

static void sendConnectionRequestPacket() {
  if (!appServerClient.connected()) {
    appServerLastError = "Socket is not connected";
    appServerConnectionConfirmed = false;
    logAppServer("Cannot send connect request: socket not connected");
    return;
  }

  // Gửi nhiều gói Code=1 cho nhóm thiết bị tương ứng với mã đang chọn.
  sendAllDeviceConnectStatesLikeTconnect();
}

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
  // Đừng tin wifiConnected/ethernetConnected vì có thể bị stale khi link rớt.
  bool staUp = (WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != INADDR_NONE) && (WiFi.localIP().toString() != "0.0.0.0");
  bool ethUp = ethernetConnected; // nếu cần chặt hơn có thể thay bằng ESP32_W5500_isConnected()
  return staUp || ethUp;
}

static bool isSupportedDeviceCode(int code) {
  switch (code) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11:
    case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59: case 60: case 61:
    case 101: case 102: case 103:
      return true;
    default:
      return false;
  }
}

static void applySelectedDeviceCode(int code) {
  if (!isSupportedDeviceCode(code)) return;
  appServerSelectedDeviceCode = code;
  appServerIdType = static_cast<uint8_t>(code);
}

static String nowIso8601IfValid() {
  time_t now = time(nullptr);
  if (now < 1700000000) {
    return "";
  }

  struct tm timeInfo;
  gmtime_r(&now, &timeInfo);
  char buf[40];
  // Thêm millisecond để match format server hay dùng: 2026-02-27T10:15:30.123Z
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  uint16_t ms = (uint16_t)(millis() % 1000);
  char out[48];
  snprintf(out, sizeof(out), "%s.%03uZ", buf, (unsigned)ms);
  return String(out);
}

static bool ensureTimeSynced(unsigned long maxWaitMs) {
  // Chỉ sync khi có uplink thật sự, tránh chờ vô ích khi AP-only.
  if (!hasNetworkUplink()) return false;

  if (!ntpConfigured) {
    // GMT+0, DST=0. Server thường dùng ISO8601 Zulu.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
    ntpConfigured = true;
    logAppServer("NTP configured, waiting time sync...");
  }

  unsigned long start = millis();
  while (millis() - start < maxWaitMs) {
    if (time(nullptr) >= 1700000000) {
      logAppServer("NTP time synced");
      return true;
    }
    delay(50);
  }
  logAppServer("NTP sync timeout (" + String(maxWaitMs) + "ms)");
  return false;
}

static bool writePacketToAppServer(int code,
                                   const String& message,
                                   bool includeStatus,
                                   int status,
                                   bool includeIndex,
                                   int indexInPacket) {
  if (!appServerClient.connected()) {
    appServerLastError = "Socket is not connected";
    return false;
  }

  DynamicJsonDocument doc(1024);
  doc["Code"] = code;
  doc["Message"] = message;
  doc["DeviceType"] = static_cast<int>(appServerIdType);

  if (includeStatus) {
    doc["Status"] = status;
  }

  String createdAt = nowIso8601IfValid();
  if (createdAt.length() > 0) {
    doc["CreatedAt"] = createdAt;
  }

  if (includeIndex) {
    doc["IndexInPacket"] = indexInPacket;
  }

  String out;
  serializeJson(doc, out);
  out += APP_PACKET_DELIM;

  size_t written = appServerClient.print(out);
  appServerLastTxAtMs = millis();
  appServerWaitingForResponse = true;
  if (written != out.length()) {
    appServerLastError = "Write packet failed";
    logAppServer("TX failed (wrote " + String(written) + "/" + String(out.length()) + " bytes)");
    return false;
  }

  logAppServer("TX Code=" + String(code) + " bytes=" + String(out.length()));
  return true;
}

static bool applyLedConfigDoc(DynamicJsonDocument& srcDoc, String* errorMessage = nullptr) {
  int boardCount = srcDoc["boardCount"] | 1;
  int lineSpacing = srcDoc["lineSpacing"] | -1;

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

  JsonArray inputLines = srcDoc["lines"];
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

  if (boardCount < 1) boardCount = 1;
  if (boardCount > ledMatrixGetMaxBoardCount()) boardCount = ledMatrixGetMaxBoardCount();

  ledMatrixShowMultiLine(lines, fontSizes, colors, 5, boardCount, lineSpacing);

  if (errorMessage) {
    *errorMessage = "";
  }
  return true;
}

static bool handleLedScreenControlMessage(const String& message, String* errorMessage = nullptr) {
  DynamicJsonDocument ledDoc(4096);
  DeserializationError err = deserializeJson(ledDoc, message);
  if (err != DeserializationError::Ok) {
    // Fallback: message is plain text, show at center line.
    ledMatrixShowCenterText(message, LED_COLOR_GREEN, 8);
    if (errorMessage) {
      *errorMessage = "";
    }
    return true;
  }

  if (ledDoc.containsKey("lines")) {
    return applyLedConfigDoc(ledDoc, errorMessage);
  }

  String text = ledDoc["text"] | ledDoc["message"] | "";
  if (text.length() == 0) {
    if (errorMessage) {
      *errorMessage = "Message JSON does not contain LED content";
    }
    return false;
  }

  String colorHex = ledDoc["color"] | "#00ff00";
  uint16_t color = ledMatrixParseColor(colorHex, LED_COLOR_GREEN);
  int boardCount = ledDoc["boardCount"] | 8;
  if (boardCount < 1) boardCount = 1;
  if (boardCount > ledMatrixGetMaxBoardCount()) boardCount = ledMatrixGetMaxBoardCount();
  ledMatrixShowCenterText(text, color, boardCount);

  if (errorMessage) {
    *errorMessage = "";
  }
  return true;
}

static bool handleAppServerPacket(const String& packetJson, String* errorMessage = nullptr) {
  DynamicJsonDocument packetDoc(6144);
  DeserializationError err = deserializeJson(packetDoc, packetJson);
  if (err != DeserializationError::Ok) {
    if (errorMessage) {
      *errorMessage = "Invalid packet JSON";
    }
    return false;
  }

  int code = packetDoc["Code"] | 0;
  if (code > 0) {
    appServerLastResponseCode = code;
  }

  if (packetDoc.containsKey("Status")) {
    appServerLastResponseStatus = packetDoc["Status"] | 0;
    appServerConnectionConfirmed = (appServerLastResponseStatus == 1);
  }

  String message = packetDoc["Message"].as<String>();

  // APP -> Controller dispatch theo bảng Code (WeighAll)
  switch (code) {
    case 1:   // Connect ACK/heartbeat payload from app server
    case 2:   // Ping/Pong payload from app server
      return true;

    case 201:  // LED
      return handleLedScreenControlMessage(message, errorMessage);

    case 401: { // Barrier: "1" mở, "2" đóng, "PAUSE" dừng
      BarrierState st;
      // Y hệt WeighAll.Fw.Device: chỉ nhận "1", "2", "PAUSE"
      if (message == "1") st = BARRIER_OPEN;
      else if (message == "2") st = BARRIER_CLOSE;
      else if (message == "PAUSE") st = BARRIER_PAUSE;
      else {
        if (errorMessage) *errorMessage = "Barrier: invalid command";
        return false;
      }
      switchDeviceControlMode(DeviceControlMode::BARRIER);
      barrierControl(st);
      return true;
    }

    case 501: { // Traffic light: 0 off, 1 xanh, 2 vàng, 3 đỏ, 4 nhấp nháy đỏ
      TrafficLightState st;
      // Y hệt WeighAll.Fw.Device: chỉ nhận "0","1","2","3","RED_FLASH"
      if (message == "0") st = TRAFFIC_OFF;
      else if (message == "1") st = TRAFFIC_GREEN;
      else if (message == "2") st = TRAFFIC_YELLOW;
      else if (message == "3") st = TRAFFIC_RED;
      else if (message == "RED_FLASH") st = TRAFFIC_RED_FLASH;
      else {
        if (errorMessage) *errorMessage = "Traffic: invalid command";
        return false;
      }
      switchDeviceControlMode(DeviceControlMode::TRAFFIC);
      trafficLightControl(st);
      return true;
    }

    case 2101: { // OTA: Message = URL
      String url = message;
      url.trim();
      if (url.length() == 0 || !url.startsWith("http")) {
        if (errorMessage) *errorMessage = "OTA: invalid URL";
        return false;
      }
      logAppServer("OTA command: " + url);
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
      return true;
    }

    default:
      break;
  }

  if (errorMessage) {
    *errorMessage = "Unsupported packet code";
  }
  return false;
}

static void loadAppServerConfig() {
  appServerIp = "";
  appServerPort = 0;
  appServerIdType = 1;
  appServerSelectedDeviceCode = 1;
  // Mặc định KHÔNG auto-reconnect để tránh loop vô hạn khi IP/Port sai.
  appServerAutoReconnect = false;
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
  int selectedCode = doc["selected_device_code"] | 0;
  if (isSupportedDeviceCode(selectedCode)) {
    applySelectedDeviceCode(selectedCode);
  } else {
    int idType = doc["id_type"] | 1;

    if (idType < 1 || idType > 255) idType = 1;

    appServerIdType = static_cast<uint8_t>(idType);
    appServerSelectedDeviceCode = idType;
  }
  appServerAutoReconnect = doc["auto_reconnect"] | true;
  appServerEnabled = doc["enabled"] | false;
}

static void saveAppServerConfig() {
  DynamicJsonDocument doc(512);
  doc["ip"] = appServerIp;
  doc["port"] = appServerPort;
  doc["id_type"] = appServerIdType;
  doc["selected_device_code"] = appServerSelectedDeviceCode;
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
    // Cập nhật lại cờ wifiConnected nếu STA đã rớt
    if (wifiConnected && WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
    }
    appServerLastError = "No WiFi STA/Ethernet uplink (STA not connected or IP=0.0.0.0)";
    logAppServer("Connect blocked: no uplink");
    logAppServer("STA.status=" + String((int)WiFi.status()) + " STA.ip=" + WiFi.localIP().toString() +
                 " mode=" + String((int)WiFi.getMode()) + " eth=" + String(ethernetConnected ? "1" : "0"));
    return false;
  }

  appServerIp.trim();
  if (appServerIp.length() == 0 || appServerPort == 0) {
    appServerLastError = "IP/Port is empty or invalid";
    logAppServer("Connect blocked: invalid IP/Port");
    return false;
  }

  if (appServerClient.connected()) {
    appServerLastError = "";
    logAppServer("Already connected");
    return true;
  }

  appServerClient.stop();
  appServerClient.setTimeout(1500);

  logAppServer("Connecting to " + appServerIp + ":" + String(appServerPort) + " id_type=" + String(appServerIdType) +
               " STA.ip=" + WiFi.localIP().toString() + " STA.status=" + String((int)WiFi.status()) +
               " mode=" + String((int)WiFi.getMode()));
  bool ok = appServerClient.connect(appServerIp.c_str(), appServerPort);
  if (!ok) {
    appServerLastError = "Connect failed";
    logAppServer("Connect failed");
    logAppServer("After fail: STA.status=" + String((int)WiFi.status()) + " STA.ip=" + WiFi.localIP().toString());
    return false;
  }

  appServerLastConnectedAtMs = millis();
  appServerRxBuffer = "";
  appServerLastError = "";
  appServerConnectionConfirmed = false;
  appServerLastResponseStatus = -1;
  appServerLastResponseCode = 0;
  appServerWaitingForResponse = false;
  logAppServer("TCP connected. Local=" + appServerClient.localIP().toString() + ":" + String(appServerClient.localPort()));
  return true;
}

static void appServerDisconnect() {
  if (appServerClient.connected()) {
    appServerClient.stop();
  }
  appServerConnectionConfirmed = false;
  appServerWaitingForResponse = false;
  appServerConnectSent = false;
  logAppServer("Disconnected");
}

static void appServerLoop() {
  if (appServerClient.connected()) {
    // Timeout chờ phản hồi sau khi gửi yêu cầu
    if (appServerWaitingForResponse && !appServerConnectionConfirmed) {
      const unsigned long timeoutMs = 5000;
      if (appServerLastTxAtMs > 0 && millis() - appServerLastTxAtMs > timeoutMs) {
        appServerWaitingForResponse = false;
        appServerLastError = "Timeout waiting server response";
        logAppServer("Timeout waiting response (" + String(timeoutMs) + "ms)");
      }
    }

    while (appServerClient.available() > 0) {
      char ch = static_cast<char>(appServerClient.read());
      appServerRxBuffer += ch;
      appServerLastRxAtMs = millis();

      if (appServerRxBuffer.length() > 8192) {
        appServerRxBuffer = "";
        appServerLastError = "RX buffer overflow";
      }
    }

    // Parse stream theo delimiter <EOF> (giống WeighAll)
    int eofIdx = appServerRxBuffer.indexOf(APP_PACKET_DELIM);
    while (eofIdx >= 0) {
      String packet = appServerRxBuffer.substring(0, eofIdx);
      appServerRxBuffer.remove(0, eofIdx + (int)strlen(APP_PACKET_DELIM));
      packet.trim();

      if (packet.length() > 0) {
        logAppServer("RX packet: " + packet);
        String err;
        bool handled = handleAppServerPacket(packet, &err);
        if (!handled && err.length() > 0) {
          appServerLastError = err;
          logAppServer("RX packet handling failed: " + err);
        } else {
          appServerWaitingForResponse = false;
        }
      }

      eofIdx = appServerRxBuffer.indexOf(APP_PACKET_DELIM);
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
  doc["selected_device_code"] = appServerSelectedDeviceCode;
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
  int selectedDeviceCode = doc["selected_device_code"] | 0;
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
  if (isSupportedDeviceCode(selectedDeviceCode)) {
    applySelectedDeviceCode(selectedDeviceCode);
  } else {
    appServerIdType = static_cast<uint8_t>(idType);
    appServerSelectedDeviceCode = idType;
  }
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
  // For connect action, only IP/Port are required.
  String body = server.arg("plain");
  if (body.length() > 0) {
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, body);
    if (err == DeserializationError::Ok) {
      if (doc.containsKey("ip")) {
        String ip = doc["ip"].as<String>();
        ip.trim();
        if (ip.length() > 0) appServerIp = ip;
      }

      if (doc.containsKey("port")) {
        int port = doc["port"] | 0;
        if (port >= 1 && port <= 65535) appServerPort = static_cast<uint16_t>(port);
      }

      if (doc.containsKey("auto_reconnect")) {
        appServerAutoReconnect = doc["auto_reconnect"] | true;
      }
    }
  }

  // Chỉ dùng cờ này để quyết định có auto-reconnect sau này hay không.
  bool wantAutoReconnect = appServerAutoReconnect;

  // Thử kết nối ngay một lần theo yêu cầu của người dùng.
  bool ok = appServerConnectNow();

  // Nếu kết nối thành công và người dùng bật auto-reconnect thì mới bật appServerEnabled
  // để vòng lặp nền tự reconnect khi rớt. Nếu kết nối thất bại hoặc tắt auto-reconnect
  // thì không bật auto-reconnect để tránh loop vô hạn.
  appServerEnabled = ok && wantAutoReconnect;
  saveAppServerConfig();

  DynamicJsonDocument resp(384);
  resp["success"] = ok;
  resp["connected"] = appServerClient.connected();
  resp["message"] = ok ? "Connected to app server (Code=1 auto-sent)" : "Connect failed";
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

static void handleAppServerSendConnectRequest() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String body = server.arg("plain");
  if (body.length() > 0) {
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      int selectedDeviceCode = doc["selected_device_code"] | appServerSelectedDeviceCode;
      if (isSupportedDeviceCode(selectedDeviceCode)) {
        applySelectedDeviceCode(selectedDeviceCode);
      }
    }
  }

  if (!appServerClient.connected()) {
    appServerConnectionConfirmed = false;
    server.send(409, "application/json", "{\"success\":false,\"error\":\"Socket is not connected\"}");
    return;
  }
  bool sentOk = sendConnectionRequestPacket();
  if (sentOk) {
    appServerConnectSent = true;
    appServerConnectionConfirmed = false;
  }

  DynamicJsonDocument resp(256);
  resp["success"] = sentOk;
  resp["message"] = sentOk ? "Connection request packet sent"
                           : "Failed to send connection request packet";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

static void handleAppServerStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");

  DynamicJsonDocument doc(512);
  doc["connected"] = appServerClient.connected();
  doc["connection_confirmed"] = appServerConnectionConfirmed;
  doc["ip"] = appServerIp;
  doc["port"] = appServerPort;
  doc["id_type"] = appServerIdType;
  doc["selected_device_code"] = appServerSelectedDeviceCode;
  doc["auto_reconnect"] = appServerAutoReconnect;
  doc["enabled"] = appServerEnabled;
  doc["last_error"] = appServerLastError;
  doc["last_connect_attempt_ms"] = appServerLastConnectAttemptMs;
  doc["last_connected_at_ms"] = appServerLastConnectedAtMs;
  doc["last_rx_at_ms"] = appServerLastRxAtMs;
  doc["last_response_status"] = appServerLastResponseStatus;
  doc["last_response_code"] = appServerLastResponseCode;

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
  Serial.print("WiFi mode(before)=");
  Serial.print((int)WiFi.getMode());
  Serial.print(" STA.status=");
  Serial.print((int)WiFi.status());
  Serial.print(" STA.ip=");
  Serial.print(WiFi.localIP());
  Serial.print(" AP.ip=");
  Serial.println(WiFi.softAPIP());

  wifi_mode_t currentMode = WiFi.getMode();
  if (currentMode == WIFI_AP) {
    Serial.println("WiFi scan: switching AP -> AP+STA");
    WiFi.mode(WIFI_AP_STA);
    delay(400);
  } else if (currentMode == WIFI_OFF) {
    Serial.println("WiFi scan: switching OFF -> AP+STA");
    WiFi.mode(WIFI_AP_STA);
    delay(400);
  }

  WiFi.scanDelete();
  delay(50);
  WiFi.setSleep(false);
  Serial.println("WiFi scan: setSleep(false), scanDelete()");

  // Trên ESP32, async scan đôi khi trả về -2 (WIFI_SCAN_FAILED) do race condition.
  // Ở đây ưu tiên scan đồng bộ nhưng giới hạn thời gian bằng max_ms_per_chan + retry.
  unsigned long scanStart = millis();
  int n = WIFI_SCAN_FAILED;
  const int maxMsPerChanFast = 250;   // ~13 kênh => khoảng vài giây
  const int maxMsPerChanSlow = 450;   // retry chậm hơn để bắt mạng yếu

  // Attempt 1: active scan (passive=false)
  Serial.println("WiFi scan attempt#1: active, showHidden=true, maxMsPerChan=" + String(maxMsPerChanFast));
  n = WiFi.scanNetworks(false, true, false, maxMsPerChanFast);
  if (n == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed (active), retry passive...");
    delay(120);
    WiFi.scanDelete();
    delay(40);

    // Attempt 2: passive scan (passive=true)
    Serial.println("WiFi scan attempt#2: passive, showHidden=true, maxMsPerChan=" + String(maxMsPerChanSlow));
    n = WiFi.scanNetworks(false, true, true, maxMsPerChanSlow);
  }

  // One more retry active if still failed
  if (n == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed (passive), retry active...");
    delay(180);
    WiFi.scanDelete();
    delay(50);
    Serial.println("WiFi scan attempt#3: active, showHidden=true, maxMsPerChan=" + String(maxMsPerChanSlow));
    n = WiFi.scanNetworks(false, true, false, maxMsPerChanSlow);
  }

  unsigned long scanDuration = millis() - scanStart;
  Serial.println("Scan completed in " + String(scanDuration) + "ms, n=" + String(n));

  if (n == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed (-2). Possible causes: antenna/EMI, WiFi sleep, AP+STA conflicts, weak signal, channel restrictions.");
    WiFi.scanDelete();
    server.send(200, "application/json", "{\"error\":\"Quét WiFi thất bại (-2). Hãy thử lại hoặc reboot thiết bị.\",\"networks\":[]}");
    return;
  }

  if (n <= 0) {
    Serial.println("WiFi scan: no networks found (n<=0). Possible causes: signal too weak, 5GHz-only AP, hidden SSID only, scan blocked by environment.");
    Serial.print("WiFi RSSI (if connected): ");
    Serial.println(WiFi.RSSI());
    server.send(200, "application/json", "{\"networks\":[]}");
    WiFi.scanDelete();
    return;
  }

  DynamicJsonDocument responseDoc(3072);
  JsonArray networks = responseDoc.createNestedArray("networks");

  int maxNetworks = min(n, 12);
  for (int i = 0; i < maxNetworks; i++) {
    String ssid = WiFi.SSID(i);
    Serial.print(" - ");
    Serial.print(i);
    Serial.print(": '");
    Serial.print(ssid);
    Serial.print("' RSSI=");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" encType=");
    Serial.println((int)WiFi.encryptionType(i));
    ssid.replace("\"", "");
    ssid.replace("\\", "");
    ssid.replace("\n", "");
    ssid.replace("\r", "");

    JsonObject item = networks.createNestedObject();
    item["ssid"] = ssid;
    item["rssi"] = WiFi.RSSI(i);
    item["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }

  String response;
  serializeJson(responseDoc, response);

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
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Always reset previous STA state so reconnecting with same/new SSID is clean.
  WiFi.disconnect(false, true);
  delay(150);

  if (wifi_use_static_ip) {
    if (!WiFi.config(wifi_static_ip, wifi_gateway, wifi_subnet, wifi_dns1, wifi_dns2)) {
      Serial.println("Failed to configure static IP");
    }
  } else {
    const IPAddress zeroIp(0, 0, 0, 0);
    if (!WiFi.config(zeroIp, zeroIp, zeroIp, zeroIp, zeroIp)) {
      Serial.println("Failed to set DHCP mode");
    }
  }

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  bool connected = false;
  wl_status_t lastStatus = WL_IDLE_STATUS;
  while (millis() - start < 15000) {
    wl_status_t st = WiFi.status();
    lastStatus = st;
    if (st == WL_CONNECTED) {
      connected = true;
      break;
    }
    if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
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
    wifiConnected = false;
    if (!ethernetConnected) {
      currentNetworkMode = NetworkMode::WIFI_AP_MODE;
    }
    Serial.println("WiFi connection failed, keeping AP active");
    Serial.println("WiFi status code: " + String(static_cast<int>(lastStatus)));
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

  String applyErr;
  if (!applyLedConfigDoc(doc, &applyErr)) {
    server.send(400, "application/json", "{\"error\":\"Invalid LED content\"}");
    return;
  }

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
  server.on("/api/app-server/send-connect-request", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/app-server/send-connect-request", HTTP_POST, handleAppServerSendConnectRequest);
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
  Serial.println("  POST /api/app-server/send-connect-request");
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
