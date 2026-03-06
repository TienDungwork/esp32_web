/*
 * @Description: RS485 test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:53:18
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include "pin_config.h"

static uint32_t temp = 0;
static size_t CycleTime = 0;

HardwareSerial SerialPort1(1); // use UART2
HardwareSerial SerialPort2(2); // use UART2
char DATA[100] = {0};

void setup()
{
    Serial.begin(9600);
    Serial.println("Ciallo");

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    SerialPort1.begin(9600, SERIAL_8N1, RS485_RX_1, RS485_TX_1);
    SerialPort2.begin(9600, SERIAL_8N1, RS485_RX_2, RS485_TX_2);

    Serial.println("RS485 is preparing");
    delay(1000);
    Serial.println("3");
    delay(1000);
    Serial.println("2");
    delay(1000);
    Serial.println("1");
    delay(1000);
    Serial.println("RS485 preparation completed");

    while (SerialPort1.available() > 0) // 清空缓存
    {
        SerialPort1.read();
    }
    while (SerialPort2.available() > 0) // 清空缓存
    {
        SerialPort2.read();
    }
}

void loop()
{
    if (millis() > CycleTime)
    {
        temp++;
        SerialPort1.printf("%d", temp);
        delay(10); // 这里的延时必须要有
        SerialPort2.printf("%d", temp);
        CycleTime = millis() + 3000;
    }

    while (SerialPort1.available() > 0)
    {
        memset(DATA, '\0', sizeof(DATA));
        SerialPort1.read(DATA, sizeof(DATA));

        if (strlen(DATA) != 0)
        {
            Serial.printf("rx: %s\n", DATA);
        }
    }

    while (SerialPort2.available() > 0)
    {
        memset(DATA, '\0', sizeof(DATA));
        SerialPort2.read(DATA, sizeof(DATA));

        if (strlen(DATA) != 0)
        {
            Serial.printf("RS485_2 receive data: %s\n", DATA);
        }
    }
}
