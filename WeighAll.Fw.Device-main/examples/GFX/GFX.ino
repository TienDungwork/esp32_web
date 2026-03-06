/*
 * @Description: GFX
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2024-07-08 18:38:18
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"

Arduino_DataBus *bus = new Arduino_HWSPI(
    SCREEN_DC /* DC */, SCREEN_CS /* CS */, SCREEN_SCLK /* SCK */,
    SCREEN_MOSI /* MOSI */, SCREEN_MISO /* MISO */);

Arduino_GFX *gfx = new Arduino_ST7796(
    bus, SCREEN_RST /* RST */, 0 /* rotation */, true /* IPS */,
    SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */,
    49 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);

void setup(void)
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    ledcAttachPin(SCREEN_BL, 1);
    ledcSetup(1, 2000, 8);
    ledcWrite(1, 255);

    gfx->begin();
    gfx->fillScreen(WHITE);
}

void loop()
{
}