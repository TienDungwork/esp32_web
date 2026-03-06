#include "gpio.h"


// Biến toàn cục để theo dõi trạng thái trước đó
static int last_beam_state = LOW; // Giả sử HIGH là trạng thái không ngắt

// Khởi tạo các chân GPIO
void gpio_init() {
  // Cấu hình các chân điều khiển barie làm OUTPUT
  pinMode(BARRIER_OPEN_PIN, OUTPUT);
  pinMode(BARRIER_CLOSE_PIN, OUTPUT);
  
  // Cấu hình các chân đèn giao thông làm OUTPUT
  pinMode(TRAFFIC_RED_PIN, OUTPUT);
  pinMode(TRAFFIC_YELLOW_PIN, OUTPUT);
  pinMode(TRAFFIC_GREEN_PIN, OUTPUT);
  
  // Cấu hình chân lưới hồng ngoại làm INPUT
  //pinMode(BEAM_PIN, INPUT);
  pinMode(BEAM_PIN, INPUT_PULLUP);

  // Đặt trạng thái ban đầu
  digitalWrite(BARRIER_OPEN_PIN, LOW);
  digitalWrite(BARRIER_CLOSE_PIN, LOW);
  digitalWrite(TRAFFIC_RED_PIN, LOW);
  digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
  digitalWrite(TRAFFIC_GREEN_PIN, LOW);
  
}

// Điều khiển barie theo trạng thái
void barrier_control(BarrierState state) {
  switch (state) {
    case BARRIER_PAUSE:
      digitalWrite(BARRIER_OPEN_PIN, LOW);
      digitalWrite(BARRIER_CLOSE_PIN, LOW);
      break;
    case BARRIER_OPEN:
      digitalWrite(BARRIER_OPEN_PIN, HIGH);
      digitalWrite(BARRIER_CLOSE_PIN, LOW);
      break;
    case BARRIER_CLOSE:
      digitalWrite(BARRIER_OPEN_PIN, LOW);
      digitalWrite(BARRIER_CLOSE_PIN, HIGH);
      break;
  }
}

// Điều khiển đèn giao thông theo trạng thái
void traffic_light_control(TrafficLightState state) {
  static unsigned long lastToggle = 0;
  static bool redState = LOW;
  const unsigned long flashInterval = 500; // Khoảng thời gian nhấp nháy (ms)

  switch (state) {
    case TRAFFIC_OFF:
      digitalWrite(TRAFFIC_RED_PIN, LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN, LOW);
      break;
    case TRAFFIC_GREEN:
      digitalWrite(TRAFFIC_RED_PIN, LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN, HIGH);
      break;
    case TRAFFIC_RED:
      digitalWrite(TRAFFIC_RED_PIN, HIGH);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN, LOW);
      break;
    case TRAFFIC_YELLOW:
      digitalWrite(TRAFFIC_RED_PIN, LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, HIGH);
      digitalWrite(TRAFFIC_GREEN_PIN, LOW);
      break;
    case TRAFFIC_RED_FLASH:
      if (millis() - lastToggle >= flashInterval) {
        redState = !redState;
        digitalWrite(TRAFFIC_RED_PIN, redState);
        digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
        digitalWrite(TRAFFIC_GREEN_PIN, LOW);
        lastToggle = millis();
      }
      break;
  }
}

// Đọc trạng thái lưới hồng ngoại (String)
int Handle_Read_Beam() {
    int current_state = digitalRead(BEAM_PIN);
    
    // In trạng thái 0 hoặc 1 để kiểm tra
    //PrintDebug("Beam state: ");
    //PrintDebugLn(current_state); // In 0 hoặc 1

    // Chỉ xử lý khi trạng thái thay đổi
    if (current_state != last_beam_state) {
        String data = String(current_state); // Tạo chuỗi "0" hoặc "1"
        
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


// Đọc trạng thái lưới hồng ngoại (Int)
/* int Handle_Read_Beam() {
    int current_state = digitalRead(BEAM_PIN);

    if (current_state != last_beam_state && current_state == HIGH) {
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%d", current_state);  // -> "1"

        if (!EventQueue_Push(EVENT_IR_BEAM_BROKEN, buffer)) {
            PrintDebugLn("[IR Beam] Failed to push EVENT_IR_BEAM_BROKEN to queue (queue full)");
        } else {
            PrintDebug("[IR Beam] Pushed EVENT_IR_BEAM_BROKEN with data: ");
            PrintDebugLn(buffer);
        }
    }

    last_beam_state = current_state;
    return current_state;
} */
