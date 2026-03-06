/*
 * @Description: 出厂测试程序
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-04-21 14:10:56
 * @License: GPL 3.0
 */

#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "Wire.h"
#include "WiFi.h"
#include <HTTPClient.h>
#include "Arduino_DriveBus_Library.h"
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Ethernet.h>
#include "utility/w5100.h"
#include "driver/twai.h"
#include "RadioLib.h"
#include "FastLED.h"

#define SOFTWARE_NAME "Original_Test"

#define SOFTWARE_LASTEDITTIME "202504181336"
#define BOARD_VERSION "V1.0"

#define WIFI_SSID "xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428"
// #define WIFI_SSID "LilyGo-AABB"
// #define WIFI_PASSWORD "xinyuandianzi"

#define WIFI_CONNECT_WAIT_MAX (10000)

#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define NTP_SERVER3 "asia.pool.ntp.org"
#define GMT_OFFSET_SEC 8 * 3600 // Time zone setting function, written as 8 * 3600 in East Eighth Zone (UTC/GMT+8:00)
#define DAY_LIGHT_OFFSET_SEC 0  // Fill in 3600 for daylight saving time, otherwise fill in 0

// Intervall:
#define POLLING_RATE_MS 100
#define TWAI_TRANSMIT_OVERTIME_MS 1000

// 文件下载链接
// const char *fileDownloadUrl = "https://code.visualstudio.com/docs/?dv=win64user";//vscode
// const char *fileDownloadUrl = "https://dldir1.qq.com/qqfile/qq/PCQQ9.7.17/QQ9.7.17.29225.exe"; // 腾讯CDN加速
// const char *fileDownloadUrl = "https://cd001.www.duba.net/duba/install/packages/ever/kinsthomeui_150_15.exe"; // 金山毒霸
const char *fileDownloadUrl = "https://freetyst.nf.migu.cn/public/product9th/product45/2022/05/0716/2018%E5%B9%B409%E6%9C%8812%E6%97%A510%E7%82%B943%E5%88%86%E7%B4%A7%E6%80%A5%E5%86%85%E5%AE%B9%E5%87%86%E5%85%A5%E5%8D%8E%E7%BA%B3179%E9%A6%96/%E6%A0%87%E6%B8%85%E9%AB%98%E6%B8%85/MP3_128_16_Stero/6005751EPFG164228.mp3?channelid=02&msisdn=d43a7dcc-8498-461b-ba22-3205e9b6aa82&Tim=1728484238063&Key=0442fa065dacda7c";
// const char *fileDownloadUrl = "https://github.com/espressif/arduino-esp32/releases/download/3.0.1/esp32-3.0.1.zip";

const uint64_t Local_MAC = ESP.getEfuseMac();

bool Wifi_Connection_Flag = true;
size_t CycleTime = 0;
size_t CycleTime_2 = 0;
size_t CycleTime_3 = 0;
size_t CycleTime_4 = 0;
size_t CycleTime_5 = 0;
size_t CycleTime_6 = 0;
size_t CycleTime_7 = 0;
size_t CycleTime_8 = 0;
uint8_t Image_Flag = 0;

bool Skip_Current_Test = false;

uint8_t Current_Rotation = 1;

struct RS485_Operator
{
    using state = enum {
        UNCONNECTED, // 未连接
        CONNECTED,   // 已连接
        CONNECTING,  // 正在连接
        PAUSE,       // 传输数据错误暂停连接
    };

    struct
    {
        struct
        {
            String code = "null";
            uint8_t count = 0;
        } error;

        uint32_t send_data = 0;
        uint32_t receive_data = 0;
        uint8_t connection_status = state::UNCONNECTED;
        bool send_flag = false;
    } device_1;

    struct
    {
        struct
        {
            String code = "null";
            uint8_t count = 0;
        } error;

        uint32_t send_data = 0;
        uint32_t receive_data = 0;
        uint8_t connection_status = state::UNCONNECTED;
        bool send_flag = false;

    } device_2;

