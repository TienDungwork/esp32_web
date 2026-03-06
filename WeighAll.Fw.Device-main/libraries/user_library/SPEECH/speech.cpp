#include "speech.h"

#if DEVICE_SPEAK
// Global objects
Audio audio;
SPIClass SPI_SD(HSPI);   // SPI cho SD card

/* void SD_Card_Init(void){
  // SD setup
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI_SD.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, SPI_SD)) {
    PrintDebugLn("Error accessing microSD card!");
    //while (true);
  }

} */

void SD_Card_Init(void) {
    // SD setup
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI_SD.begin(SD_SCK, SD_MISO, SD_MOSI);

    int retryCount = 0;
    const int maxRetries = 5;      // số lần thử lại
    const int retryDelayMs = 1000; // thời gian giữa các lần thử (ms)

    while (!SD.begin(SD_CS, SPI_SD)) {
        PrintDebugf("[SD] Error accessing microSD card! Retry %d/%d\n", retryCount + 1, maxRetries);
        retryCount++;
        if (retryCount >= maxRetries) {
            PrintDebugLn("[SD] Failed after maximum retries!");
            return; // hoặc gán cờ lỗi để xử lý ngoài
        }
        delay(retryDelayMs);
    }

    PrintDebugLn("[SD] Card initialized successfully!");
}


void I2S_Init(void){
  // I2S setup
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);
}

// Phát các file MP3 theo lệnh dạng "1-sad-hello-2-3" hoặc "1,2,3"
void playTracks(String cmd) {
  cmd.replace(",", "-");  // Hỗ trợ cả dấu , và -

  int start = 0;
  while (true) {
    int sep = cmd.indexOf('-', start);
    String token = (sep == -1) ? cmd.substring(start) : cmd.substring(start, sep);
    token.trim();

    if (token.length() > 0) {
      String filename = "/" + token + ".mp3";
      PrintDebug("> Playing: "); PrintDebugLn(filename);

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
#endif