#ifndef ETHERNET_TCP_H
#define ETHERNET_TCP_H

#include "driver_config.h"
#include "string.h"
#include <Arduino.h>
#include <Preferences.h>
#include <EthernetUdp.h>
#include <Update.h>
#include <stdarg.h>
#include <stdio.h>

#define UDP_LISTEN_PORT     12345
#define UDP_CLIENT_PORT     5000
#define UDP_BUFFER_SIZE     512

// Khai báo biến toàn cục
extern EthernetClient tcpClient;
extern EthernetClient httpClient;
extern IPAddress serverIP;
extern int serverPort; // Thay đổi từ const int thành int để cho phép cập nhật động
extern const char* LED_SERVER_IP;
extern const int LED_SERVER_PORT;
extern bool tcpConnected;


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

#define RECEIVE_BUF_SIZE 256
#define EVENT_QUEUE_SIZE 10

typedef enum {
    EVENT_NONE,
    EVENT_TCP_DATA_RECEIVED,
    EVENT_TCP_DISCONNECTED,
    EVENT_TCP_PING_TIMEOUT,
    EVENT_TCP_CONNECTED,
    EVENT_TCP_SEND_PING,
    EVENT_QRCODE_SCANNED,
    EVENT_IR_BEAM_BROKEN,
    EVENT_INDUCTIVE_LOOP_TRIGGERED,
    EVENT_RFID_CARD_ISSUED,
    EVENT_RFID_CARD_SCANNED,
    EVENT_LOADCELL_CHANGED
    // thêm nếu cần
} AppEvent;

typedef struct {
    AppEvent type;
    char data[RECEIVE_BUF_SIZE];
} Event;

extern Ethernet_Relay_Operator Ethernet_Relay_OP;
extern bool pongReceived;

// Prototype hàm
void Ethernet_Reset(const uint8_t resetPin);
bool Ethernet_Initialization_Assertion(String *assertion);
void Ethernet_Initialization(void);
void sendHttpRequest(String path);

// Hàm UDP
void udp_broadcast_init(void);
int udp_receive(uint8_t *buf, uint16_t buf_size, IPAddress *src_ip, uint16_t *src_port);
void udp_handle_packet(uint8_t *buf, int len, IPAddress src_ip, uint16_t src_port);
void udp_process(void);
void udp_send_response(IPAddress dst_ip, uint16_t dst_port, const char *msg);
bool load_server_config(IPAddress &ip, uint16_t &port);

bool EventQueue_Push(AppEvent type, const char* data);
bool EventQueue_PushOverwrite(AppEvent type, const char* data);
bool EventQueue_Pop(Event* e);
void HandleEvent(Event* e);

void GFX_Print_Ethernet_Info(void);                            // Hiển thị thông tin Ethernet
void GFX_Print_Ethernet_Info_Loop(void);                       // Cập nhật thông tin Ethernet
void GFX_Update_Ethernet_Status();

void TCP_Client_CheckConnection();
void TCP_Client_HandleReceive();
void TCP_Client_HandlePing();
String BuildJson(int code, String message, int deviceType);
void SendAllDeviceConnectStates();
void ProcessEven(void);

#endif  // ETHERNET_TCP_H