    uint8_t send_package_1[105] =
        {
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

    uint8_t send_package_2[105] =
        {
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
};

struct CAN_Operator
{
    using state = enum {
        UNCONNECTED, // 未连接
        CONNECTED,   // 已连接
        CONNECTING,  // 正在连接
        PAUSE,       // 传输数据错误暂停连接
    };

    struct
    {
        struct
        {
            String code = "null";
            uint8_t count = 0;
        } error;

        uint32_t send_data = 0;
        uint32_t receive_data = 0;
        uint8_t connection_status = state::UNCONNECTED;
        bool send_flag = false;
    } device_1;
};

struct Ethernet_Relay_Operator
{
    struct
    {
        String code = "null";
        bool flag = false;
    } initialization;

    // Web relay trigger flag
    bool html_relay1_flag = false;
};

struct SX1262_Operator
{
    using state = enum {
        UNCONNECTED, // 未连接
        CONNECTED,   // 已连接
        CONNECTING,  // 正在连接
    };

    using mode = enum {
        LORA, // 普通LoRa模式
        FSK,  // 普通FSK模式
    };

    struct
    {
        float value = 920.0;
        bool change_flag = false;
    } frequency;
    struct
    {
        float value = 125.0;
        bool change_flag = false;
    } bandwidth;
    struct
    {
        uint8_t value = 12;
        bool change_flag = false;
    } spreading_factor;
    struct
    {
        uint8_t value = 8;
        bool change_flag = false;
    } coding_rate;
    struct
    {
        uint8_t value = 0x14;
        bool change_flag = false;
    } sync_word;
    struct
    {
        int8_t value = 22;
        bool change_flag = false;
    } output_power;
    struct
    {
        float value = 140;
        bool change_flag = false;
    } current_limit;
    struct
    {
        int16_t value = 16;
        bool change_flag = false;
    } preamble_length;
    struct
    {
        bool value = true;
        bool change_flag = false;
    } crc;

    struct
    {
        uint64_t mac = 0;
        uint32_t send_data = 0;
        uint32_t receive_data = 0;
        uint8_t connection_flag = state::UNCONNECTED;
        bool send_flag = false;
        uint8_t error_count = 0;
    } device_1;

    uint8_t current_mode = mode::LORA;

    volatile bool operation_flag = false;
    bool initialization_flag = false;
    bool mode_change_flag = false;

    uint8_t send_package[16] = {'M', 'A', 'C', ':',
                                (uint8_t)(Local_MAC >> 56), (uint8_t)(Local_MAC >> 48),
                                (uint8_t)(Local_MAC >> 40), (uint8_t)(Local_MAC >> 32),
                                (uint8_t)(Local_MAC >> 24), (uint8_t)(Local_MAC >> 16),
                                (uint8_t)(Local_MAC >> 8), (uint8_t)Local_MAC,
                                0, 0, 0, 0};

    float receive_rssi = 0;
    float receive_snr = 0;
};

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
// The MAC address must be an even number first !
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

RS485_Operator RS485_OP;
CAN_Operator CAN_OP;
Ethernet_Relay_Operator Ethernet_Relay_OP;
SX1262_Operator SX1262_OP;

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

int8_t colour_count = 0;

SX1262 radio = new Module(SX1262_CS, SX1262_DIO1, SX1262_RST, SX1262_BUSY, SPI);

void Dio1_Action_Interrupt(void)
{
    // we sent or received a packet, set the flag
    SX1262_OP.operation_flag = true;
}

void Skip_Test_Loop()
{
    if (digitalRead(ESP_BOOT) == LOW)
    {
        delay(300);
        Serial.printf("ESP_BOOT triggered Start the next test\n\n");
        Skip_Current_Test = true;
    }
}

bool Uart_Check_Dynamic_Data(uint8_t *uart_data, uint32_t check_data)
{
    uint32_t temp = (uint32_t)uart_data[1] << 24 | (uint32_t)uart_data[2] << 16 |
                    (uint32_t)uart_data[3] << 8 | (uint32_t)uart_data[4];

    if (temp == 0)
    {
        if (temp == check_data)
        {
            return true;
        }
    }
    else
    {
        if (temp == check_data + 1)
        {
            return true;
        }
    }

    return false;
}

bool Uart_Check_Static_Data(uint8_t *uart_data, uint8_t *check_data)
{
    if (memcmp(&uart_data[5], &check_data[5], 100) == 0)
    {
        return true;
    }
    return false;
}

void Ethernet_Reset(const uint8_t resetPin)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(250);
    digitalWrite(resetPin, LOW);
    delay(50);
    digitalWrite(resetPin, HIGH);
    delay(350);
}

void Twai_Send_Message()
{
    // Configure message to transmit
    twai_message_t message;
    message.identifier = 0xF1;
    // message.data_length_code = 1;
    // message.data[0] = CAN_Count++;
    message.data_length_code = 8;
    message.data[0] = 0x0A;
    message.data[1] = 0x0A;
    message.data[2] = 0x0A;
    message.data[3] = 0x0A;
    message.data[4] = (uint8_t)(CAN_OP.device_1.send_data >> 24);
    message.data[5] = (uint8_t)(CAN_OP.device_1.send_data >> 16);
    message.data[6] = (uint8_t)(CAN_OP.device_1.send_data >> 8);
    message.data[7] = (uint8_t)CAN_OP.device_1.send_data;

    // Queue message for transmission
    if (twai_transmit(&message, pdMS_TO_TICKS(TWAI_TRANSMIT_OVERTIME_MS)) == ESP_OK)
    {
        // printf("Message queued for transmission\n");
    }
    else
    {
        printf("[CAN] Failed to queue message for transmission\n");
    }
}

void Twai_Receive_Message(twai_message_t &message)
{
    // Process received message
    if (message.extd)
    {
        Serial.println("[CAN] Message is in Extended Format");
    }
    else
    {
        // Serial.println("Message is in Standard Format");
    }
    // Serial.printf("ID: 0x%X\n", message.identifier);
    if (!(message.rtr))
    {
        // for (int i = 0; i < message.data_length_code; i++)
        // {
        //     Serial.printf("Data [%d] = %d\n", i, message.data[i]);
        // }
        // Serial.println("");

        if ((message.data[0] == 0x0A) &&
            (message.data[1] == 0x0A) &&
            (message.data[2] == 0x0A) &&
            (message.data[3] == 0x0A))
        {
            CAN_OP.device_1.receive_data =
                ((uint32_t)message.data[4] << 24) |
                ((uint32_t)message.data[5] << 16) |
                ((uint32_t)message.data[6] << 8) |
                (uint32_t)message.data[7];

            CAN_OP.device_1.send_data = CAN_OP.device_1.receive_data + 1;
            CAN_OP.device_1.connection_status = CAN_OP.state::CONNECTED;
            // 清除错误计数看门狗
            CAN_OP.device_1.error.count = 0;
            CAN_OP.device_1.send_flag = true;
            CycleTime_7 = millis() + 1000;
        }
    }
}

bool Ethernet_Initialization_Assertion(String *assertion)
{
    switch (Ethernet.hardwareStatus())
    {
    case EthernetNoHardware:
        // no point in carrying on, so do nothing forevermore:

        Serial.println("Ethernet No Hardware");
        *assertion = "Ethernet No Hardware";
        return false;

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

        Serial.print("Link status: ");
        Serial.println("Unknown");
        Serial.println("Hardware error !");

        *assertion = "Hardware error";
        return false;

        break;
    case LinkON:
        Serial.print("Link status: ");
        Serial.println("ON");
        break;
    case LinkOFF:
        Serial.print("Link status: ");
        Serial.println("OFF");
        Serial.println("The network cable is not connected !");

        *assertion = "Please insert the network cable";
        return false;

        break;
    }

    // start the Ethernet connection:
    Serial.println("Trying to get an IP address using DHCP...");
    if (Ethernet.begin(mac, 10000, 10000) == 0)
    {
        // initialize the Ethernet device not using DHCP:
        // Ethernet.begin(mac, ip, myDns, gateway, subnet);

        Serial.println("-------------------------");
        Serial.println("[INFO] Configuring random DHCP failed !");
        Serial.println("");
        // Serial.println("[INFO] Configuring static IP...");
        // Serial.print("[Static] IP Address: ");
        // Serial.println(Ethernet.localIP());
        // Serial.print("[Static] Subnet Mask: ");
        // Serial.println(Ethernet.subnetMask());
        // Serial.print("[Static] Gateway: ");
        // Serial.println(Ethernet.gatewayIP());
        // Serial.print("[Static] DNS: ");
        // Serial.println(Ethernet.dnsServerIP());
        // Serial.println("-------------------------");
        // Serial.println("");

        *assertion = "DHCP configuration failed";
        return false;
    }
    else
    {
        Serial.println("-------------------------");
        Serial.println("[INFO] Configuring random DHCP successfully !");
        Serial.println("");
        Serial.print("[DHCP] IP Address: ");
        Serial.println(Ethernet.localIP());
        Serial.print("[DHCP] Subnet Mask: ");
        Serial.println(Ethernet.subnetMask());
        Serial.print("[DHCP] Gateway: ");
        Serial.println(Ethernet.gatewayIP());
        Serial.print("[DHCP] DNS: ");
        Serial.println(Ethernet.dnsServerIP());
        Serial.println("-------------------------");
        Serial.println("");
    }
    return true;
}

void RS485_Initialization(void)
{
    while (Serial1.available() > 0) // 清空缓存
    {
        Serial1.read();
    }
    while (Serial2.available() > 0) // 清空缓存
    {
        Serial2.read();
    }
}

void CAN_Drive_Initialization(void)
{
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.println("[CAN] Driver installed");
    }
    else
    {
        Serial.println("[CAN] Failed to install driver");
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        Serial.println("[CAN] Driver started");
    }
    else
    {
        Serial.println("[CAN] Failed to start driver");
    }

    // 配置
    uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                                TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                                TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_DATA |
                                TWAI_ALERT_RX_QUEUE_FULL;

    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
        Serial.println("[CAN] CAN Alerts reconfigured");
    }
    else
    {
        Serial.println("[CAN] Failed to reconfigure alerts");
    }
}

void Ethernet_Initialization(void)
{
    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS);
    Ethernet_Reset(W5500_RST);
    Ethernet.init(W5500_CS);

    W5100.init();
}

void Wifi_STA_Test(void)
{
    String text;
    int wifi_num = 0;

    Serial.println("\nScanning wifi");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.setTxPower(wifi_power_t ::WIFI_POWER_19_5dBm);
    delay(100);

    wifi_num = WiFi.scanNetworks();
    if (wifi_num == 0)
    {
        text = "\nWiFi scan complete !\nNo wifi discovered.\n";
    }
    else
    {
        text = "\nWiFi scan complete !\n";
        text += wifi_num;
        text += " wifi discovered.\n\n";

        for (int i = 0; i < wifi_num; i++)
        {
            text += (i + 1);
            text += ": ";
            text += WiFi.SSID(i);
            text += " (";
            text += WiFi.RSSI(i);
            text += ")";
            text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
            delay(10);
        }
    }

    Serial.println(text);

    delay(3000);
    text.clear();

    Serial.print("Connecting to ");

    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t last_tick = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);

        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        {
            Wifi_Connection_Flag = false;
            break;
        }
    }

    if (Wifi_Connection_Flag == true)
    {
        Serial.print("\nThe connection was successful ! \nTakes ");
        Serial.print(millis() - last_tick);
        Serial.print(" ms\n");

        // Serial.println("\nWifi test passed!");
    }
    else
    {
        Serial.println("\nWifi test error!\n");
    }
}

