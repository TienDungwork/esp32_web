#include "api.h"
#include <string.h>
#include <Preferences.h>
#include "esp_ota_ops.h"


// Hàm mã hóa URL
String urlEncode(String str) {
    String encoded = "";
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char hex[4];
            sprintf(hex, "%%%02X", (uint8_t)c);
            encoded += hex;
        }
    }
    return encoded;
}

// hàm remove escape
void unescapeJson(char* input) {
    char* src = input;
    char* dst = input;
    while (*src) {
        if (src[0] == '\\' && src[1] == '\"') {
            *dst++ = '\"';
            src += 2;
        } else if (src[0] == '\\' && src[1] == '\\') {
            *dst++ = '\\';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}


// So sánh token key trong JSON
int json_get_string_value(const char *json, jsmntok_t *tokens, int token_count,
                          const char *key, char *output, size_t max_len) {
    for (int i = 1; i < token_count; i++) {
        if (jsoneq(json, &tokens[i], key) == 0) {
            size_t len = tokens[i + 1].end - tokens[i + 1].start;
            if (len >= max_len) len = max_len - 1;
            strncpy(output, json + tokens[i + 1].start, len);
            output[len] = '\0';
            return 0; // OK
        }
    }
    return -1; // Not found
}

// Xử lý JSON con trong message cho LED
void handle_led_message(const char *message_json) {
    jsmn_parser parser;
    jsmntok_t tokens[32];

    jsmn_init(&parser);
    int r = jsmn_parse(&parser, message_json, strlen(message_json), tokens, 32);
    if (r < 0) {
        PrintDebug("\r\n[LED] JSON parse failed");
        return;
    }

    char address[4], info[10], status_code[4], status_value[4];
    json_get_string_value(message_json, tokens, r, "address", address, sizeof(address));
    json_get_string_value(message_json, tokens, r, "info", info, sizeof(info));
    json_get_string_value(message_json, tokens, r, "status_code", status_code, sizeof(status_code));
    json_get_string_value(message_json, tokens, r, "status_value", status_value, sizeof(status_value));

    PrintDebug("\r\n[LED] address: " + String(address));
    PrintDebug("\r\n[LED] info: " + String(info));
    PrintDebug("\r\n[LED] status_code: " + String(status_code));
    PrintDebug("\r\n[LED] status_value: " + String(status_value));

    // TODO: Thực hiện hiển thị lên LED thật sự tại đây
}

// Hàm xử lý JSON
void api_process_json(const char *json_data) {
    jsmn_parser parser;
    jsmntok_t tokens[64];
    char code_api_str[5];//Mã code nhận từ server
    int code_api = -1;
    char message_json[128]; //Messenger nhận từ server
    char device_type[16]; //Nhận từ server
    char request_status[16];// Nhận từ server


    jsmn_init(&parser);
    int r = jsmn_parse(&parser, json_data, strlen(json_data), tokens, 64);
    if (r < 0) {
        PrintDebug("\r\n[API] JSON parse failed");
        return;
    }

    if (json_get_string_value(json_data, tokens, r, "Code", code_api_str, sizeof(code_api_str)) != 0) {
        PrintDebug("\r\n[API] Missing Code field");
        return;
    }
    code_api = atoi(code_api_str);  // Chuyển sang số nguyên

    PrintDebug("\r\n[API] Received code: ");
    PrintDebugLn(code_api);

    // Trích xuất DeviceType và Status
    json_get_string_value(json_data, tokens, r, "DeviceType", device_type, sizeof(device_type));
    json_get_string_value(json_data, tokens, r, "Status", request_status, sizeof(request_status));

    // Xử lý theo từng mã code
    switch (code_api) {
        case API_CONNECT:
        tcpConnected = true;
            //handle_connect_request();
            break;

        case API_PING:
            pongReceived = true;
            PrintDebugLn("[TCP] Received pong");
            //handle_ping_request();
            break;

        #if DEVICE_HAS_DISPLAY
        case API_LED: {
            if (json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json)) != 0) {
                PrintDebug("\r\n[API] Message failed");
                return;
            }

            // Bỏ escape trong chuỗi Message
            unescapeJson(message_json);

            jsmn_parser sub_parser;
            jsmntok_t sub_tokens[16];
            jsmn_init(&sub_parser);
            int sub_r = jsmn_parse(&sub_parser, message_json, strlen(message_json), sub_tokens, 16);
            if (sub_r < 0) {
                PrintDebug("\r\n[API] Parse child JSON failed");
                return;
            }

            char license_plate[32] = {0};
            char message_info[64] = {0};

            json_get_string_value(message_json, sub_tokens, sub_r, "LicensePlateNumber", license_plate, sizeof(license_plate));
            json_get_string_value(message_json, sub_tokens, sub_r, "Message", message_info, sizeof(message_info));

            PrintDebug("\r\n[LED] License Plate: ");
            PrintDebug(license_plate);
            PrintDebug("\r\n[LED] Message Info: ");
            PrintDebugLn(message_info);

            handle_display_led("0002", license_plate);
            handle_display_led("0003", message_info);

            break;
        }


        case API_LOADCELL:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_weight(message_json);
            break;

        case API_BARRIER:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_barrier(message_json);
            break;

        case API_TRAFFIC_LIGHT:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_traffic_light(message_json);
            break;

        case API_RFID_READ:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_rfid_read(message_json);
            break;

        case API_RFID_INFO:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_rfid_info(message_json);
            break;

        case API_INVOICE:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_invoice_print(message_json);
            break;

        case API_SENSOR:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_sensor_status(message_json);
            break;

        case API_BEAM:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_beam_status(message_json);
            break;

        case API_QRCODE:
            json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json));
            handle_QRcode_info(message_json);
            break;
        #endif
        #if DEVICE_SPEAK
        case API_SPEECH: {
            if (json_get_string_value(json_data, tokens, r, "Message", message_json, sizeof(message_json)) != 0) {
                PrintDebugLn("\r\n[API] Missing or invalid Message for speech");
                return;
            }


            // Phát file từ SD theo danh sách nhận được
            handle_speech(message_json);
            break;
        }
        #endif
        // case API_UPDATE_FIRMWARE: {
        //     char url_json[128];
        //     json_get_string_value(json_data, tokens, r, "Message", url_json, sizeof(url_json));
        //     // Ví dụ: server gửi "http://192.168.1.123/firmware.bin"

        //     PrintDebugf("[API] OTA request: %s\n", url_json);

        //     // Phân tích chuỗi URL thành host, port, path
        //     String url = url_json;
        //     String host;
        //     String path = "/";
        //     uint16_t port = 80;

        //     if (url.startsWith("http://")) {
        //         url.remove(0, 7); // bỏ "http://"
        //     }
        //     int slash = url.indexOf('/');
        //     if (slash >= 0) {
        //         host = url.substring(0, slash);
        //         path = url.substring(slash);
        //     } else {
        //         host = url;
        //     }
        //     int colon = host.indexOf(':');
        //     if (colon >= 0) {
        //         port = host.substring(colon + 1).toInt();
        //         host = host.substring(0, colon);
        //     }

        //     // Gọi OTA update
        //     http_ota_update_1file(host.c_str(), port, path.c_str());
        //     break;
        // }
        case API_UPDATE_FIRMWARE: {
            char url_json[256];
            json_get_string_value(json_data, tokens, r, "Message", url_json, sizeof(url_json));
            // Ví dụ: server gửi "http://192.168.1.123/firmware.bin"
            PrintDebugf("[API] OTA request: %s\n", url_json);

            // Chỉ cần gọi hàm này, không tải file ở đây nữa
            handle_ota_command(url_json);
            break;
        }



        default:
            PrintDebug("\r\n[API] This code is not defined.");
            break;
    }

}

