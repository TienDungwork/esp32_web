/*
 * @Description: GFX SX1262 test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:51:57
 * @License: GPL 3.0
 */

#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "RadioLib.h"
#include "pin_config.h"
#include "Arduino_DriveBus_Library.h"

static const uint64_t Local_MAC = ESP.getEfuseMac();

static size_t CycleTime = 0;
static size_t CycleTime_2 = 0;
static size_t CycleTime_3 = 0;

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
        float value = 868.6;
        bool change_flag = false;
    } frequency;
    struct
    {
        float value = 125.0;
        bool change_flag = false;
    } bandwidth;
    struct
    {
        uint8_t value = 9;
        bool change_flag = false;
    } spreading_factor;
    struct
    {
        uint8_t value = 6;
        bool change_flag = false;
    } coding_rate;
    struct
    {
        uint8_t value = 0xAB;
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
        bool value = false;
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

SX1262_Operator SX1262_OP;

SX1262 radio = new Module(SX1262_CS, SX1262_DIO1, SX1262_RST, SX1262_BUSY, SPI);

Arduino_DataBus *bus = new Arduino_HWSPI(
    SCREEN_DC /* DC */, SCREEN_CS /* CS */, SCREEN_SCLK /* SCK */,
    SCREEN_MOSI /* MOSI */, SCREEN_MISO /* MISO */);

Arduino_GFX *gfx = new Arduino_ST7796(
    bus, SCREEN_RST /* RST */, 0 /* rotation */, true /* IPS */,
    SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */,
    49 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> CST226SE(new Arduino_CST2xxSE(IIC_Bus, CST226SE_DEVICE_ADDRESS,
                                                           TOUCH_RST, TOUCH_INT, Arduino_IIC_Touch_Interrupt));

void Arduino_IIC_Touch_Interrupt(void)
{
    CST226SE->IIC_Interrupt_Flag = true;
}

void Dio1_Action_Interrupt(void)
{
    // we sent or received a packet, set the flag
    SX1262_OP.operation_flag = true;
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

void GFX_Print_1()
{
    gfx->fillRect(20, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, ORANGE);
    gfx->drawRect(20, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, PURPLE);
    gfx->fillRect(120, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, PURPLE);
    gfx->drawRect(120, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, ORANGE);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(35, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 15);
    gfx->printf("Try Again");
    gfx->setCursor(135, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 15);
    gfx->printf("Next Test");
}

void GFX_Print_TEST(String s)
{
    gfx->fillScreen(WHITE);
    gfx->setCursor(SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4);
    gfx->setTextSize(3);
    gfx->setTextColor(PALERED);
    gfx->printf("TEST");

    gfx->setCursor(20, SCREEN_HEIGHT / 4 + 50);
    gfx->setTextSize(1);
    gfx->setTextColor(BLACK);
    gfx->print(s);

    gfx->setCursor(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2);
    gfx->setTextSize(4);
    gfx->setTextColor(RED);
    gfx->printf("3");
    delay(1000);
    gfx->fillRect(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 + 40, SCREEN_HEIGHT / 2 + 40, WHITE);
    gfx->setCursor(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2);
    gfx->printf("2");
    delay(1000);
    gfx->fillRect(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 + 40, SCREEN_HEIGHT / 2 + 40, WHITE);
    gfx->setCursor(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2);
    gfx->printf("1");
    delay(1000);
}

void GFX_Print_SX1262_Connect_Button()
{
    gfx->fillRect(SCREEN_WIDTH / 4, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) - 30, SCREEN_WIDTH / 2, 50, PALERED);
    gfx->drawRect(SCREEN_WIDTH / 4, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) - 30, SCREEN_WIDTH / 2, 50, RED);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);

    gfx->setCursor(SCREEN_WIDTH / 4 + 30, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) - 40 + 30);
    gfx->printf("Reconnect");
}

