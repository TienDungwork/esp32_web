#include "wifi_ap.h"

#if ENABLE_WIFI_AP

#define AP_SSID "T-Connect"
#define AP_PASS ""
#define DNS_PORT 53

DNSServer dnsServer;
WebServer webServer(80);

// HTML hỗ trợ khách hàng
const char HTML_PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Device Offline</title>
  <style>
    body {
      font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
      background-color: #f8f8f8;
      color: #333;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      margin: 0;
      padding: 20px;
      text-align: center;
    }

    .card {
      background-color: white;
      padding: 30px;
      border-radius: 12px;
      box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
      max-width: 400px;
      width: 100%;
    }

    h1 {
      color: #d9534f;
      font-size: 24px;
      margin-bottom: 20px;
    }

    p {
      font-size: 16px;
      margin: 10px 0;
    }

    .contact {
      font-weight: bold;
      color: #007bff;
    }

    @media (max-width: 500px) {
      .card {
        padding: 20px;
      }
      h1 {
        font-size: 20px;
      }
      p {
        font-size: 14px;
      }
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP-Speak is currently offline</h1>
    <p>The device is unable to connect to the server.</p>
    <p>Please contact technical support:</p>
    <p class="contact">+84 868804969</p>
  </div>
</body>
</html>
)rawliteral";


bool captiveStarted = false;

void startCaptivePortal() {
  if (captiveStarted) return;
  captiveStarted = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  IPAddress myIP = WiFi.softAPIP();
  PrintDebugLn("[Captive] IP AP: " + myIP.toString());

  dnsServer.start(DNS_PORT, "*", myIP);

  webServer.onNotFound([]() {
    webServer.send(200, "text/html", HTML_PAGE);
  });

  webServer.begin();
  Serial.println("[Captive] Portal started!");
}

void stopCaptivePortal() {
  if (!captiveStarted) return;

  PrintDebugLn("[Captive] Stopping captive portal...");

  WiFi.softAPdisconnect(true);  // Tắt WiFi Access Point
  dnsServer.stop();             // Dừng DNS giả
  webServer.stop();            // Dừng HTTP server

  captiveStarted = false;
}

void handleCaptiveTasks() {
  if (captiveStarted) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
}



#endif  //ENABLE_WIFI_AP