void WIFI_HTTP_Download_File(void)
{
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
        // 获取文件大小
        size_t fileSize = http.getSize();
        Serial.printf("Starting file download...\n");
        Serial.printf("file size: %f MB\n", fileSize / 1024.0 / 1024.0);

        // 读取HTTP响应
        WiFiClient *stream = http.getStreamPtr();

        size_t temp_count_s = 0;
        size_t temp_fileSize = fileSize;
        // uint8_t *buf_1 = (uint8_t *)heap_caps_malloc(64 * 1024, MALLOC_CAP_SPIRAM);
        uint8_t buf_1[4096] = {0};
        CycleTime = millis() + 3000; // 开始计时
        while (http.connected() && (temp_fileSize > 0 || temp_fileSize == -1))
        {
            // 获取可用数据的大小
            size_t availableSize = stream->available();
            if (availableSize)
            {
                // temp_fileSize -= stream->read(buf_1, min(availableSize, (size_t)(64 * 1024)));
                temp_fileSize -= stream->read(buf_1, min(availableSize, (size_t)(4096)));

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
            if (temp_count_s > 30)
            {
                break;
            }
        }
        // 关闭HTTP客户端
        http.end();

        // 记录下载结束时间并计算总花费时间
        size_t endTime = millis();

        if (temp_count_s < 30)
        {
            Serial.printf("Download completed!\n");
            Serial.printf("Total download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
            Serial.printf("Average download speed: %f KB/s\n", (fileSize / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
        }
        else
        {
            Serial.printf("Download incomplete!\n");
            Serial.printf("Total download time: %f s\n", (endTime - startTime - uselessTime) / 1000.0);
            Serial.printf("Average download speed: %f KB/s\n", ((fileSize - temp_fileSize) / 1024.0) / ((endTime - startTime - uselessTime) / 1000.0));
        }
    }
    else
    {
        Serial.printf("Failed to download\n");
        Serial.printf("Error httpCode: %d \n", httpCode);
    }
}

// void PrintLocalTime(void)
// {
//     struct tm timeinfo;
//     uint32_t temp_buf = 0;
//     bool temp_buf_2 = true;

//     while (getLocalTime(&timeinfo) == false)
//     {
//         Serial.printf("\n.");
//         Serial.println("\n.");
//         configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
//         temp_buf++;
//         if (temp_buf > 5)
//         {
//             temp_buf_2 = false;
//             break;
//         }
//     }
//     if (temp_buf_2 == false)
//     {
//         Serial.println("Failed to obtain time");
//         gfx->setCursor(80, 200);
//         gfx->setTextColor(RED);
//         gfx->print("Failed to obtain time!");
//         return;
//     }

//     Serial.println("Get time success");
//     Serial.println(&timeinfo, "%A,%B %d %Y %H:%M:%S"); // Format Output
//     gfx->setCursor(80, 200);
//     gfx->setTextColor(PURPLE);
//     gfx->print(&timeinfo, " %Y");
//     gfx->setCursor(80, 215);
//     gfx->print(&timeinfo, "%B %d");
//     gfx->setCursor(80, 230);
//     gfx->print(&timeinfo, "%H:%M:%S");
// }

void GFX_Print_Time_Info_Loop()
{
    if (Wifi_Connection_Flag == true)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 1000))
        {
            Serial.println("Failed to obtain time");
            return;
        }
        Serial.println("Get time success");
        Serial.println(&timeinfo, "%A,%B %d %Y %H:%M:%S"); // Format Output
    }
    else
    {
        Serial.println("Network error");
    }

    Serial.printf("SYS Time: %d\n", (uint32_t)millis() / 1000);
}

void Serial_Print_TEST(String s)
{
    Skip_Current_Test = false;

    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Black;
    }
    FastLED.show();
    delay(1000);

    Serial.printf("TEST\n%s\n\n", s.c_str());

    Serial.printf("4\n");
    *led[3] = CRGB::White;
    FastLED.show();
    delay(1000);
    Serial.printf("3\n");
    *led[2] = CRGB::White;
    FastLED.show();
    for (int i = 0; i < 100; i++)
    {
        Skip_Test_Loop();
        delay(10);

        if (Skip_Current_Test == true)
        {
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Black;
            }
            FastLED.show();
            return;
        }
    }
    Serial.printf("2\n");
    *led[1] = CRGB::White;
    FastLED.show();
    for (int i = 0; i < 100; i++)
    {
        Skip_Test_Loop();
        delay(10);

        if (Skip_Current_Test == true)
        {
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Black;
            }
            FastLED.show();
            return;
        }
    }
    Serial.printf("1\n");
    *led[0] = CRGB::White;
    FastLED.show();
    for (int i = 0; i < 100; i++)
    {
        Skip_Test_Loop();
        delay(10);

        if (Skip_Current_Test == true)
        {
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Black;
            }
            FastLED.show();
            return;
        }
    }

    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Black;
    }
    FastLED.show();
}

// void GFX_Print_FINISH()
// {
//     gfx->setCursor(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4 - 30);
//     gfx->setTextSize(3);
//     gfx->setTextColor(ORANGE);
//     Serial.println("FINISH");
// }

// void GFX_Print_START()
// {
//     switch (Current_Rotation)
//     {
//     case 0:
//     case 2:
//         gfx->setCursor(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 50);
//         break;
//     case 1:
//     case 3:
//         gfx->setCursor(SCREEN_HEIGHT / 2 - 50, SCREEN_WIDTH / 2 - 20);
//         break;

//     default:
//         break;
//     }

//     gfx->setTextSize(4);
//     gfx->setTextColor(RED);
//     Serial.println("START");
// }

void GFX_Print_RS485CAN_Info(void)
{
    Serial.println("\nRS485232CAN Info");

    Serial.println("[RS485232]:115200 bps/s");
    Serial.println("[CAN]:1 mbit/s\n");
}

void GFX_Print_Ethernet_Info(void)
{
    Serial.println("Eth Relay Info");
}

