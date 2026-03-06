/*
 * @Description: Relay test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:53:00
 * @License: GPL 3.0
 */

#include <Arduino.h>
#include "pin_config.h"

static uint8_t i = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, i);
}

void loop()
{
    digitalWrite(RELAY_1, i);

    i = !i;
    delay(3000);
}