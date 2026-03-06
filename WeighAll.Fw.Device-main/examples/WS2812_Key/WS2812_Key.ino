/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-04-18 14:05:20
 * @LastEditTime: 2025-04-18 14:48:09
 * @License: GPL 3.0
 */

#include <Arduino.h>
#include "pin_config.h"
#include "FastLED.h"

CRGB leds_1;
CRGB leds_2;
CRGB leds_3;
CRGB leds_4;

CRGB *led[] =
    {
        &leds_1,
        &leds_2,
        &leds_3,
        &leds_4,
};

CRGB colour[] =
    {
        CRGB::Black,
        CRGB::Red,
        CRGB::Green,
        CRGB::Blue,
};

int8_t colour_count = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(KEY_A, INPUT_PULLUP);
    pinMode(KEY_B, INPUT_PULLUP);

    FastLED.addLeds<WS2812B, WS2812_DATA_1, GRB>(led[0], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_2, GRB>(led[1], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_3, GRB>(led[2], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_4, GRB>(led[3], 1);

    FastLED.setBrightness(50);

    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = colour[colour_count];
    }
    FastLED.show();
}

void loop()
{
    // for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    // {
    //     *led[i] = CRGB::Red;
    // }
    // FastLED.show();
    // delay(1000);
    // for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    // {
    //     *led[i] = CRGB::Green;
    // }
    // FastLED.show();
    // delay(1000);
    // for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    // {
    //     *led[i] = CRGB::Blue;
    // }
    // FastLED.show();
    // delay(1000);
    // for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    // {
    //     *led[i] = CRGB::Black;
    // }
    // FastLED.show();
    // delay(1000);

    if (digitalRead(KEY_A) == LOW)
    {
        Serial.println("trigger: KEY_A");
        delay(300);
        colour_count--;
        if (colour_count < 0)
        {
            colour_count = 0;
        }

        for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
        {
            *led[i] = colour[colour_count];
        }
        FastLED.show();
    }

    if (digitalRead(KEY_B) == LOW)
    {
        Serial.println("trigger: KEY_B");
        delay(300);
        colour_count++;
        if (colour_count > 3)
        {
            colour_count = 3;
        }

        for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
        {
            *led[i] = colour[colour_count];
        }
        FastLED.show();
    }
}
