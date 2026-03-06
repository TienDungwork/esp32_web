#include <HardwareSerial.h>
#include "PRINTER/printer.h"

// UART2 (TX: GPIO17, RX: GPIO16)
HardwareSerial PrinterSerial(2);

// Chân BUSY (nếu máy in hỗ trợ), HIGH = busy
#define BUSY_PIN 5

// Đừng khai báo lại với extern "C" ở đây nữa!
void printer_send_data(const uint8_t *data, size_t len) {
  PrinterSerial.write(data, len);
}


void setup() {
  Serial.begin(9600); // UART0 debug
  PrinterSerial.begin(9600, SERIAL_8N1, 16, 17); // UART2 cho máy in

  pinMode(BUSY_PIN, INPUT);

  printer_init(); // Khởi tạo máy in
  PrintDebugLn("Printer ready");
}

/* void loop() {
  if (digitalRead(BUSY_PIN) == HIGH) {
    PrintDebugLn("Printer is busy...");
    delay(500);
    return;
  }

  PrintDebugLn("Sending print job...");

  // In văn bản tiếng Việt
  printer_set_alignment(1); // căn giữa
  printer_set_bold(true);
  printer_println("Chao ban!");
  printer_set_bold(false);
  printer_new_line(1);

  // In tiếng Việt có dấu (sẽ map lại UTF-8 đơn giản)
  printer_print_vietnamese("Xin chào ESP32!\n");

  // In mã vạch Code128
  printer_set_alignment(0); // trái
  printer_set_barcode_height(80);
  printer_set_barcode_width(3);
  printer_set_barcode_text_position(2); // dưới
  printer_print_barcode("123456", 73); // CODE128 = 73

  printer_new_line(2);

  delay(10000); // chờ 10s
} */

void loop() {
  if (digitalRead(BUSY_PIN) == HIGH) {
    PrintDebugLn("Printer is busy...");
    delay(500);
    return;
  }

  PrintDebugLn("In QR Code thanh toán...");

  printer_set_alignment(1); // căn giữa
  printer_println("Quét mã để thanh toán");

  printer_print_qrcode(
    "00020101021138560010A0000007270126000697041501121008690222360208QRIBFTTA53037045802VN63043E22",
    5,  // phiên bản QR
    2,  // mức sửa lỗi M
    4   // zoom phóng đại
  );

  printer_new_line(3); // thêm dòng trắng
  delay(15000); // đợi 15s
}
