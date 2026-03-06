#include "uart.h"

// Biến static để lưu dữ liệu nhận được
static String lastRS485Data = "";
static String lastRS232Data = "";

// Getter cho dữ liệu RS485
String GetLastRS485Data() {
    return lastRS485Data;
}

//Loadcell
#define MAX_VALID_SAMPLES 10
String validSamples[MAX_VALID_SAMPLES];
int validCount = 0;
bool capturing = false;




void UART_Initialization(void)
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

/******************************************************************************/
/*                      QR CODE                                */
/******************************************************************************/
// Nhận dữ liệu từ máy quét QR Code
void Handle_RS485_Data() {
    while (Serial1.available()) {

        String received = Serial1.readStringUntil('\n');
        if (received.length() > 0) {
            received.trim();

            PrintDebugf("%s\n", received.c_str());
            //PrintDebugf("[RS485] Received data: %s\n", received.c_str());

            lastRS485Data = received;  // Lưu lại dữ liệu
            // Chuyển dữ liệu thành sự kiện QR code
            EventQueue_Push(EVENT_QRCODE_SCANNED, lastRS485Data.c_str());
            
        }
    }
}


/******************************************************************************/
/*                      LOAD CELL                               */
/******************************************************************************/
#define SAMPLE_COUNT 10

String rs232_input = "";
float samples[SAMPLE_COUNT];
int sampleIndex = 0;
bool collecting = false;
float last_sent_value = -1.0f; // Giá trị cuối cùng đã gửi lên server để biết "cân rỗng"

// Hàm tìm giá trị xuất hiện nhiều nhất
float findMostFrequent(float arr[], int n) {
    int maxCount = 0;
    float mostFrequent = arr[0];

    for (int i = 0; i < n; i++) {
        int count = 1;
        for (int j = i + 1; j < n; j++) {
            if (fabs(arr[i] - arr[j]) < 0.0001f) {
                count++;
            }
        }

        if (count > maxCount) {
            maxCount = count;
            mostFrequent = arr[i];
        }
    }

    return mostFrequent;
}

// Hàm xử lý dữ liệu RS232

// Không gửi giá trị "0" khi cân xog
/* void Handle_RS232_Data() {
    while (Serial2.available()) {
        char c = Serial2.read();
        rs232_input += c;

        if (c == '=') {
            String valueStr = rs232_input.substring(0, rs232_input.length() - 1);
            float value = valueStr.toFloat();
            rs232_input = "";

            // Bỏ qua 0.00000
            if (value == 0.0f) {
                collecting = false;
                sampleIndex = 0;
                continue;
            }

            // Bắt đầu thu thập nếu chưa
            if (!collecting) {
                collecting = true;
                sampleIndex = 0;
            }

            if (sampleIndex < SAMPLE_COUNT) {
                samples[sampleIndex++] = value;
            }

            // Đã đủ 10 mẫu
            if (sampleIndex == SAMPLE_COUNT) {
                float mostFrequent = findMostFrequent(samples, SAMPLE_COUNT);

                // Chuyển float thành chuỗi để gửi vào EventQueue
                char buffer[20];
                snprintf(buffer, sizeof(buffer), "%.5f", mostFrequent);

                // Gửi vào EventQueue
                EventQueue_Push(EVENT_LOADCELL_CHANGED, buffer);

                // Reset
                collecting = false;
                sampleIndex = 0;
            }
        }
    }
} */

// gửi giá trị "0"
void Handle_RS232_Data() {
    while (Serial2.available()) {
        char c = Serial2.read();
        rs232_input += c;

        if (c == '=') {
            String valueStr = rs232_input.substring(0, rs232_input.length() - 1);
            float value = valueStr.toFloat();
            rs232_input = "";

            // ======= Trường hợp bỏ đồ ra cân (0.00000) =======
            if (value == 0.0f) {
                // Nếu lần trước đã gửi != 0 thì giờ mới gửi 0 để tránh gửi dư
                if (last_sent_value != 0.0f) {
                    char buffer[20];
                    snprintf(buffer, sizeof(buffer), "%.5f", 0.0f);
                    EventQueue_Push(EVENT_LOADCELL_CHANGED, buffer);
                    last_sent_value = 0.0f;
                    PrintDebugLn("[RS232] Send data 0.00000\r\n"); //do không còn vật trên cân
                }

                collecting = false;
                sampleIndex = 0;
                return;
            }

            // ======= Bắt đầu thu thập mẫu nếu có giá trị khác 0 =======
            if (!collecting) {
                collecting = true;
                sampleIndex = 0;
            }

            if (sampleIndex < SAMPLE_COUNT) {
                samples[sampleIndex++] = value;
            }

            // ======= Đã thu đủ SAMPLE_COUNT mẫu =======
            if (sampleIndex == SAMPLE_COUNT) {
                float mostFrequent = findMostFrequent(samples, SAMPLE_COUNT);

                // Gửi nếu giá trị mới khác với lần gửi trước
                if (fabs(mostFrequent - last_sent_value) > 0.0001f) {
                    char buffer[20];
                    snprintf(buffer, sizeof(buffer), "%.5f", mostFrequent);
                    EventQueue_Push(EVENT_LOADCELL_CHANGED, buffer);
                    last_sent_value = mostFrequent;
                    PrintDebugf("[RS232] Send data %.5f\r\n", mostFrequent);
                }

                collecting = false;
                sampleIndex = 0;
            }
        }
    }
}
/******************************************************************************/
/*                      UART                                */
/******************************************************************************/
void GFX_Print_UART_Info_Loop(void){
    Handle_RS232_Data();
    Handle_RS485_Data();
}

