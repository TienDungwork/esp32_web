/*
 * @Description: None
 * @version: V1.0.0
 * @Author: None
 * @Date: 2024-07-08 16:34:12
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-07-09 15:25:06
 * @License: GPL 3.0
 */

#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include "pin_config.h"

#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#define ETH_PHY_CS W5500_CS
#define ETH_PHY_IRQ W5500_INT
#define ETH_PHY_RST W5500_RST

static uint8_t i = 0;

static bool eth_connected = false;

void onEvent(arduino_event_id_t event, arduino_event_info_t info)
{
    switch (event)
    {
    case ARDUINO_EVENT_ETH_START:
        Serial.println("ETH Started");
        // set eth hostname here
        ETH.setHostname("esp32-eth0");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif));
        Serial.println(ETH);
        eth_connected = true;
        break;
    case ARDUINO_EVENT_ETH_LOST_IP:
        Serial.println("ETH Lost IP");
        eth_connected = false;
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, i);

    Network.onEvent(onEvent);

    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI);
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
}

void loop()
{
    if (eth_connected)
    {
        
    }
}