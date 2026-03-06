/*
 * @Description: Ethernet HTTP test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:49:57
 * @License: GPL 3.0
 */
#include <SPI.h>
#include <Ethernet.h>
#include "pin_config.h"
#include <iostream>
#include <memory>
#include "utility/w5100.h"
#include <HTTPClient.h>
#include <ETH.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
// The MAC address must be an even number first !
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

const char *fileDownloadUrl = "http://music.163.com/song/media/outer/url?id=26122999.mp3";

static size_t CycleTime = 0;

IPAddress ip(192, 168, 1, 177);

// initialize the library instance:
EthernetClient client;

// IPAddress server(204, 79, 197, 200); // Bing的IP地址
IPAddress server(220, 181, 38, 150); // 百度的IP地址

/**
 * @brief W5500 reset pin delay
 * @param resetPin W5500 reset pin
 * @return
 * @Date 2023-07-19 11:31:23
 */
void EthernetReset(const uint8_t resetPin)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(250);
    digitalWrite(resetPin, LOW);
    delay(50);
    digitalWrite(resetPin, HIGH);
    delay(350);
}

void setup()
{
    Serial.begin(115200);

    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS);
    SPI.setFrequency(80000000);
    EthernetReset(W5500_RST);
    Ethernet.init(W5500_CS);

    W5100.init();
    delay(1000); // give the Ethernet shield a second to initialize

    switch (Ethernet.hardwareStatus())
    {
    case EthernetNoHardware:
        // no point in carrying on, so do nothing forevermore:
        while (true)
        {
            Serial.println("Ethernet No Hardware");
            delay(1000);
        }
        break;
    case EthernetW5100:
        Serial.println("Ethernet W5100 Discovery !");
        break;
    case EthernetW5200:
        Serial.println("Ethernet W5200 Discovery !");
        break;
    case EthernetW5500:
        Serial.println("Ethernet W5500 Discovery !");
        break;
    }

    switch (Ethernet.linkStatus())
    {
    case Unknown:
        // no point in carrying on, so do nothing forevermore:
        while (true)
        {
            Serial.print("Link status: ");
            Serial.println("Unknown");
            Serial.println("Hardware error !");
            delay(1000);
        }
        break;
    case LinkON:
        Serial.print("Link status: ");
        Serial.println("ON");
        break;
    case LinkOFF:
        Serial.print("Link status: ");
        Serial.println("OFF");
        Serial.println("The network cable is not connected !");

        while (Ethernet.linkStatus() == LinkOFF)
        {
            Serial.println("Please insert the network cable and try again !");
            delay(1000);
        }
        if (Ethernet.linkStatus() == LinkON)
        {
            Serial.print("Link status: ");
            Serial.println("ON");
        }
        else
        {
            // no point in carrying on, so do nothing forevermore:
            while (true)
            {
                Serial.println("Hardware error !");
                delay(1000);
            }
        }
        break;
    }

    // attempt a DHCP connection:
    Serial.println("Attempting to get an IP address using DHCP:");
    if (!Ethernet.begin(mac))
    {
        // if DHCP fails, start with a hard-coded address:
        Serial.println("failed to get an IP address using DHCP, trying manually");
        Ethernet.begin(mac, ip);
    }
    Serial.print("My address:");
    Serial.println(Ethernet.localIP());
}

void loop()
{
    size_t temp_start_time;
    size_t temp_end_time;
    size_t temp_website_content_length;

    // 连接到服务器
    if (client.connect(server, 80))
    {
        // 发送HTTP请求
        client.println("GET / HTTP/1.1");
        client.println("Host: www.baidu.com");
        client.println("Connection: close");
        client.println();

        // 读取响应头
        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            if (line == "\r")
            {
                break; // 响应头结束
            }
            if (line.startsWith("Content-Length:"))
            {
                temp_website_content_length = line.substring(16).toInt();
                Serial.printf("Website Content-Length: %d bytes", temp_website_content_length);
            }
        }

        // 读取并打印响应内容
        while (client.available())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }

        client.stop();
    }

    // 连接到服务器
    if (client.connect(server, 80))
    {
        temp_start_time = millis();

        // 发送HTTP请求
        client.println("GET / HTTP/1.1");
        client.println("Host: www.baidu.com");
        client.println("Connection: close");
        client.println();

        // 读取响应头
        while (client.connected())
        {
            if (client.available())
            {
                client.read();

                // char c = client.read();
                // Serial.print(c);
            }
        }
        temp_end_time = millis();
        client.stop();

        size_t temp_response_time = temp_end_time - temp_start_time;

        Serial.print("Response time: ");
        Serial.print(temp_response_time);
        Serial.println(" ms");

        float temp_speed = 0;
        if (temp_response_time > 0)
        {
            temp_speed = ((float)temp_website_content_length / 1000.0) / ((float)temp_response_time / 1000.0); // Kb/秒
        }

        Serial.printf("Throughput: %f Kb/s", temp_speed);
    }
    else
    {
        Serial.println("Connection failed");
    }

    delay(5000); // 每隔5秒发送一次请求
}
