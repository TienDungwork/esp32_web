#include <Arduino.h>
#include "driver_config.h"
// Định nghĩa các chân
#define BARRIER_OPEN_PIN  1  // Chân mở barie
#define BARRIER_CLOSE_PIN 2  // Chân đóng barie
#define BEAM_PIN          6  // Chân lưới hồng ngoại
#define TRAFFIC_RED_PIN   7  // Chân đèn đỏ
#define TRAFFIC_YELLOW_PIN 15 // Chân đèn vàng
#define TRAFFIC_GREEN_PIN 16 // Chân đèn xanh

// Hàm blink cho một chân
void blinkPin(int pin, const char* pinName) {
  digitalWrite(pin, HIGH); // Bật
  PrintDebug(pinName);
  PrintDebugLn(" ON");
  delay(500); // Đợi 500ms
  digitalWrite(pin, LOW);  // Tắt
  PrintDebug(pinName);
  PrintDebugLn(" OFF");
  delay(500); // Đợi 500ms
}

void setup() {
  Serial.begin(115200); // Khởi tạo Serial để debug
  
  // Cấu hình các chân làm đầu ra
  pinMode(BARRIER_OPEN_PIN, OUTPUT);
  pinMode(BARRIER_CLOSE_PIN, OUTPUT);
  pinMode(BEAM_PIN, OUTPUT); // Tạm thời dùng OUTPUT để test blink
  pinMode(TRAFFIC_RED_PIN, OUTPUT);
  pinMode(TRAFFIC_YELLOW_PIN, OUTPUT);
  pinMode(TRAFFIC_GREEN_PIN, OUTPUT);
  pinMode(4 , OUTPUT);
  pinMode(5, OUTPUT);


  // Đảm bảo các chân tắt ban đầu
  digitalWrite(BARRIER_OPEN_PIN, LOW);
  digitalWrite(BARRIER_CLOSE_PIN, LOW);
  digitalWrite(BEAM_PIN, LOW);
  digitalWrite(TRAFFIC_RED_PIN, LOW);
  digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
  digitalWrite(TRAFFIC_GREEN_PIN, LOW);
  digitalWrite(4 , LOW);
  digitalWrite(5 , LOW);

  //
}

void loop() {
  // Blink lần lượt từng chân
  blinkPin(BARRIER_OPEN_PIN, "Barrier Open");
  blinkPin(BARRIER_CLOSE_PIN, "Barrier Close");
  blinkPin(BEAM_PIN, "Beam");
  blinkPin(TRAFFIC_RED_PIN, "Traffic Red");
  blinkPin(TRAFFIC_YELLOW_PIN, "Traffic Yellow");
  blinkPin(TRAFFIC_GREEN_PIN, "Traffic Green");
  //blinkPin(4 , "Tx");
  //blinkPin(5 , "Rx");
  
  PrintDebugLn("----- End of cycle -----");
  delay(1000); // Đợi 1 giây trước khi lặp lại
}