void GFX_Print_RS485_Info_Loop(void)
{
    if (millis() > CycleTime)
    {
        Serial.println("<----------UART Info----------> ");
        Serial.println("[RS232]:");

        if (RS485_OP.device_1.connection_status == RS485_OP.state::UNCONNECTED)
        {
            Serial.println("   [Connect]: Unconnected");
        }
        else if (RS485_OP.device_1.connection_status == RS485_OP.state::CONNECTED)
        {
            Serial.println("   [Connect]: Connected");
            Serial.printf("   [Receive Data]: %u\n", RS485_OP.device_1.receive_data);
        }
        else if (RS485_OP.device_1.connection_status == RS485_OP.state::CONNECTING)
        {
            Serial.println("   [Connect]: Connecting");
            Serial.printf("   [Send Data]: %u\n", RS485_OP.device_1.send_data);
        }
        else if (RS485_OP.device_1.connection_status == RS485_OP.state::PAUSE)
        {
            Serial.println("   [Connect]: Pause");
            Serial.printf("   [Error]: %s\n", RS485_OP.device_1.error.code.c_str());
        }

        Serial.println("[RS485]:");
        if (RS485_OP.device_2.connection_status == RS485_OP.state::UNCONNECTED)
        {
            Serial.println("   [Connect]: Unconnected");
        }
        else if (RS485_OP.device_2.connection_status == RS485_OP.state::CONNECTED)
        {
            Serial.println("   [Connect]: Connected");
            Serial.printf("   [Receive Data]: %u\n", RS485_OP.device_2.receive_data);
        }
        else if (RS485_OP.device_2.connection_status == RS485_OP.state::CONNECTING)
        {
            Serial.println("   [Connect]: Connecting");
            Serial.printf("   [Send Data]: %u\n", RS485_OP.device_2.send_data);
        }
        else if (RS485_OP.device_2.connection_status == RS485_OP.state::PAUSE)
        {
            Serial.println("   [Connect]: Pause");
            Serial.printf("   [Error]: %s\n", RS485_OP.device_2.error.code.c_str());
        }

        CycleTime = millis() + 2000;
    }

    if (RS485_OP.device_1.send_flag == true)
    {
        if (millis() > CycleTime_2)
        {
            RS485_OP.device_1.send_flag = false;

            RS485_OP.send_package_1[1] = (uint8_t)(RS485_OP.device_1.send_data >> 24);
            RS485_OP.send_package_1[2] = (uint8_t)(RS485_OP.device_1.send_data >> 16);
            RS485_OP.send_package_1[3] = (uint8_t)(RS485_OP.device_1.send_data >> 8);
            RS485_OP.send_package_1[4] = (uint8_t)RS485_OP.device_1.send_data;

            // send another one
            Serial.println("[RS232] Sending another packet ... ");

            Serial1.write(RS485_OP.send_package_1, 105);
        }
    }

    if (RS485_OP.device_2.send_flag == true)
    {
        if (millis() > CycleTime_4)
        {
            RS485_OP.device_2.send_flag = false;

            RS485_OP.send_package_2[1] = (uint8_t)(RS485_OP.device_2.send_data >> 24);
            RS485_OP.send_package_2[2] = (uint8_t)(RS485_OP.device_2.send_data >> 16);
            RS485_OP.send_package_2[3] = (uint8_t)(RS485_OP.device_2.send_data >> 8);
            RS485_OP.send_package_2[4] = (uint8_t)RS485_OP.device_2.send_data;

            // send another one
            Serial.println("[RS485] Sending another packet ... ");

            Serial2.write(RS485_OP.send_package_2, 105);
        }
    }

    if (RS485_OP.device_1.connection_status != RS485_OP.state::PAUSE)
    {
        while (Serial1.available() > 0)
        {
            // delay(1000); // 接收等待
            uint8_t uart_receive_package[105] = {0};
            Serial1.read(uart_receive_package, sizeof(uart_receive_package));

            if (uart_receive_package[0] == 0x0A)
            {
                if (Uart_Check_Dynamic_Data(uart_receive_package, RS485_OP.device_1.send_data) == false) // 动态数据校验
                {
                    // while (1)
                    // {
                    Serial.printf("[RS232] Check Dynamic Data Failed\n");
                    Serial.printf("[RS232] Check Data: %d\n", RS485_OP.device_1.send_data);
                    Serial.printf("[RS232] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                                                                     (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                    Serial.printf("[RS232] Received Buf[1]: %#X\n", uart_receive_package[1]);
                    Serial.printf("[RS232] Received Buf[2]: %#X\n", uart_receive_package[2]);
                    Serial.printf("[RS232] Received Buf[3]: %#X\n", uart_receive_package[3]);
                    Serial.printf("[RS232] Received Buf[4]: %#X\n", uart_receive_package[4]);
                    //     delay(1000);
                    // }
                    RS485_OP.device_1.error.code = "Dynamic data error";
                    RS485_OP.device_1.connection_status = RS485_OP.state::PAUSE;
                }
                else if (Uart_Check_Static_Data(uart_receive_package, RS485_OP.send_package_1) == false) // 静态数据校验
                {
                    Serial.printf("[RS232] Check Static Data Failed\n");
                    for (int i = 0; i < 100; i++)
                    {
                        Serial.printf("[RS232] Received Buf[%d]: %#X\n", i + 5, uart_receive_package[i + 5]);
                    }
                    RS485_OP.device_1.error.code = "Static data error";
                    RS485_OP.device_1.connection_status = RS485_OP.state::PAUSE;
                    // delay(1000);
                }
                else
                {
                    // delay(500);

                    // Serial.printf("[RS232] Check Data Successful\n");
                    // Serial.printf("[RS232] Check Data: %d\n", RS485_OP.device_1.send_data);
                    // Serial.printf("[RS232] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                    //                                                  (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                    // Serial.printf("[RS232] Received Buf[1]: %#X\n", uart_receive_package[1]);
                    // Serial.printf("[RS232] Received Buf[2]: %#X\n", uart_receive_package[2]);
                    // Serial.printf("[RS232] Received Buf[3]: %#X\n", uart_receive_package[3]);
                    // Serial.printf("[RS232] Received Buf[4]: %#X\n", uart_receive_package[4]);

                    // Serial.printf("[RS232] Received Buf[105]: %#X\n", uart_receive_package[104]);

                    RS485_OP.device_1.receive_data =
                        ((uint32_t)uart_receive_package[1] << 24) |
                        ((uint32_t)uart_receive_package[2] << 16) |
                        ((uint32_t)uart_receive_package[3] << 8) |
                        (uint32_t)uart_receive_package[4];

                    RS485_OP.device_1.send_data = RS485_OP.device_1.receive_data + 1;

                    RS485_OP.device_1.connection_status = RS485_OP.state::CONNECTED;
                    // 清除错误计数看门狗
                    RS485_OP.device_1.error.count = 0;

                    RS485_OP.device_1.send_flag = true;
                    CycleTime_2 = millis() + 1000;
                }
            }
            else
            {
                Serial.printf("[RS232] Check Header Failed\n");
                Serial.printf("[RS232] Received Header: %#X\n", uart_receive_package[0]);
                Serial.printf("[RS232] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                                                                 (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                Serial.printf("[RS232] Received Buf[1]: %#X\n", uart_receive_package[1]);
                Serial.printf("[RS232] Received Buf[2]: %#X\n", uart_receive_package[2]);
                Serial.printf("[RS232] Received Buf[3]: %#X\n", uart_receive_package[3]);
                Serial.printf("[RS232] Received Buf[4]: %#X\n", uart_receive_package[4]);

                RS485_OP.device_1.error.code = "Header error";
                RS485_OP.device_1.connection_status = RS485_OP.state::PAUSE;
            }
        }
    }
    else
    {
        if (Serial1.available() > 0)
        {
            RS485_OP.device_1.connection_status = RS485_OP.state::CONNECTING;
            RS485_OP.device_1.send_data = 0;
            // 清除错误计数看门狗
            RS485_OP.device_1.error.count = 0;
        }
    }

    if (RS485_OP.device_2.connection_status != RS485_OP.state::PAUSE)
    {
        while (Serial2.available() > 0)
        {
            // delay(1000); // 接收等待
            uint8_t uart_receive_package[105] = {0};
            Serial2.read(uart_receive_package, sizeof(uart_receive_package));

            if (uart_receive_package[0] == 0x0A)
            {
                if (Uart_Check_Dynamic_Data(uart_receive_package, RS485_OP.device_2.send_data) == false) // 动态数据校验
                {
                    // while (1)
                    // {
                    Serial.printf("[RS485] Check Dynamic Data Failed\n");
                    Serial.printf("[RS485] Check Data: %d\n", RS485_OP.device_2.send_data);
                    Serial.printf("[RS485] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                                                                     (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                    Serial.printf("[RS485] Received Buf[1]: %#X\n", uart_receive_package[1]);
                    Serial.printf("[RS485] Received Buf[2]: %#X\n", uart_receive_package[2]);
                    Serial.printf("[RS485] Received Buf[3]: %#X\n", uart_receive_package[3]);
                    Serial.printf("[RS485] Received Buf[4]: %#X\n", uart_receive_package[4]);
                    //     delay(1000);
                    // }
                    RS485_OP.device_2.error.code = "Dynamic data error";
                    RS485_OP.device_2.connection_status = RS485_OP.state::PAUSE;
                }
                else if (Uart_Check_Static_Data(uart_receive_package, RS485_OP.send_package_2) == false) // 静态数据校验
                {
                    Serial.printf("[RS485] Check Static Data Failed\n");
                    for (int i = 0; i < 100; i++)
                    {
                        Serial.printf("[RS485] Received Buf[%d]: %#X\n", i + 5, uart_receive_package[i + 5]);
                    }
                    RS485_OP.device_2.error.code = "Static data error";
                    RS485_OP.device_2.connection_status = RS485_OP.state::PAUSE;
                    // delay(1000);
                }
                else
                {
                    // delay(500);

                    Serial.printf("[RS485] Check Data Successful\n");
                    Serial.printf("[RS485] Check Data: %d\n", RS485_OP.device_2.send_data);
                    Serial.printf("[RS485] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                                                                     (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                    Serial.printf("[RS485] Received Buf[1]: %#X\n", uart_receive_package[1]);
                    Serial.printf("[RS485] Received Buf[2]: %#X\n", uart_receive_package[2]);
                    Serial.printf("[RS485] Received Buf[3]: %#X\n", uart_receive_package[3]);
                    Serial.printf("[RS485] Received Buf[4]: %#X\n", uart_receive_package[4]);

                    Serial.printf("[RS485] Received Buf[105]: %#X\n", uart_receive_package[104]);

                    RS485_OP.device_2.receive_data =
                        ((uint32_t)uart_receive_package[1] << 24) |
                        ((uint32_t)uart_receive_package[2] << 16) |
                        ((uint32_t)uart_receive_package[3] << 8) |
                        (uint32_t)uart_receive_package[4];

                    RS485_OP.device_2.send_data = RS485_OP.device_2.receive_data + 1;

                    RS485_OP.device_2.connection_status = RS485_OP.state::CONNECTED;
                    // 清除错误计数看门狗
                    RS485_OP.device_2.error.count = 0;

                    RS485_OP.device_2.send_flag = true;
                    CycleTime_4 = millis() + 1000;
                }
            }
            else
            {
                Serial.printf("[RS485] Check Header Failed\n");
                Serial.printf("[RS485] Received Header: %#X\n", uart_receive_package[0]);
                Serial.printf("[RS485] Received Data: %d\n", (uint32_t)uart_receive_package[1] << 24 | (uint32_t)uart_receive_package[2] << 16 |
                                                                 (uint32_t)uart_receive_package[3] << 8 | (uint32_t)uart_receive_package[4]);
                Serial.printf("[RS485] Received Buf[1]: %#X\n", uart_receive_package[1]);
                Serial.printf("[RS485] Received Buf[2]: %#X\n", uart_receive_package[2]);
                Serial.printf("[RS485] Received Buf[3]: %#X\n", uart_receive_package[3]);
                Serial.printf("[RS485] Received Buf[4]: %#X\n", uart_receive_package[4]);

                RS485_OP.device_2.error.code = "Header error";
                RS485_OP.device_2.connection_status = RS485_OP.state::PAUSE;
            }
        }
    }
    else
    {
        if (Serial2.available() > 0)
        {
            RS485_OP.device_2.connection_status = RS485_OP.state::CONNECTING;
            RS485_OP.device_2.send_data = 0;
            // 清除错误计数看门狗
            RS485_OP.device_2.error.count = 0;
        }
    }

    if (RS485_OP.device_1.connection_status != RS485_OP.state::PAUSE)
    {
        if (millis() > CycleTime_3)
        {
            RS485_OP.device_1.error.count++;
            if (RS485_OP.device_1.error.count > 5)
            {
                RS485_OP.device_1.error.count = 6;
                RS485_OP.device_1.send_data = 0;
                RS485_OP.device_1.connection_status = RS485_OP.state::UNCONNECTED;
            }

            CycleTime_3 = millis() + 1000;
        }
    }

    if (RS485_OP.device_2.connection_status != RS485_OP.state::PAUSE)
    {
        if (millis() > CycleTime_8)
        {
            RS485_OP.device_2.error.count++;
            if (RS485_OP.device_2.error.count > 5)
            {
                RS485_OP.device_2.error.count = 6;
                RS485_OP.device_2.send_data = 0;
                RS485_OP.device_2.connection_status = RS485_OP.state::UNCONNECTED;
            }

            CycleTime_8 = millis() + 1000;
        }
    }
}

void GFX_Print_CAN_Info_Loop(void)
{
    if (millis() > CycleTime_5)
    {
        Serial.println("<------------CAN Info------------> ");
        Serial.println("[CAN]:");
        if (CAN_OP.device_1.connection_status == CAN_OP.state::UNCONNECTED)
        {
            Serial.println("   [Connect]: Unconnected");
        }
        else if (CAN_OP.device_1.connection_status == CAN_OP.state::CONNECTED)
        {
            Serial.println("   [Connect]: Connected");
            Serial.printf("   [Receive Data]: %u\n", CAN_OP.device_1.receive_data);
        }
        else if (CAN_OP.device_1.connection_status == CAN_OP.state::CONNECTING)
        {
            Serial.println("   [Connect]: Connecting");
            Serial.printf("   [Send Data]: %u\n", CAN_OP.device_1.send_data);
        }
        else if (CAN_OP.device_1.connection_status == CAN_OP.state::PAUSE)
        {
            Serial.println("   [Connect]: Pause");
            Serial.printf("   [Error]: %s\n", CAN_OP.device_1.error.code.c_str());
        }

        Serial.println();

        CycleTime_5 = millis() + 2000;
    }

    if (CAN_OP.device_1.send_flag == true)
    {
        if (millis() > CycleTime_7)
        {
            CAN_OP.device_1.send_flag = false;

            // send another one
            Serial.println("[CAN] Sending another packet ... \n");

            Twai_Send_Message();
        }
    }

    if (CAN_OP.device_1.connection_status != CAN_OP.state::PAUSE)
    {
        // 通信报警检测
        uint32_t alerts_triggered;
        twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS));
        // 总线状态信息
        twai_status_info_t twai_status_info;
        twai_get_status_info(&twai_status_info);

        // switch (alerts_triggered)
        // {
        // case TWAI_ALERT_ERR_PASS:
        //     Serial.println("\nAlert: TWAI controller has become error passive.");
        //     CAN_OP.device_1.error.code = "TWAI_ERR_PASS";
        //     CAN_OP.device_1.connection_status = CAN_OP.state::PAUSE;
        //     // delay(1000);
        //     break;
        // case TWAI_ALERT_BUS_ERROR:
        //     Serial.println("\nAlert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
        //     Serial.printf("Bus error count: %d\n", twai_status_info.bus_error_count);
        //     // delay(1000);
        //     break;
        // case TWAI_ALERT_TX_FAILED:
        //     Serial.println("\nAlert: The Transmission failed.");
        //     Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
        //     Serial.printf("TX error: %d\t", twai_status_info.tx_error_counter);
        //     Serial.printf("TX failed: %d\n", twai_status_info.tx_failed_count);
        //     // delay(1000);
        //     break;
        // case TWAI_ALERT_TX_SUCCESS:
        //     Serial.println("\nAlert: The Transmission was successful.");
        //     Serial.printf("TX buffered: %d\t", twai_status_info.msgs_to_tx);
        //     break;
        // case TWAI_ALERT_RX_QUEUE_FULL:
        //     Serial.println("\nAlert: The RX queue is full causing a received frame to be lost.");
        //     Serial.printf("RX buffered: %d\t", twai_status_info.msgs_to_rx);
        //     Serial.printf("RX missed: %d\t", twai_status_info.rx_missed_count);
        //     Serial.printf("RX overrun %d\n", twai_status_info.rx_overrun_count);
        //     // delay(1000);
        //     break;

        // default:
        //     break;
        // }

        // switch (twai_status_info.state)
        // {
        // case TWAI_STATE_RUNNING:
        //     Serial.println("\nTWAI_STATE_RUNNING");
        //     break;
        // case TWAI_STATE_BUS_OFF:
        //     Serial.println("\nTWAI_STATE_BUS_OFF");
        //     twai_initiate_recovery();
        //     // delay(1000);
        //     CAN_OP.device_1.error.code = "BUS_OFF";
        //     CAN_OP.device_1.connection_status = CAN_OP.state::PAUSE;
        //     break;
        // case TWAI_STATE_STOPPED:
        //     Serial.println("\nTWAI_STATE_STOPPED");
        //     twai_start();
        //     // delay(1000);
        //     CAN_OP.device_1.error.code = "BUS_STOPPED";
        //     CAN_OP.device_1.connection_status = CAN_OP.state::PAUSE;
        //     break;
        // case TWAI_STATE_RECOVERING:
        //     Serial.println("\nTWAI_STATE_RECOVERING");
        //     // delay(1000);
        //     break;

        // default:
        //     break;
        // }

        // 如果TWAI有信息接收到
        if (alerts_triggered & TWAI_ALERT_RX_DATA)
        {
            twai_message_t rx_buf;
            while (twai_receive(&rx_buf, 0) == ESP_OK)
            {
                Twai_Receive_Message(rx_buf);
            }
        }
    }

    if (CAN_OP.device_1.connection_status != CAN_OP.state::PAUSE)
    {
        if (millis() > CycleTime_6)
        {
            CAN_OP.device_1.error.count++;
            if (CAN_OP.device_1.error.count > 5)
            {
                CAN_OP.device_1.error.count = 6;
                CAN_OP.device_1.send_data = 0;
                CAN_OP.device_1.connection_status = CAN_OP.state::UNCONNECTED;
            }
            CycleTime_6 = millis() + 1000;
        }
    }
}

void GFX_Print_RS485CAN_Info_Loop(void)
{
    GFX_Print_RS485_Info_Loop();
    GFX_Print_CAN_Info_Loop();
}

void GFX_Print_Ethernet_Info_Loop(void)
{
    if (millis() > CycleTime)
    {
        if (Ethernet_Relay_OP.initialization.flag == false)
        {
            Serial.println("[State]:");
            Serial.println("  Initialization failed");
            Serial.println("[Assertion]:");
            Serial.printf("  %s\n", Ethernet_Relay_OP.initialization.code.c_str());
        }
        else
        {
            Serial.println("[State]:");
            Serial.println("  Initialization successful");
            Serial.print("[IP address]: ");
            Serial.println(Ethernet.localIP());
            Serial.print("[Subnet mask]: ");
            Serial.println(Ethernet.subnetMask());
            Serial.print("[Gateway]: ");
            Serial.println(Ethernet.gatewayIP());
            Serial.print("[DNS]: ");
            Serial.println(Ethernet.dnsServerIP());
        }

        if (Ethernet_Relay_OP.initialization.flag == false)
        {
            if (Ethernet_Initialization_Assertion(&Ethernet_Relay_OP.initialization.code) == false)
            {
                Ethernet_Relay_OP.initialization.flag = false;
            }
            else
            {
                // start listening for clients
                server.begin();

                Ethernet_Relay_OP.initialization.flag = true;
            }
        }

        CycleTime = millis() + 3000;
    }

    if (millis() > CycleTime_2)
    {
        if (Ethernet.linkStatus() != LinkON)
        {
            Ethernet_Relay_OP.initialization.flag = false;
        }
        CycleTime_2 = millis() + 3000;
    }

    if (Ethernet_Relay_OP.initialization.flag == true)
    {
        EthernetClient client = server.available();
        if (client)
        {                                  // if you get a client,
            Serial.println("New Client."); // print a message out the serial port
            String currentLine = "";       // make a String to hold incoming data from the client
            while (client.connected())
            { // loop while the client's connected
                if (client.available())
                {                           // if there's bytes to read from the client,
                    char c = client.read(); // read a byte, then
                    Serial.write(c);        // print it out the serial monitor
                    if (c == '\n')
                    { // if the byte is a newline character

                        // if the current line is blank, you got two newline characters in a row.
                        // that's the end of the client HTTP request, so send a response:
                        if (currentLine.length() == 0)
                        {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println();

                            // the content of the HTTP response follows the header:
                            client.println("</head><body><div align=\"center\"><h2>Relay Ethernet Server Test</h2><br />");
                            client.print("<h3>Click <a href=\"/RELAY_1\">here</a> to change the Relay 1 state.<br></h3>");
                            // The HTTP response ends with another blank line:
                            client.println();
                            // break out of the while loop:
                            break;
                        }
                        else
                        { // if you got a newline, then clear currentLine:
                            currentLine = "";
                        }
                    }
                    else if (c != '\r')
                    {                     // if you got anything else but a carriage return character,
                        currentLine += c; // add it to the end of the currentLine
                    }

                    // Check to see if the client request was "GET /H" or "GET /L":
                    if (currentLine.endsWith("GET /RELAY_1"))
                    {
                        Ethernet_Relay_OP.html_relay1_flag = !Ethernet_Relay_OP.html_relay1_flag;

                        if (Ethernet_Relay_OP.html_relay1_flag == 0)
                        {
                            digitalWrite(RELAY_1, HIGH);
                        }
                        else
                        {
                            digitalWrite(RELAY_1, LOW);
                        }
                    }
                }
            }
            // close the connection:
            client.stop();
            Serial.println("Client Disconnected.");
            // give the web browser time to receive the data
            // delay(500);
        }
    }
}

bool SX1262_Set_Default_Parameters(String *assertion)
{
    if (radio.setFrequency(SX1262_OP.frequency.value) == RADIOLIB_ERR_INVALID_FREQUENCY)
    {
        *assertion = "Failed to set frequency value";
        return false;
    }
    if (radio.setBandwidth(SX1262_OP.bandwidth.value) == RADIOLIB_ERR_INVALID_BANDWIDTH)
    {
        *assertion = "Failed to set bandwidth value";
        return false;
    }
    if (radio.setOutputPower(SX1262_OP.output_power.value) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    {
        *assertion = "Failed to set output_power value";
        return false;
    }
    if (radio.setCurrentLimit(SX1262_OP.current_limit.value) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT)
    {
        *assertion = "Failed to set current_limit value";
        return false;
    }
    if (radio.setPreambleLength(SX1262_OP.preamble_length.value) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
    {
        *assertion = "Failed to set preamble_length value";
        return false;
    }
    if (radio.setCRC(SX1262_OP.crc.value) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION)
    {
        *assertion = "Failed to set crc value";
        return false;
    }
    if (SX1262_OP.current_mode == SX1262_OP.mode::LORA)
    {
        if (radio.setSpreadingFactor(SX1262_OP.spreading_factor.value) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
        {
            *assertion = "Failed to set spreading_factor value";
            return false;
        }
        if (radio.setCodingRate(SX1262_OP.coding_rate.value) == RADIOLIB_ERR_INVALID_CODING_RATE)
        {
            *assertion = "Failed to set coding_rate value";
            return false;
        }
        if (radio.setSyncWord(SX1262_OP.sync_word.value) != RADIOLIB_ERR_NONE)
        {
            *assertion = "Failed to set sync_word value";
            return false;
        }
    }
    else
    {
    }
    return true;
}

bool GFX_Print_SX1262_Info(void)
{
    Serial.println("SX1262 initialization begins");

    SPI.begin(SX1262_SCLK, SX1262_MISO, SX1262_MOSI);

    int16_t state = -1;
    if (SX1262_OP.current_mode == SX1262_OP.mode::LORA)
    {
        state = radio.begin();
    }
    else
    {
        state = radio.beginFSK();
    }

    if (state == RADIOLIB_ERR_NONE)
    {
        String temp_str;
        if (SX1262_Set_Default_Parameters(&temp_str) == false)
        {
            Serial.printf("SX1262 Failed to set default parameters\n");
            Serial.printf("SX1262 assertion: %s\n", temp_str.c_str());
            SX1262_OP.initialization_flag = false;
            return false;
        }
        if (radio.startReceive() != RADIOLIB_ERR_NONE)
        {
            Serial.printf("SX1262 Failed to start receive\n");
            SX1262_OP.initialization_flag = false;
            return false;
        }
    }
    else
    {
        Serial.printf("SX1262 initialization failed\n");
        Serial.printf("Error code: %d\n", state);
        SX1262_OP.initialization_flag = false;
        return false;
    }

    Serial.printf("SX1262 initialization successful\n");
    SX1262_OP.initialization_flag = true;

    return true;
}

void GFX_Print_SX1262_Info_Loop()
{
    if (SX1262_OP.initialization_flag == true)
    {
        if (millis() > CycleTime)
        {
            Serial.println("[Status]: Init successful");
            Serial.printf("[Local MAC]: %012llX\n", Local_MAC);
            if (SX1262_OP.current_mode == SX1262_OP.mode::LORA)
            {
                Serial.println("[Mode]: LoRa");
            }
            else
            {
                Serial.println("[Mode]: FSK");
            }
            Serial.printf("[Frequency]: %.1f MHz\n", SX1262_OP.frequency.value);
            Serial.printf("[Bandwidth]: %.1f KHz\n", SX1262_OP.bandwidth.value);
            Serial.printf("[Output Power]: %d dBm\n", SX1262_OP.output_power.value);

            if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::CONNECTED)
            {
                Serial.println("[Connect]: Connected");
                Serial.printf("[Connecting MAC]: %012llX\n", SX1262_OP.device_1.mac);
                Serial.printf("[Send Data]: %u\n", SX1262_OP.device_1.send_data);

                Serial.printf("[Receive Data]: %u\n", SX1262_OP.device_1.receive_data);
                Serial.printf("[Receive RSSI]: %.1f dBm\n", SX1262_OP.receive_rssi);
                Serial.printf("[Receive SNR]: %.1f dB\n", SX1262_OP.receive_snr);
            }
            else if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::CONNECTING)
            {
                Serial.println("[Connect]: Connecting");
                Serial.printf("[Send Data]: %u\n", SX1262_OP.device_1.send_data);
                Serial.println("[Receive Data]: null");
            }
            else if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::UNCONNECTED)
            {
                Serial.println("[Connect]: Unconnected");
                Serial.println("[Send Data]: null");
                Serial.println("[Receive Data]: null");
            }
            CycleTime = millis() + 5000;
        }

        // if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::CONNECTED)
        // {
        if (SX1262_OP.device_1.send_flag == true)
        {
            if (millis() > CycleTime_2)
            {
                SX1262_OP.device_1.send_flag = false;

                SX1262_OP.send_package[12] = (uint8_t)(SX1262_OP.device_1.send_data >> 24);
                SX1262_OP.send_package[13] = (uint8_t)(SX1262_OP.device_1.send_data >> 16);
                SX1262_OP.send_package[14] = (uint8_t)(SX1262_OP.device_1.send_data >> 8);
                SX1262_OP.send_package[15] = (uint8_t)SX1262_OP.device_1.send_data;

                for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
                {
                    *led[i] = CRGB::Blue;
                }
                FastLED.show();
                delay(1000);
                for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
                {
                    *led[i] = CRGB::Black;
                }
                FastLED.show();

                // send another one
                Serial.println("[SX1262] Sending another packet ... ");

                radio.transmit(SX1262_OP.send_package, 16);
                radio.startReceive();
                SX1262_OP.operation_flag = false;
            }
        }
        // }

        if (SX1262_OP.operation_flag == true)
        {
            SX1262_OP.operation_flag = false;

            uint8_t receive_package[16] = {'\0'};
            if (radio.readData(receive_package, 16) == RADIOLIB_ERR_NONE)
            {

                if ((receive_package[0] == 'M') &&
                    (receive_package[1] == 'A') &&
                    (receive_package[2] == 'C') &&
                    (receive_package[3] == ':'))
                {
                    uint64_t temp_mac =
                        ((uint64_t)receive_package[4] << 56) |
                        ((uint64_t)receive_package[5] << 48) |
                        ((uint64_t)receive_package[6] << 40) |
                        ((uint64_t)receive_package[7] << 32) |
                        ((uint64_t)receive_package[8] << 24) |
                        ((uint64_t)receive_package[9] << 16) |
                        ((uint64_t)receive_package[10] << 8) |
                        (uint64_t)receive_package[11];

                    if (temp_mac != Local_MAC)
                    {
                        SX1262_OP.device_1.mac = temp_mac;
                        SX1262_OP.device_1.receive_data =
                            ((uint32_t)receive_package[12] << 24) |
                            ((uint32_t)receive_package[13] << 16) |
                            ((uint32_t)receive_package[14] << 8) |
                            (uint32_t)receive_package[15];

                        for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
                        {
                            *led[i] = CRGB::Green;
                        }
                        FastLED.show();
                        delay(1000);
                        for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
                        {
                            *led[i] = CRGB::Black;
                        }
                        FastLED.show();

                        // packet was successfully received
                        Serial.printf("[SX1262] Received packet\n");

                        // print data of the packet
                        for (int i = 0; i < 16; i++)
                        {
                            Serial.printf("[SX1262] Data[%d]: %#X\n", i, receive_package[i]);
                        }

                        // print RSSI (Received Signal Strength Indicator)
                        SX1262_OP.receive_rssi = radio.getRSSI();
                        Serial.printf("[SX1262] RSSI: %.1f dBm", SX1262_OP.receive_rssi);

                        // print SNR (Signal-to-Noise Ratio)
                        SX1262_OP.receive_snr = radio.getSNR();
                        Serial.printf("[SX1262] SNR: %.1f dB", SX1262_OP.receive_snr);

                        SX1262_OP.device_1.send_data = SX1262_OP.device_1.receive_data + 1;

                        SX1262_OP.device_1.send_flag = true;
                        SX1262_OP.device_1.connection_flag = SX1262_OP.state::CONNECTED;
                        // 清除错误计数看门狗
                        SX1262_OP.device_1.error_count = 0;
                        CycleTime_2 = millis() + 3000;
                    }
                }
            }
        }
        if (millis() > CycleTime_3)
        {
            SX1262_OP.device_1.error_count++;
            if (SX1262_OP.device_1.error_count > 10) // 10秒超时
            {
                SX1262_OP.device_1.error_count = 11;
                SX1262_OP.device_1.send_data = 0;
                SX1262_OP.device_1.connection_flag = SX1262_OP.state::UNCONNECTED;
            }
            CycleTime_3 = millis() + 1000;
        }
    }
    else
    {
        if (millis() > CycleTime)
        {
            Serial.println("[Status]: Init failed");
            CycleTime = millis() + 1000;
        }
    }
}

void Original_Test_4()
{
    Serial.printf("led set red\n");
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Red;
    }
    FastLED.show();
    delay(1000);
    Serial.printf("led set green\n");
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Green;
    }
    FastLED.show();
    delay(1000);
    Serial.printf("led set blue\n");
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Blue;
    }
    FastLED.show();
    delay(1000);
    Serial.printf("led set white\n");
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::White;
    }
    FastLED.show();
    delay(1000);
    Serial.printf("led set black\n");
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Black;
    }
    FastLED.show();
    delay(1000);

    Serial.printf("led tests complete\n\n");
}

