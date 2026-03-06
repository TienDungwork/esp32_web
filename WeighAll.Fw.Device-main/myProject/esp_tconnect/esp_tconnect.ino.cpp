# 1 "C:\\Users\\Emic\\AppData\\Local\\Temp\\tmppcbszr5b"
#include <Arduino.h>
# 1 "D:/ME/Project/WeighAll.Fw.Device/myProject/esp_tconnect/esp_tconnect.ino"
#include "driver_config.h"
#include "TFT/Material_16Bit_222x480px.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#define SOFTWARE_NAME "Original_Test"
#define SOFTWARE_LASTEDITTIME "202504261030"
#define BOARD_VERSION "V1.0"


#define LOOP_DELAY 50
#define TOUCH_READ_INTERVAL 100
void cb_tcp_check_connection(void);
void cb_tcp_handle_receive(void);
void cb_tcp_handle_ping(void);
void cb_udp_broadcast(void);
void cb_ap_CaptiveTasks(void);
void cb_process_even(void);
void cb_printer(void);
void UART_Init();
void Ethernet_Init();
void Ethernet_Display();
void Main_Loop();
void printPartitions();
void setup();
void loop();
#line 16 "D:/ME/Project/WeighAll.Fw.Device/myProject/esp_tconnect/esp_tconnect.ino"
void cb_tcp_check_connection(void) {
    TCP_Client_CheckConnection();

}

void cb_tcp_handle_receive(void) {
    TCP_Client_HandleReceive();

}

void cb_tcp_handle_ping(void) {
    TCP_Client_HandlePing();

}

void cb_udp_broadcast(void) {
    udp_process();
}

void cb_ap_CaptiveTasks(void){
    handleCaptiveTasks();
}

void cb_process_even(void){
    ProcessEven();
}

void cb_printer(void){
    printer_set_alignment(1);
    printer_set_bold(true);
    printer_println("Chao ban!");
    printer_set_bold(false);
    printer_new_line(1);
}

void UART_Init()
{
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SCREEN_CS, LOW);
    UART_Initialization();
    digitalWrite(SCREEN_CS, HIGH);
}



void Ethernet_Init()
{
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SCREEN_CS, HIGH);
    Ethernet_Initialization();
}

void Ethernet_Display()
{
    digitalWrite(SCREEN_CS, LOW);
    gfx->fillScreen(WHITE);
    GFX_Print_Ethernet_Info();
    GFX_Print_1();
    digitalWrite(SCREEN_CS, HIGH);
}


void Main_Loop()
{

    unsigned long lastTouchRead = 0;


    Ethernet_Init();
    UART_Init();

    Ethernet_Display();

    udp_broadcast_init();




    while (1)
    {
        if (Skip_Current_Test == true)
        {
            digitalWrite(W5500_CS, HIGH);
            digitalWrite(SCREEN_CS, HIGH);
            break;
        }


        GFX_Print_Ethernet_Info_Loop();
        GFX_Print_UART_Info_Loop();
        Handle_Read_Beam();


        if (millis() - lastTouchRead >= TOUCH_READ_INTERVAL)
        {
            lastTouchRead = millis();
            uint8_t fingers_number = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);

            if (fingers_number > 0)
            {
                int32_t touch_x = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
                int32_t touch_y = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
                Touch_Rotation_Convert(&touch_x, &touch_y);


                if (GFX_Print_1_Trigger(touch_x, touch_y) == 1)
                {
                    digitalWrite(SCREEN_CS, LOW);
                    Ethernet_Init();
                    Ethernet_Display();
                    digitalWrite(SCREEN_CS, HIGH);
                    if (Skip_Current_Test == true)
                    {
                        return;
                    }
                }
            }
        }

        delay(LOOP_DELAY);
    }
}
# 146 "D:/ME/Project/WeighAll.Fw.Device/myProject/esp_tconnect/esp_tconnect.ino"
void printPartitions() {
  Serial.println("=== Partition Table ===");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP,
                                                   ESP_PARTITION_SUBTYPE_ANY,
                                                   NULL);
  while (it != NULL) {
    const esp_partition_t *part = esp_partition_get(it);
    Serial.printf("Name:%s Type:%d SubType:%d Addr:0x%06X Size:%dKB\n",
                  part->label, part->type, part->subtype,
                  part->address, part->size/1024);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);

  const esp_partition_t *running = esp_ota_get_running_partition();
  Serial.printf("Currently running from: %s\n", running->label);
}


void setup()
{
    Serial.begin(9600);
    PrintDebugLn("Ciallo");
    PrintDebugLn("[T-Connect-Pro_" + (String)BOARD_VERSION "][" + (String)SOFTWARE_NAME +
                   "]_firmware_" + (String)SOFTWARE_LASTEDITTIME);

    pinMode(W5500_CS, OUTPUT);
    pinMode(SCREEN_CS, OUTPUT);
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SCREEN_CS, HIGH);

    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, HIGH);

    ledcAttachPin(SCREEN_BL, 1);
    ledcSetup(1, 2000, 8);
    ledcWrite(1, 255);

    Serial2.begin(9600, SERIAL_8N1, RS232_RX_1, RS232_TX_1);
    Serial1.begin(9600, SERIAL_8N1, RS485_RX_2, RS485_TX_2);
    if (CST226SE->begin() == false)
    {
        PrintDebugLn("CST226SE initialization fail");
        delay(2000);
    }
    else
    {
        PrintDebugLn("CST226SE initialization successfully");
    }

    Serial.println("=== MAIN APP START v1.0 ===");

    gfx->begin();
    gfx->fillScreen(WHITE);
    gfx->setTextSize(1);
    gfx->fillScreen(PALERED);

    gpio_init();
    printer_init();
    printPartitions();


    soft_timer_init();
    soft_timer_start(1, true, cb_tcp_check_connection);
    soft_timer_start(30, true, cb_tcp_handle_receive);
    soft_timer_start(500, true, cb_tcp_handle_ping);
    soft_timer_start(500, true, cb_udp_broadcast);

    soft_timer_start(100, true, cb_process_even);
#if ENABLE_WIFI_AP
    soft_timer_start(500, true, cb_ap_CaptiveTasks);
#endif

    IPAddress saved_ip;
    uint16_t saved_port;
    if (load_server_config(saved_ip, saved_port)) {
        serverIP = saved_ip;
        serverPort = saved_port;


        if (tcpClient.connect(serverIP, serverPort)) {
            PrintDebugLn("[TCP] Connected to saved server config.");
            EventQueue_Push(EVENT_TCP_CONNECTED, NULL);
        } else {
            PrintDebugLn("[TCP] Failed to connect to saved server.");
            EventQueue_Push(EVENT_TCP_DISCONNECTED, NULL);
        }
    }


}

void loop()
{
    Main_Loop();

}