// Dùng hậu tố <EOF> ở cuối mỗi object JSON điểm phân tách chuỗi
// Ví dụ: {"Code":1,"Message":"2025-07-24 08:59:27","Status":1,"DeviceType":0,"CreatedAt":"2025-07-24T01:59:27.4161328Z","IndexInPacket":0}<EOF>{"Code":1,"Message":"2025-07-24 08:59:27","Status":1,"DeviceType":0,"CreatedAt":"2025-07-24T01:59:27.4193862Z","IndexInPa
// Nếu chỉ dùng api_process_json thì chuỗi trên sẽ lỗi [API] JSON parse failed
void process_incoming_json_stream(const char *data) {
    const char *start = data;
    const char *end;

    while ((end = strstr(start, "<EOF>")) != NULL) {
        char json_chunk[512];  // Đảm bảo đủ lớn để chứa từng JSON
        int len = end - start;
        if (len >= sizeof(json_chunk)) len = sizeof(json_chunk) - 1;
        strncpy(json_chunk, start, len);
        json_chunk[len] = '\0';

        // Gọi xử lý từng JSON object
        api_process_json(json_chunk);

        start = end + 5; // bỏ qua "<EOF>"
    }
}


void handle_connect_request() {
    PrintDebug("\r\n[API] Connect request");

    // Gửi phản hồi
    SendAllDeviceConnectStates();
}

