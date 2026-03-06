/* #include <Arduino.h>

#define BEAM_PIN 6 // Chân GPIO 6

// Hàm đọc và xử lý trạng thái beam hồng ngoại
void readBeamState() {
  if (digitalRead(BEAM_PIN) == HIGH) {
    PrintDebugLn("Co vat can!"); // Có vật cản
  } else {
    PrintDebugLn("Khong co vat can."); // Không có vật cản
  }
}

void setup() {
  pinMode(BEAM_PIN, INPUT); // Cấu hình GPIO 6 làm đầu vào
  Serial.begin(115200);     // Khởi tạo Serial để debug
}

void loop() {
  readBeamState(); // Gọi hàm đọc trạng thái
  delay(500);      // Đợi 500ms trước khi đọc lại
}
 */
#include <string>
#include "driver_config.h"

#define SOFTWARE_NAME "Original_Test"
#define SOFTWARE_LASTEDITTIME "202504261030"
#define BOARD_VERSION "V1.0"
#define BEAM_PIN 6 // Sử dụng GPIO 6 cho hồng ngoại

int last_beam_state = LOW; // Khởi tạo trạng thái ban đầu

// Đọc trạng thái lưới hồng ngoại
int Handle_Read_Beam_test() {
    int current_state = digitalRead(BEAM_PIN);
    
    // In trạng thái 0 hoặc 1 để kiểm tra
    PrintDebug("Beam state: ");
    PrintDebugLn(current_state); // In 0 hoặc 1

    // Chỉ xử lý khi trạng thái thay đổi và có vật cản (current_state == HIGH)
    if (current_state != last_beam_state && current_state == HIGH) {
        String data = String(current_state); // "1" khi có vật cản
        
        if (!EventQueue_Push(EVENT_IR_BEAM_BROKEN, data.c_str())) {
            PrintDebugLn("[IR Beam] Failed to push EVENT_IR_BEAM_BROKEN to queue (queue full)");
        } else {
            PrintDebug("[IR Beam] Pushed EVENT_IR_BEAM_BROKEN with data: ");
            PrintDebugLn(data.c_str());
        }
    }
    
    // Cập nhật trạng thái trước đó
    last_beam_state = current_state;
    
    return current_state;
}

void setup() {
    Serial.begin(9600); // Khởi tạo Serial để debug
    pinMode(BEAM_PIN, INPUT); // Cấu hình GPIO 6 làm đầu vào với pull-down nội
}

void loop() {
    Handle_Read_Beam_test(); // Gọi hàm đọc trạng thái hồng ngoại
    delay(100);       // Đợi 100ms trước khi đọc lại
}