#ifndef PARKING_API_H
#define PARKING_API_H

#include "driver_config.h"
#include "JSMN/jsmn.h"


// Enum các mã API
enum ApiCode {
    API_CONNECT         = 1,
    API_PING            = 2,
    API_SPEECH          = 101,
    API_LED             = 201,
    API_LOADCELL        = 301,
    API_BARRIER         = 401,
    API_TRAFFIC_LIGHT   = 501,
    API_RFID_READ       = 601,
    API_RFID_INFO       = 701,
    API_INVOICE         = 801,
    API_SENSOR          = 901,
    API_BEAM            = 1001,
    API_QRCODE          = 1101,

    API_UPDATE_FIRMWARE = 2101
};

enum StationDeviceType {
    // Thiết bị cổng vào
    QrCodeReaderEnter = 1,        // Đầu đọc QR
    // LoopSensorEnter = 2,
    BarrierEnter = 3,
    LedScreenEnter = 4,
    TrafficLightEnter = 5,
    InfraredGridEnter = 6,
    PrinterEnter = 7,
    CardDispenserEnter = 8,
    CardReaderEnter = 9,


    // Thiết bị cổng ra
    QrCodeReaderExit = 51,
    // LoopSensorExit = 52,
    BarrierExit = 53,
    LedScreenExit = 54,
    TrafficLightExit = 55,
    InfraredGridExit = 56,
    PrinterExit = 57,
    CardDispenserExit = 58,
    CardReaderExit = 59,

    // Thiết bị chung
    LoadCell = 101,
    Speaker = 102,

};


void api_process_json(const char *json_data);
void process_incoming_json_stream(const char *data);
int json_get_string_value(const char *json, jsmntok_t *tokens, int token_count, const char *key, char *output, size_t max_len);
void handle_connect_request();
void handle_ping_request();
void handle_speech(const char* message);
void handle_display_led(const char* address, const char* info);
void handle_weight(const char* message);
void handle_barrier(const char* message);
void handle_traffic_light(const char* message);
void handle_rfid_read(const char* message);
void handle_rfid_info(const char* message);
void handle_invoice_print(const char* message);
void handle_sensor_status(const char* message);
void handle_beam_status(const char* message);
void handle_QRcode_info(const char* message);
void handle_ota_command(const char *url_json);

#endif