bool GFX_Print_SX1262_Info(void)
{
    gfx->fillScreen(WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(PURPLE);
    gfx->setCursor(40, 30);
    gfx->printf("SX1262 Info");

    gfx->setTextSize(1);
    gfx->setTextColor(BLACK);
    gfx->setCursor(20, 110);
    gfx->printf("[Local MAC]: %012llX", Local_MAC);

    gfx->setTextColor(ORANGE);
    gfx->setCursor(17, 160);
    gfx->printf("<----------Send Info----------> ");

    gfx->setTextColor(ORANGE);
    gfx->setCursor(15, 190);
    gfx->printf("<---------Receive Info---------> ");

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
            gfx->fillRect(0, 60, SCREEN_WIDTH, 40, WHITE);
            gfx->setTextSize(1);
            gfx->setTextColor(DARKGREEN);
            gfx->setCursor(20, 60);
            gfx->printf("[Status]: Init successful");
            gfx->setTextColor(BLACK);
            gfx->setCursor(20, 70);
            if (SX1262_OP.current_mode == SX1262_OP.mode::LORA)
            {
                gfx->printf("[Mode]: LoRa");
            }
            else
            {
                gfx->printf("[Mode]: FSK");
            }
            gfx->setTextColor(BLACK);
            gfx->setCursor(20, 80);
            gfx->printf("[Frequency]: %.1f MHz", SX1262_OP.frequency.value);

            gfx->setCursor(20, 90);
            gfx->printf("[Bandwidth]: %.1f KHz", SX1262_OP.bandwidth.value);

            gfx->setCursor(20, 100);
            gfx->printf("[Output Power]: %d dBm", SX1262_OP.output_power.value);

            gfx->fillRect(0, 130, SCREEN_WIDTH, 30, WHITE);
            gfx->setCursor(20, 130);
            if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::CONNECTED)
            {
                gfx->setTextColor(DARKGREEN);
                gfx->printf("[Connect]: Connected");
                gfx->setCursor(20, 140);
                gfx->printf("[Connecting MAC]: %012llX", SX1262_OP.device_1.mac);

                gfx->fillRect(0, 170, SCREEN_WIDTH, 20, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 170);
                gfx->printf("[Send Data]: %u", SX1262_OP.device_1.send_data);

                gfx->fillRect(0, 200, SCREEN_WIDTH, 40, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 200);
                gfx->printf("[Receive Data]: %u", SX1262_OP.device_1.receive_data);
                gfx->setCursor(20, 210);
                gfx->printf("[Receive RSSI]: %.1f dBm", SX1262_OP.receive_rssi);
                gfx->setCursor(20, 220);
                gfx->printf("[Receive SNR]: %.1f dB", SX1262_OP.receive_snr);
            }
            else if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::CONNECTING)
            {
                gfx->setTextColor(BLUE);
                gfx->printf("[Connect]: Connecting");

                gfx->fillRect(0, 170, SCREEN_WIDTH, 20, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 170);
                gfx->printf("[Send Data]: %u", SX1262_OP.device_1.send_data);

                gfx->fillRect(0, 200, SCREEN_WIDTH, 40, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 200);
                gfx->printf("[Receive Data]: null");
            }
            else if (SX1262_OP.device_1.connection_flag == SX1262_OP.state::UNCONNECTED)
            {
                gfx->setTextColor(RED);
                gfx->printf("[Connect]: Unconnected");

                gfx->fillRect(0, 170, SCREEN_WIDTH, 20, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 170);
                gfx->printf("[Send Data]: null");

                gfx->fillRect(0, 200, SCREEN_WIDTH, 40, WHITE);
                gfx->setTextColor(BLACK);
                gfx->setCursor(20, 200);
                gfx->printf("[Receive Data]: null");
            }
            CycleTime = millis() + 500;
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

                // send another one
                Serial.println("[SX1262] Sending another packet ... ");

                radio.transmit(SX1262_OP.send_package, 16);
                radio.startReceive();
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
                        CycleTime_2 = millis() + 500;
                    }
                }
            }
        }
        if (millis() > CycleTime_3)
        {
            SX1262_OP.device_1.error_count++;
            if (SX1262_OP.device_1.error_count > 5)
            {
                SX1262_OP.device_1.error_count = 6;
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
            gfx->fillRect(0, 60, SCREEN_WIDTH, 40, WHITE);
            gfx->setTextSize(1);
            gfx->setTextColor(RED);
            gfx->setCursor(20, 60);
            gfx->printf("[Status]: Init failed");
            CycleTime = millis() + 1000;
        }
    }
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    ledcAttachPin(SCREEN_BL, 1);
    ledcSetup(1, 2000, 8);
    ledcWrite(1, 255);

    if (CST226SE->begin() == false)
    {
        Serial.println("CST226SE initialization fail");
        delay(2000);
    }
    else
    {
        Serial.println("CST226SE initialization successfully");
        Serial.printf("ID: %#X \n\n", (int32_t)CST226SE->IIC_Device_ID());
    }

    radio.setDio1Action(Dio1_Action_Interrupt);

    gfx->begin();
    gfx->fillScreen(WHITE);

    GFX_Print_TEST("SX1262 callback distance test");
    GFX_Print_SX1262_Info();
    GFX_Print_SX1262_Connect_Button();
    GFX_Print_1();
}

void loop()
{
    if (CST226SE->IIC_Interrupt_Flag == true)
    {
        delay(100);
        CST226SE->IIC_Interrupt_Flag = false;

        int32_t touch_x = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t touch_y = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
        uint8_t fingers_number = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);

        if (fingers_number > 0)
        {
            if (touch_x > SCREEN_WIDTH / 4 && touch_x < SCREEN_WIDTH / 4 + SCREEN_WIDTH / 2 &&
                touch_y > SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) - 30 && touch_y < SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) - 30 + 50)
            {
                SX1262_OP.device_1.send_flag = true;
                SX1262_OP.device_1.connection_flag = SX1262_OP.state::CONNECTING;
                // 清除错误计数看门狗
                SX1262_OP.device_1.error_count = 0;
                CycleTime_2 = millis() + 1000;
            }
        }
    }

    GFX_Print_SX1262_Info_Loop();
}