// void Original_Test_4_Loop()
// {
//     // switch (Current_Rotation)
//     // {
//     // case 0:
//     // case 2:
//     //     gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_2, SCREEN_WIDTH, SCREEN_HEIGHT);
//     //     break;
//     // case 1:
//     // case 3:
//     //     gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_4, SCREEN_HEIGHT, SCREEN_WIDTH);
//     //     break;

//     // default:
//     //     break;
//     // }

//     gfx->fillScreen(BLACK);

//     GFX_Print_FINISH();

//     GFX_Print_1();
// }

void Original_Test_5()
{
    RS485_Initialization();
    CAN_Drive_Initialization();

    // RS485_OP.device_1.send_flag = true;
    RS485_OP.device_1.send_data = 0;
    RS485_OP.device_1.connection_status = RS485_OP.state::UNCONNECTED;
    // 清除错误计数看门狗
    RS485_OP.device_1.error.count = 0;

    // RS485_OP.device_2.send_flag = true;
    RS485_OP.device_2.send_data = 0;
    RS485_OP.device_2.connection_status = RS485_OP.state::UNCONNECTED;
    // 清除错误计数看门狗
    RS485_OP.device_2.error.count = 0;

    // CAN_OP.device_1.send_flag = true;
    CAN_OP.device_1.connection_status = CAN_OP.state::UNCONNECTED;
    // 清除错误
    CAN_OP.device_1.error.code = "null";
    // 清除错误计数看门狗
    CAN_OP.device_1.error.count = 0;
    CycleTime_7 = millis() + 1000;
}

