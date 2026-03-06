/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-06-05 13:01:59
 * @LastEditTime: 2025-04-26 11:06:13
 */
#pragma once

#if DEVICE_HAS_DISPLAY

#define T_Connect_Pro_V1_0
//#define T_Connect_Pro_V1_0_No_Screen

// #define T_Connect_Pro_V1_0_SX1262
#define T_Connect_Pro_V1_0_SX1276

#ifdef T_Connect_Pro_V1_0

#define IIC_SDA 39
#define IIC_SCL 40

// ST7796
#define SCREEN_WIDTH 222
#define SCREEN_HEIGHT 480
#define SCREEN_BL 46
#define SCREEN_MOSI 11
#define SCREEN_MISO 13
#define SCREEN_SCLK 12
#define SCREEN_CS 21
#define SCREEN_DC 41
#define SCREEN_RST -1

// CST226SE
#define TOUCH_SDA IIC_SDA
#define TOUCH_SCL IIC_SCL
#define TOUCH_RST 47
#define TOUCH_INT 3

#elif defined T_Connect_Pro_V1_0_No_Screen

#define WS2812_DATA_1 46
#define WS2812_DATA_2 3
#define WS2812_DATA_3 21
#define WS2812_DATA_4 41

#define KEY_A 39
#define KEY_B 40

#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

// Relay
#define RELAY_1 8

// Ethernet
#define W5500_SCLK 12
#define W5500_MISO 13
#define W5500_MOSI 11
#define W5500_CS 10
#define W5500_RST 48
#define W5500_INT 9


// SX1262
#define SX1262_CS 14
#define SX1262_RST 42
#define SX1262_SCLK 12
#define SX1262_MOSI 11
#define SX1262_MISO 13
#define SX1262_BUSY 38
#define SX1262_INT 45
#define SX1262_DIO1 45

// SX1276
#define SX1276_CS 14
#define SX1276_RST 42
#define SX1276_SCLK 12
#define SX1276_MOSI 11
#define SX1276_MISO 13
#define SX1276_BUSY 38
#define SX1276_INT 45
#define SX1276_DIO1 45

// RS485
#define RS485_TX_1 4
#define RS485_RX_1 5
#define RS485_TX_2 17
#define RS485_RX_2 18

// RS232
#define RS232_TX_1 4
#define RS232_RX_1 5

// CAN
/* #define CAN_TX 6
#define CAN_RX 7 */

// ESPBOOT
#define ESP_BOOT 0

#endif  // DEVICE_HAS_DISPLAY

#if DEVICE_SPEAK
// Chống error
#define SCREEN_WIDTH 222
#define SCREEN_HEIGHT 480

// LED
#define LED             1

// Micro SD
#define SD_CS           39
#define SD_MOSI         20
#define SD_MISO         37
#define SD_SCK          36

// Ethernet: W5500
#define W5500_CS        10
#define W5500_MOSI      11 
#define W5500_MISO      13
#define W5500_SCLK      12
#define W5500_RST       46
#define W5500_INT       9

// Speaker: MAX98357
#define I2S_DOUT        7
#define I2S_BCLK        15
#define I2S_LRC         16

// ESPBOOT
#define ESP_BOOT 0

#endif // DEVICE_SPEAK