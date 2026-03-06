/*
 * @Description: None
 * @version: V1.0.0
 * @Author: None
 * @Date: 2024-07-08 18:01:54
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-07-09 10:08:03
 * @License: GPL 3.0
 */
#include <ETH.h>
#include <SPI.h>
#include "pin_config.h"
#include <HTTPClient.h>

// Set this to 1 to enable dual Ethernet support
#define USE_TWO_ETH_PORTS 0

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#define ETH_PHY_CS W5500_CS
#define ETH_PHY_IRQ W5500_INT
#define ETH_PHY_RST W5500_RST
#endif

// SPI pins
#define ETH_SPI_SCK W5500_SCLK
#define ETH_SPI_MISO W5500_MISO
#define ETH_SPI_MOSI W5500_MOSI

#if USE_TWO_ETH_PORTS
// Second port on shared SPI bus
#ifndef ETH1_PHY_TYPE
#define ETH1_PHY_TYPE ETH_PHY_W5500
#define ETH1_PHY_ADDR 1
#define ETH1_PHY_CS 32
#define ETH1_PHY_IRQ 33
#define ETH1_PHY_RST 18
#endif
ETHClass ETH1(1);
#endif

const char *fileDownloadUrl = "http://music.163.com/song/media/outer/url?id=26122999.mp3";

static size_t CycleTime = 0;

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
#if USE_TWO_ETH_PORTS
        Serial.println(ETH1);
#endif
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

void testClient(const char *host, uint16_t port)
{
    Serial.print("\nconnecting to ");
    Serial.println(host);

    NetworkClient client;
    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return;
    }
    client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (client.connected() && !client.available())
        ;
    while (client.available())
    {
        Serial.write(client.read());
    }

    Serial.println("closing connection\n");
    client.stop();
}

void setup()
{
    Serial.begin(115200);
    Network.onEvent(onEvent);

    SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
#if USE_TWO_ETH_PORTS
    ETH1.begin(ETH1_PHY_TYPE, ETH1_PHY_ADDR, ETH1_PHY_CS, ETH1_PHY_IRQ, ETH1_PHY_RST, SPI);
#endif

    if (eth_connected)
    {
        // testClient(fileDownloadUrl, 80);
        // 初始化HTTP客户端
        HTTPClient http;
        http.begin(fileDownloadUrl);
        // 获取重定向的URL
        const char *headerKeys[] = {"Location"};
        http.collectHeaders(headerKeys, 1);

        // 记录下载开始时间
        size_t startTime = millis();
        // 无用时间
        size_t uselessTime = 0;

        // 发起GET请求
        int httpCode = http.GET();

        while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND)
        {
            String newUrl = http.header("Location");
            Serial.printf("Redirecting to: %s\n", newUrl.c_str());
            http.end(); // 关闭旧的HTTP连接

            // 使用新的URL重新发起GET请求
            http.begin(newUrl);
            httpCode = http.GET();
        }

        if (httpCode == HTTP_CODE_OK)
        {
            //    获取文件大小
            size_t fileSize = http.getSize();
            Serial.printf("Starting file download...\n");
            Serial.printf("file size: %f MB\n", fileSize / 1024.0 / 1024.0);

            // 读取HTTP响应
            NetworkClient *stream = http.getStreamPtr();

            size_t temp_count_s = 0;
            size_t temp_fileSize = fileSize;
            uint8_t *buf_1 = (uint8_t *)heap_caps_malloc(64 * 1024, MALLOC_CAP_SPIRAM);
            // uint8_t buf_1[4096] = {0};
            CycleTime = millis() + 3000; // 开始计时
            while (http.connected() && (fileSize > 0 || fileSize == -1))
            {
                // 获取可用数据的大小
                size_t availableSize = stream->available();
                if (availableSize)
                {
                    temp_fileSize -= stream->read(buf_1, min(availableSize, (size_t)(64 * 1024)));

                    if (millis() > CycleTime)
                    {
                        size_t temp_time_1 = millis();
                        temp_count_s += 3;
                        Serial.printf("Download speed: %f KB/s\n", ((fileSize - temp_fileSize) / 1024.0) / temp_count_s);
                        Serial.printf("Remaining file size: %f MB\n\n", temp_fileSize / 1024.0 / 1024.0);

                        CycleTime = millis() + 3000;
                        size_t temp_time_2 = millis();

                        uselessTime = uselessTime + (temp_time_2 - temp_time_1);
                    }
                }
                // delay(1);

                if (temp_fileSize == 0)
                {
                    break;
                }
            }

            // 关闭HTTP客户端
            http.end();

            // 记录下载结束时间并计算总花费时间
            size_t endTime = millis();
            Serial.printf("Download completed!\n");
            Serial.printf("Total download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
            Serial.printf("Average download speed: %f KB/s\n", (fileSize / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
        }
        else
        {
            Serial.printf("Failed to download\n");
            Serial.printf("Error httpCode: %d \n", httpCode);
        }
    }
}

void loop()
{
}