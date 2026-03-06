// Web ESP PIO - Cấu hình mạng + LED
// - Phát AP
// - WiFi STA (từ cấu hình web)
// - Ethernet W5500 (LAN)
// - Giao diện web riêng (HTML/CSS trong LittleFS)

#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <LittleFS.h>

#include "led_matrix.h"
#include "network.h"
#include "web_server.h"

static void logLittleFsFiles() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS root open failed");
    return;
  }

  Serial.println("LittleFS files:");
  File file = root.openNextFile();
  while (file) {
    Serial.print(" - ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    file = root.openNextFile();
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Booting Web ESP PIO (AP / WiFi / ETH + LED)...");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
  } else {
    Serial.println("LittleFS mounted.");
    logLittleFsFiles();

    if (LittleFS.exists("/index.html") || LittleFS.exists("index.html")) {
      Serial.println("LittleFS check: index.html found");
    } else {
      Serial.println("LittleFS check: index.html NOT found");
    }
    if (LittleFS.exists("/style.css") || LittleFS.exists("style.css")) {
      Serial.println("LittleFS check: style.css found");
    } else {
      Serial.println("LittleFS check: style.css NOT found - chay 'pio run -t uploadfs' de upload file");
    }
    if (!LittleFS.exists("/index.html") && !LittleFS.exists("index.html")) {
      Serial.println("Hint: reflash partitions.bin + littlefs.bin with matching partition table.");
    }
  }

  ledMatrixInit();
  ledMatrixShowCenterText("BOOT", LED_COLOR_CYAN, 8);

  networkInit();

  webServerInit();

  ledMatrixShowCenterText("READY", LED_COLOR_GREEN, 8);
}

void loop() {
  webServerHandleClient();
}
