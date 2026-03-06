#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define R1_PIN 10
#define G1_PIN 46
#define B1_PIN 3
#define R2_PIN 18
#define G2_PIN 17
#define B2_PIN 16
#define A_PIN 14
#define B_PIN 13
#define C_PIN 12
#define D_PIN 11
#define E_PIN -1
#define LAT_PIN 7
#define OE_PIN 21
#define CLK_PIN 15

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 6

// 0: normal, 1: 90 deg, 2: 180 deg, 3: 270 deg
#define PANEL_ROTATION 2

extern MatrixPanel_I2S_DMA* dma_display;

extern uint16_t LED_COLOR_BLACK;
extern uint16_t LED_COLOR_WHITE;
extern uint16_t LED_COLOR_RED;
extern uint16_t LED_COLOR_GREEN;
extern uint16_t LED_COLOR_BLUE;
extern uint16_t LED_COLOR_YELLOW;
extern uint16_t LED_COLOR_CYAN;

void ledMatrixInit();
void ledMatrixShowCenterText(const String& text, uint16_t color, int y = 8);
void ledMatrixTestBasic();
void ledMatrixShowMultiLine(const String lines[], const float fontSizes[], int lineCount, int boardCount);
int ledMatrixGetMaxBoardCount();
