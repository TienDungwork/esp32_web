#ifndef GPIO_H
#define GPIO_H

#include "driver_config.h"


// Định nghĩa các chân GPIO
#define BARRIER_OPEN_PIN  6  // Chân mở barie
#define BARRIER_CLOSE_PIN 7  // Chân đóng barie
#define BEAM_PIN          2 // Chân lưới hồng ngoại
#define TRAFFIC_YELLOW_PIN   1  // Chân đèn đỏ
#define TRAFFIC_RED_PIN 15 // Chân đèn vàng
#define TRAFFIC_GREEN_PIN 16 // Chân đèn xanh


// Enum cho trạng thái barie
typedef enum {
  BARRIER_PAUSE = 0, // Tạm dừng
  BARRIER_OPEN  = 1, // Mở
  BARRIER_CLOSE = 2  // Đóng
} BarrierState;

// Enum cho trạng thái đèn giao thông
typedef enum {
  TRAFFIC_OFF      = 0, // Tắt hết
  TRAFFIC_GREEN    = 1, // Đèn xanh
  TRAFFIC_RED      = 2, // Đèn đỏ
  TRAFFIC_YELLOW   = 3, // Đèn vàng
  TRAFFIC_RED_FLASH = 4 // Nhấp nháy cảnh báo đỏ
} TrafficLightState;

// Hàm khởi tạo GPIO
void gpio_init();

// Hàm điều khiển barie
void barrier_control(BarrierState state);

// Hàm điều khiển đèn giao thông
void traffic_light_control(TrafficLightState state);

// Hàm đọc trạng thái lưới hồng ngoại
int Handle_Read_Beam();

#endif