void handle_ping_request() {
    PrintDebug("\r\n[API] Ping request");

    // Gửi phản hồi dạng pong (code = 2) 
    BuildJson(API_PING, "", 0);
    PrintDebugLn("\r\n[API] Sent pong for ping");
#if DEVICE_SPEAK
    digitalWrite(1, HIGH); //Đèn báo loa
#endif
}

#if DEVICE_SPEAK
void handle_speech(const char *message) {
    // Log chuỗi nhận được từ server
    PrintDebug("\r\n[SPEECH] Received track list: ");
    PrintDebugLn(message);
    String cmd = String(message);
    playTracks(cmd);
}
#endif

void handle_display_led(const char* address, const char* info) {
    String endpoint = String("/data?") + String(address) + "=" + urlEncode(String(info));
    sendHttpRequest(endpoint); // Gửi chuỗi mã hóa URL
    PrintDebug("\r\n[API] Display LED: address=");
    PrintDebug(address);
    PrintDebug(", info=");
    PrintDebugLn(info);
}

void handle_weight(const char* message) {
    double weight = atof(message);
    PrintDebug("\r\n[API] Trọng lượng: ");
    PrintDebugLn(String(weight, 2));
}


void handle_barrier(const char *message_json) {
  PrintDebug("Barrier command received: ");
  PrintDebugLn(message_json);

  if (strcmp(message_json, "1") == 0) { 
    PrintDebugLn("Opening barrier...");
    barrier_control(BARRIER_OPEN);
  } else if (strcmp(message_json, "2") == 0) {
    PrintDebugLn("Closing barrier...");
    barrier_control(BARRIER_CLOSE);
  } else if (strcmp(message_json, "PAUSE") == 0) {
    PrintDebugLn("Pausing barrier...");
    barrier_control(BARRIER_PAUSE);
  } else {
    PrintDebugLn("Unknown barrier command!");
  }
}


void handle_traffic_light(const char *message_json) {
  PrintDebug("Traffic light command received: ");
  PrintDebugLn(message_json);

  if (strcmp(message_json, "0") == 0) {
    PrintDebugLn("Turning traffic light OFF...");
    traffic_light_control(TRAFFIC_OFF);
  } else if (strcmp(message_json, "1") == 0) {
    PrintDebugLn("Turning traffic light GREEN...");
    traffic_light_control(TRAFFIC_GREEN);
  } else if (strcmp(message_json, "3") == 0) {
    PrintDebugLn("Turning traffic light RED...");
    traffic_light_control(TRAFFIC_RED);
  } else if (strcmp(message_json, "2") == 0) {
    PrintDebugLn("Turning traffic light YELLOW...");
    traffic_light_control(TRAFFIC_YELLOW);
  } else if (strcmp(message_json, "RED_FLASH") == 0) {
    PrintDebugLn("Turning traffic light RED FLASH...");
    traffic_light_control(TRAFFIC_RED_FLASH);
  } else {
    PrintDebugLn("Unknown traffic light command!");
  }
}

//
void handle_rfid_read(const char* message) {
    PrintDebug("\r\n[API] RFID read: ");
    PrintDebugLn(message);
}

void handle_rfid_info(const char* message) {
    PrintDebug("\r\n[API] RFID info: ");
    PrintDebugLn(message);
}

void handle_invoice_print(const char* message) {
    PrintDebug("\r\n[API] Invoice print: ");
    PrintDebugLn(message);

    
    PrintDebugLn("Sending print job...");

    // In văn bản tiếng Việt
    printer_set_alignment(1); // căn giữa
    printer_set_bold(true);
    printer_println(message);
    printer_set_bold(false);
    printer_new_line(1);

}

void handle_sensor_status(const char* message) {
    PrintDebug("\r\n[API] Sensor status: ");
    PrintDebugLn(message);
}

void handle_beam_status(const char* message) {
    PrintDebug("\r\n[API] Beam: ");
    PrintDebugLn(message);

}

void handle_QRcode_info(const char* message) {
    PrintDebug("\r\n[API] QR Code: ");
    PrintDebugLn(message);

    /* printer_set_alignment(1); // căn giữa
    printer_println("Quet ma de thanh toan");

    printer_print_qrcode(
        message,
        5,  // phiên bản QR
        2,  // mức sửa lỗi M
        4   // zoom phóng đại
    );

    printer_new_line(3); // thêm dòng trắng */

}

void handle_ota_command(const char *url_json) {
    Preferences pref;
    pref.begin("ota_req", false);
    pref.putString("url", url_json);
    pref.putBool("ota_request", true);
    pref.end();

    // tìm partition agent
    const esp_partition_t* agent = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                                           ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                                           "ota_agent");
    if (agent) {
        esp_ota_set_boot_partition(agent); // set boot vào agent
    }

    ESP.restart(); // reboot sang agent
}