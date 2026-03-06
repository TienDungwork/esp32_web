#include <Arduino.h>
#include "pin_config.h"


HardwareSerial SerialPort1(1); // UART2
char DATA[256] = {0};
String rs232_input = "";
uint32_t temp = 0;
uint32_t CycleTime = 0;

void setup()
{
    Serial.begin(9600);
    SerialPort1.begin(9600, SERIAL_8N1, RS485_RX_1, RS485_TX_1);

    Serial.println("RS232 Initialized");
}

String receiveRS232Data() {
    String receivedData = "";
    while (SerialPort1.available()) {
        char c = SerialPort1.read();
        if (c == '=') {
            Serial.print("Received: ");
            Serial.println(receivedData);
            return receivedData;
        } else {
            receivedData += c;
        }
    }
    return receivedData; // Trả về chuỗi rỗng nếu không có dữ liệu mới
}

void loop() {
    // Gửi chuỗi mỗi 3 giây
    if (millis() > CycleTime) {
        temp++;
        String toSend = String(temp) + "\n";
        SerialPort1.print(toSend);
        SerialPort1.print("Hoang Tuan Anh\n");

        Serial.print("Sent: ");
        Serial.print(toSend);
        Serial.print("Hoang Tuan Anh\n");

        SerialPort1.flush();
        CycleTime = millis() + 3000;
    }

    // Nhận dữ liệu
    while (SerialPort1.available() > 0)
    {
        memset(DATA, '\0', sizeof(DATA));
        SerialPort1.read(DATA, sizeof(DATA));

        if (strlen(DATA) != 0)
        {
            SerialPort1.printf("%s\n", DATA);
            Serial.printf("rx: %s\n", DATA);
        }
    }



    // Nhận dữ liệu
/*         String data = receiveRS232Data();
        if (data != "") {
            // Xử lý thêm nếu cần
        } */


}