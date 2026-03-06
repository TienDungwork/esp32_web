#include "speech.h"

#ifdef ENABLE_SPEECH

#include <SPI.h>
#include <SD.h>
#include "Audio.h"

static Audio audio;
static SPIClass SPI_SD(HSPI);
static bool sdReady = false;

void speechInit() {
  // SD card setup
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  SPI_SD.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);

  int retryCount = 0;
  const int maxRetries = 3;

  while (!SD.begin(SD_CS_PIN, SPI_SD)) {
    Serial.printf("[Speech] SD card init failed, retry %d/%d\n", retryCount + 1, maxRetries);
    retryCount++;
    if (retryCount >= maxRetries) {
      Serial.println("[Speech] SD card failed after max retries!");
      return;
    }
    delay(500);
  }

  sdReady = true;
  Serial.println("[Speech] SD card initialized");

  // I2S setup
  audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
  audio.setVolume(21);
  Serial.printf("[Speech] I2S initialized (BCLK=%d LRC=%d DOUT=%d)\n",
                I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
}

void playTracks(String cmd) {
  if (!sdReady) {
    Serial.println("[Speech] SD card not ready, cannot play");
    return;
  }

  cmd.replace(",", "-");

  int start = 0;
  while (true) {
    int sep = cmd.indexOf('-', start);
    String token = (sep == -1) ? cmd.substring(start) : cmd.substring(start, sep);
    token.trim();

    if (token.length() > 0) {
      String filename = "/" + token + ".mp3";
      Serial.print("[Speech] Playing: ");
      Serial.println(filename);

      audio.connecttoFS(SD, filename.c_str());

      // Đợi phát xong
      while (audio.isRunning()) {
        audio.loop();
        delay(1);
      }
    }

    if (sep == -1) break;
    start = sep + 1;
  }
}

#endif  // ENABLE_SPEECH
