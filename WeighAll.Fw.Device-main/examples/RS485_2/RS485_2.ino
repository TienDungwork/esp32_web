/*
 * @Description: RS485_2 test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:53:57
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "pin_config.h"
#include <HardwareSerial.h>

static size_t CycleTime = 0;
static uint8_t Uart_Buf[105] = {0};

static uint32_t RS485_1_Count = 0;
static uint32_t RS485_2_Count = 0;

static uint8_t Uart_Data[] = {

    0x0A, // 设备标头识别

    // 动态数据计数
    0B00000000, // 数据高2位
    0B00000000, // 数据高1位
    0B00000000, // 数据低2位
    0B00000000, // 数据低1位

    // 100个数据包
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
    0xAA,
};

bool Uart_Check_Dynamic_Data(uint8_t *uart_data, uint32_t check_data)
{
    uint32_t temp;

    temp = (uint32_t)uart_data[1] << 24 | (uint32_t)uart_data[2] << 16 |
           (uint32_t)uart_data[3] << 8 | (uint32_t)uart_data[4];

    if (temp == check_data)
    {
        return true;
    }

    return false;
}

bool Uart_Check_Static_Data(uint8_t *uart_data)
{
    if (memcmp(&uart_data[5], &Uart_Data[5], 100) == 0)
    {
        return true;
    }
    return false;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    Serial1.begin(115200, SERIAL_8N1, RS485_RX_1, RS485_TX_1);
    Serial2.begin(115200, SERIAL_8N1, RS485_RX_2, RS485_TX_2);

    Serial.println("RS485 is preparing");
    delay(1000);
    Serial.println("5");
    delay(1000);
    Serial.println("4");
    delay(1000);
    Serial.println("3");
    delay(1000);
    Serial.println("2");
    delay(1000);
    Serial.println("1");
    delay(1000);
    Serial.println("RS485 preparation completed");

    while (Serial1.available() > 0) // 清空缓存
    {
        Serial1.read();
    }
    while (Serial2.available() > 0) // 清空缓存
    {
        Serial2.read();
    }
}

void loop()
{
    if (digitalRead(0) == 0)
    {
        delay(300);
        Serial1.write(Uart_Data, 105);
        Serial2.write(Uart_Data, 105);

        RS485_1_Count++;
        RS485_2_Count++;
    }

    while (Serial1.available() > 0)
    {
        delay(1000); // 接收等待
        Serial1.read(Uart_Buf, sizeof(Uart_Buf));

        if (Uart_Buf[0] == 0x0A)
        {
            if (Uart_Check_Dynamic_Data(Uart_Buf, RS485_1_Count) == false) // 动态数据校验
            {
                while (1)
                {
                    Serial.printf("[RS485_1] Check Dynamic Data Failed\n");
                    Serial.printf("[RS485_1] Check Data: %d\n", RS485_1_Count);
                    Serial.printf("[RS485_1] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                                       (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                    Serial.printf("[RS485_1] Received Buf[1]: %#X\n", Uart_Buf[1]);
                    Serial.printf("[RS485_1] Received Buf[2]: %#X\n", Uart_Buf[2]);
                    Serial.printf("[RS485_1] Received Buf[3]: %#X\n", Uart_Buf[3]);
                    Serial.printf("[RS485_1] Received Buf[4]: %#X\n", Uart_Buf[4]);
                    delay(1000);
                }
            }
            else if (Uart_Check_Static_Data(Uart_Buf) == false) // 静态数据校验
            {
                Serial.printf("[RS485_1] Check Static Data Failed\n");
                for (int i = 0; i < 100; i++)
                {
                    Serial.printf("[RS485_1] Received Buf[%d]: %#X\n", i + 5, Uart_Buf[i + 5]);
                }
                delay(1000);
            }
            else
            {
                delay(500);

                Serial.printf("[RS485_1] Check Data Successful\n");
                Serial.printf("[RS485_1] Check Data: %d\n", RS485_1_Count);
                Serial.printf("[RS485_1] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                                   (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                Serial.printf("[RS485_1] Received Buf[1]: %#X\n", Uart_Buf[1]);
                Serial.printf("[RS485_1] Received Buf[2]: %#X\n", Uart_Buf[2]);
                Serial.printf("[RS485_1] Received Buf[3]: %#X\n", Uart_Buf[3]);
                Serial.printf("[RS485_1] Received Buf[4]: %#X\n", Uart_Buf[4]);

                Serial.printf("[RS485_1] Received Buf[105]: %#X\n", Uart_Buf[104]);

                RS485_1_Count++;

                Uart_Data[1] = RS485_1_Count >> 24;
                Uart_Data[2] = RS485_1_Count >> 16;
                Uart_Data[3] = RS485_1_Count >> 8;
                Uart_Data[4] = RS485_1_Count;

                delay(1000);
                Serial1.write(Uart_Data, 105);

                RS485_1_Count++;
            }
        }
        else
        {
            delay(500);
            Serial.printf("[RS485_1] Check Header Failed\n");
            Serial.printf("[RS485_1] Received Header: %#X\n", Uart_Buf[0]);
            Serial.printf("[RS485_1] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                               (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
            Serial.printf("[RS485_1] Received Buf[1]: %#X\n", Uart_Buf[1]);
            Serial.printf("[RS485_1] Received Buf[2]: %#X\n", Uart_Buf[2]);
            Serial.printf("[RS485_1] Received Buf[3]: %#X\n", Uart_Buf[3]);
            Serial.printf("[RS485_1] Received Buf[4]: %#X\n", Uart_Buf[4]);
        }
    }

    while (Serial2.available() > 0)
    {
        delay(1000); // 接收等待
        Serial2.read(Uart_Buf, sizeof(Uart_Buf));

        if (Uart_Buf[0] == 0x0A)
        {
            if (Uart_Check_Dynamic_Data(Uart_Buf, RS485_2_Count) == false) // 动态数据校验
            {
                while (1)
                {
                    Serial.printf("[RS485_2] Check Dynamic Data Failed\n");
                    Serial.printf("[RS485_2] Check Data: %d\n", RS485_2_Count);
                    Serial.printf("[RS485_2] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                                       (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                    Serial.printf("[RS485_2] Received Buf[1]: %#X\n", Uart_Buf[1]);
                    Serial.printf("[RS485_2] Received Buf[2]: %#X\n", Uart_Buf[2]);
                    Serial.printf("[RS485_2] Received Buf[3]: %#X\n", Uart_Buf[3]);
                    Serial.printf("[RS485_2] Received Buf[4]: %#X\n", Uart_Buf[4]);
                    delay(1000);
                }
            }
            else if (Uart_Check_Static_Data(Uart_Buf) == false) // 静态数据校验
            {
                Serial.printf("[RS485_2] Check Static Data Failed\n");
                for (int i = 0; i < 100; i++)
                {
                    Serial.printf("[RS485_2] Received Buf[%d]: %#X\n", i + 5, Uart_Buf[i + 5]);
                }
                delay(1000);
            }
            else
            {
                delay(500);

                Serial.printf("[RS485_2] Check Data Successful\n");
                Serial.printf("[RS485_2] Check Data: %d\n", RS485_2_Count);
                Serial.printf("[RS485_2] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                                   (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
                Serial.printf("[RS485_2] Received Buf[1]: %#X\n", Uart_Buf[1]);
                Serial.printf("[RS485_2] Received Buf[2]: %#X\n", Uart_Buf[2]);
                Serial.printf("[RS485_2] Received Buf[3]: %#X\n", Uart_Buf[3]);
                Serial.printf("[RS485_2] Received Buf[4]: %#X\n", Uart_Buf[4]);

                Serial.printf("[RS485_2] Received Buf[105]: %#X\n", Uart_Buf[104]);

                RS485_2_Count++;

                Uart_Data[1] = RS485_2_Count >> 24;
                Uart_Data[2] = RS485_2_Count >> 16;
                Uart_Data[3] = RS485_2_Count >> 8;
                Uart_Data[4] = RS485_2_Count;

                delay(1000);
                Serial2.write(Uart_Data, 105);

                RS485_2_Count++;
            }
        }
        else
        {
            delay(500);
            Serial.printf("[RS485_2] Check Header Failed\n");
            Serial.printf("[RS485_2] Received Header: %#X\n", Uart_Buf[0]);
            Serial.printf("[RS485_2] Received Data: %d\n", (uint32_t)Uart_Buf[1] << 24 | (uint32_t)Uart_Buf[2] << 16 |
                                                               (uint32_t)Uart_Buf[3] << 8 | (uint32_t)Uart_Buf[4]);
            Serial.printf("[RS485_2] Received Buf[1]: %#X\n", Uart_Buf[1]);
            Serial.printf("[RS485_2] Received Buf[2]: %#X\n", Uart_Buf[2]);
            Serial.printf("[RS485_2] Received Buf[3]: %#X\n", Uart_Buf[3]);
            Serial.printf("[RS485_2] Received Buf[4]: %#X\n", Uart_Buf[4]);
        }
    }


}