void Original_Test_5_Loop()
{
    GFX_Print_RS485CAN_Info();
}

void Original_Test_6()
{
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SX1262_CS, HIGH);

    Ethernet_Initialization();
}

void Original_Test_6_Loop()
{
    GFX_Print_Ethernet_Info();
}

void Original_Test_7()
{
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SX1262_CS, HIGH);

    GFX_Print_SX1262_Info();
}

void Original_Test_8()
{
    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::White;
    }
    FastLED.show();

    Wifi_STA_Test();

    delay(2000);

    if (Wifi_Connection_Flag == true)
    {
        // Obtain and set the time from the network time server
        // After successful acquisition, the chip will use the RTC clock to update the holding time
        configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
        // PrintLocalTime();

        WIFI_HTTP_Download_File();

        for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
        {
            *led[i] = CRGB::Black;
        }
        FastLED.show();

        delay(500);

        for (uint8_t i = 0; i < 3; i++)
        {
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Green;
            }
            FastLED.show();
            delay(500);
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Black;
            }
            FastLED.show();
            delay(500);
        }
    }
    else
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Red;
            }
            FastLED.show();
            delay(500);
            for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
            {
                *led[i] = CRGB::Black;
            }
            FastLED.show();
            delay(500);
        }
    }
}

void Original_Test_Loop()
{
    Serial_Print_TEST("SX1262 callback distance test");
    if (Skip_Current_Test == false)
    {
        Original_Test_7();
        while (1)
        {
            bool temp = false;

            GFX_Print_SX1262_Info_Loop();

            if (digitalRead(KEY_A) == LOW)
            {
                delay(300);
                Serial.printf("KEY_A triggered reinitialize test\n\n");
                Serial_Print_TEST("SX1262 callback distance test");
                Original_Test_7();
                if (Skip_Current_Test == true)
                {
                    temp = true;
                }
            }

            if (digitalRead(ESP_BOOT) == LOW)
            {
                delay(300);
                Serial.printf("ESP_BOOT triggered Start the next test\n\n");
                temp = true;
            }

            if (digitalRead(KEY_B) == LOW)
            {
                SX1262_OP.device_1.send_flag = true;
                SX1262_OP.device_1.connection_flag = SX1262_OP.state::CONNECTING;
                // 清除错误计数看门狗
                SX1262_OP.device_1.error_count = 0;
                CycleTime_2 = millis() + 1000;
            }

            if (temp == true)
            {
                digitalWrite(W5500_CS, HIGH);
                digitalWrite(SX1262_CS, HIGH);

                break;
            }
        }
    }

    Serial_Print_TEST("LED Color Test");
    if (Skip_Current_Test == false)
    {
        Original_Test_4();
        while (1)
        {
            bool temp = false;

            if (digitalRead(KEY_A) == LOW)
            {
                delay(300);
                Serial.printf("KEY_A triggered reinitialize test\n\n");
                Serial_Print_TEST("LCD Color Test");
                Original_Test_4();
                if (Skip_Current_Test == true)
                {
                    temp = true;
                }
            }

            if (digitalRead(ESP_BOOT) == LOW)
            {
                delay(300);
                Serial.printf("ESP_BOOT triggered Start the next test\n\n");
                temp = true;
            }

            if (temp == true)
            {
                break;
            }
        }
    }

    Serial_Print_TEST("WIFI STA Test");
    if (Skip_Current_Test == false)
    {
        Original_Test_8();
        while (1)
        {
            bool temp = false;

            if (digitalRead(KEY_A) == LOW)
            {
                delay(300);
                Serial.printf("KEY_A triggered reinitialize test\n\n");
                Serial_Print_TEST("WIFI STA Test");
                Original_Test_8();
                if (Skip_Current_Test == true)
                {
                    temp = true;
                }
            }

            if (digitalRead(ESP_BOOT) == LOW)
            {
                delay(300);
                Serial.printf("ESP_BOOT triggered Start the next test\n\n");
                temp = true;
            }

            if (temp == true)
            {
                break;
            }
        }
    }

    Serial_Print_TEST("RS485232CAN Test");
    if (Skip_Current_Test == false)
    {
        Original_Test_5();
        Original_Test_5_Loop();
        while (1)
        {
            bool temp = false;

            GFX_Print_RS485CAN_Info_Loop();

            if (digitalRead(KEY_A) == LOW)
            {
                delay(300);
                Serial.printf("KEY_A triggered reinitialize test\n\n");
                Serial_Print_TEST("RS485232CAN Test");
                Original_Test_5();
                Original_Test_5_Loop();
                if (Skip_Current_Test == true)
                {
                    temp = true;
                }
            }

            if (digitalRead(ESP_BOOT) == LOW)
            {
                delay(300);
                Serial.printf("ESP_BOOT triggered Start the next test\n\n");
                temp = true;
            }

            if (digitalRead(KEY_B) == LOW)
            {
                RS485_OP.device_1.send_flag = true;
                RS485_OP.device_1.send_data = 0;
                RS485_OP.device_1.connection_status = RS485_OP.state::CONNECTING;
                // 清除错误计数看门狗
                RS485_OP.device_1.error.count = 0;

                RS485_OP.device_2.send_flag = true;
                RS485_OP.device_2.send_data = 0;
                RS485_OP.device_2.connection_status = RS485_OP.state::CONNECTING;
                // 清除错误计数看门狗
                RS485_OP.device_2.error.count = 0;

                CAN_OP.device_1.send_flag = true;
                CAN_OP.device_1.connection_status = CAN_OP.state::CONNECTING;
                // 清除错误
                CAN_OP.device_1.error.code = "null";
                // 清除错误计数看门狗
                CAN_OP.device_1.error.count = 0;

                CycleTime_2 = millis() + 1000;
                CycleTime_4 = millis() + 1000;
                CycleTime_7 = millis() + 1000;
            }

            if (temp == true)
            {
                break;
            }
        }
    }

    Serial_Print_TEST("Ethernet Relay Test");
    if (Skip_Current_Test == false)
    {
        Original_Test_6();
        Original_Test_6_Loop();
        while (1)
        {
            bool temp = false;

            GFX_Print_Ethernet_Info_Loop();

            if (digitalRead(KEY_A) == LOW)
            {
                delay(300);
                Serial.printf("KEY_A triggered reinitialize test\n\n");
                Serial_Print_TEST("Ethernet Relay Test");
                Original_Test_6();
                Original_Test_6_Loop();
                if (Skip_Current_Test == true)
                {
                    temp = true;
                }
            }

            if (digitalRead(ESP_BOOT) == LOW)
            {
                delay(300);
                Serial.printf("ESP_BOOT triggered Start the next test\n\n");
                temp = true;
            }

            if (digitalRead(KEY_B) == LOW)
            {
                delay(300);
                if (Ethernet_Relay_OP.html_relay1_flag == 0)
                {
                    digitalWrite(RELAY_1, LOW);
                    Ethernet_Relay_OP.html_relay1_flag = 1;
                }
                else
                {
                    digitalWrite(RELAY_1, HIGH);
                    Ethernet_Relay_OP.html_relay1_flag = 0;
                }
            }

            if (temp == true)
            {
                digitalWrite(W5500_CS, HIGH);
                digitalWrite(SX1262_CS, HIGH);
                break;
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");
    Serial.println("[T-Connect-Pro_" + (String)BOARD_VERSION "_No_Screen][" + (String)SOFTWARE_NAME +
                   "]_firmware_" + (String)SOFTWARE_LASTEDITTIME);

    pinMode(W5500_CS, OUTPUT);
    pinMode(SX1262_CS, OUTPUT);

    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SX1262_CS, HIGH);

    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, HIGH);

    // 跳过测试
    pinMode(ESP_BOOT, INPUT_PULLUP);

    // 重新初始化一次当前测试
    pinMode(KEY_A, INPUT_PULLUP);

    // 功能按键
    pinMode(KEY_B, INPUT_PULLUP);

    FastLED.addLeds<WS2812B, WS2812_DATA_1, GRB>(led[0], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_2, GRB>(led[1], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_3, GRB>(led[2], 1);
    FastLED.addLeds<WS2812B, WS2812_DATA_4, GRB>(led[3], 1);

    FastLED.setBrightness(50);

    for (uint8_t i = 0; i < sizeof(led) / sizeof(*led); i++)
    {
        *led[i] = CRGB::Black;
    }
    FastLED.show();

    // 初始化串口，并重新定义引脚
    // 参数包括串行通信的波特率、串行模式、使用的 RX 引脚和 TX 引脚。
    Serial1.begin(115200, SERIAL_8N1, RS485_RX_1, RS485_TX_1);
    Serial2.begin(115200, SERIAL_8N1, RS485_RX_2, RS485_TX_2);

    radio.setDio1Action(Dio1_Action_Interrupt);

    Original_Test_Loop();
}

void loop()
{
    if (millis() > CycleTime)
    {
        GFX_Print_Time_Info_Loop();
        CycleTime = millis() + 1000;
    }
}