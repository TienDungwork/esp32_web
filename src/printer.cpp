#include "printer.h"
#include <string.h>
#include <Arduino.h>

// ══════════════════════════════════════════════════
//  Serial2 setup
// ══════════════════════════════════════════════════
void printerSetup() {
  Serial2.begin(PRINTER_BAUD, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
  printer_init();
  Serial.printf("[Printer] UART2 initialized (TX=%d RX=%d baud=%d)\n",
                PRINTER_TX_PIN, PRINTER_RX_PIN, PRINTER_BAUD);
}

// ══════════════════════════════════════════════════
//  Gửi dữ liệu qua Serial2
// ══════════════════════════════════════════════════
static void printer_send_data(const uint8_t *data, size_t len) {
  Serial2.write(data, len);
}

static void send_byte(uint8_t b) {
  printer_send_data(&b, 1);
}

static void send_command(const uint8_t *cmd, size_t len) {
  printer_send_data(cmd, len);
}

// ══════════════════════════════════════════════════
//  Lệnh cơ bản
// ══════════════════════════════════════════════════
void printer_init(void) {
  printer_reset();
}

void printer_reset(void) {
  const uint8_t cmd[] = { 0x1B, 0x40 }; // ESC @
  send_command(cmd, sizeof(cmd));
}

void printer_set_line_spacing(uint8_t n) {
  const uint8_t cmd[] = { 0x1B, 0x31, n };
  send_command(cmd, sizeof(cmd));
}

void printer_set_char_spacing(uint8_t n) {
  const uint8_t cmd[] = { 0x1B, 0x20, n };
  send_command(cmd, sizeof(cmd));
}

void printer_set_alignment(uint8_t align) {
  const uint8_t cmd[] = { 0x1B, 0x61, align };
  send_command(cmd, sizeof(cmd));
}

void printer_set_bold(bool enable) {
  const uint8_t cmd[] = { 0x1B, 0x45, (uint8_t)(enable ? 1 : 0) };
  send_command(cmd, sizeof(cmd));
}

void printer_set_underline(bool enable) {
  const uint8_t cmd[] = { 0x1B, 0x2E, (uint8_t)(enable ? 1 : 0) };
  send_command(cmd, sizeof(cmd));
}

void printer_set_reverse(bool enable) {
  const uint8_t cmd[] = { 0x1D, 0x42, (uint8_t)(enable ? 1 : 0) };
  send_command(cmd, sizeof(cmd));
}

void printer_set_text_size(uint8_t width, uint8_t height) {
  if (width < 1 || width > 8) width = 1;
  if (height < 1 || height > 8) height = 1;
  const uint8_t cmd[] = { 0x1B, 0x58, width, height };
  send_command(cmd, sizeof(cmd));
}

// ══════════════════════════════════════════════════
//  In văn bản
// ══════════════════════════════════════════════════
void printer_print_text(const char *text) {
  printer_send_data((const uint8_t *)text, strlen(text));
}

void printer_println(const char *text) {
  printer_print_text(text);
  send_byte('\n');
}

void printer_new_line(uint8_t lines) {
  for (uint8_t i = 0; i < lines; i++) {
    send_byte('\n');
  }
}

// ══════════════════════════════════════════════════
//  Mã vạch 1D
// ══════════════════════════════════════════════════
void printer_set_barcode_height(uint8_t height) {
  const uint8_t cmd[] = { 0x1D, 0x68, height };
  send_command(cmd, sizeof(cmd));
}

void printer_set_barcode_width(uint8_t width) {
  const uint8_t cmd[] = { 0x1D, 0x77, width };
  send_command(cmd, sizeof(cmd));
}

void printer_set_barcode_text_position(uint8_t pos) {
  const uint8_t cmd[] = { 0x1D, 0x48, pos };
  send_command(cmd, sizeof(cmd));
}

void printer_print_barcode(const char *data, uint8_t type) {
  size_t len = strlen(data);
  if (len > 255) len = 255;
  send_byte(0x1D); send_byte(0x6B); send_byte(type);
  printer_send_data((const uint8_t *)data, len);
  send_byte(0x00);
}

// ══════════════════════════════════════════════════
//  QR Code
// ══════════════════════════════════════════════════
void printer_print_qrcode(const char *data, uint8_t version, uint8_t error_level, uint8_t zoom) {
  const uint8_t zoom_cmd[] = { 0x1D, 0x57, zoom };
  send_command(zoom_cmd, sizeof(zoom_cmd));

  size_t len = strlen(data);
  if (len > 255) len = 255;

  uint8_t buf[300];
  size_t i = 0;
  buf[i++] = 0x1D; buf[i++] = 0x6B; buf[i++] = 32;
  buf[i++] = version;
  buf[i++] = error_level;
  memcpy(&buf[i], data, len); i += len;
  buf[i++] = 0x00;

  printer_send_data(buf, i);
}

// ══════════════════════════════════════════════════
//  Bitmap
// ══════════════════════════════════════════════════
void printer_print_bitmap(const uint8_t *bitmap_data, uint16_t width, uint16_t height) {
  uint16_t bytes_per_row = (width + 7) / 8;

  for (uint16_t row = 0; row < height; row += 8) {
    uint8_t nL = bytes_per_row & 0xFF;
    uint8_t nH = (bytes_per_row >> 8) & 0xFF;
    uint8_t cmd[] = { 0x1B, 0x2A, 0x00, nL, nH };
    printer_send_data(cmd, sizeof(cmd));

    for (uint16_t x = 0; x < bytes_per_row; x++) {
      uint8_t data = 0x00;
      for (uint8_t bit = 0; bit < 8; bit++) {
        uint16_t y = row + bit;
        if (y >= height) continue;
        uint16_t byte_index = y * bytes_per_row + x;
        if (bitmap_data[byte_index] & (0x80 >> (x % 8))) {
          data |= (1 << bit);
        }
      }
      printer_send_data(&data, 1);
    }

    const uint8_t lf = 0x0A;
    printer_send_data(&lf, 1);
  }
}

// ══════════════════════════════════════════════════
//  Tiếng Việt (demo — ánh xạ đơn giản)
// ══════════════════════════════════════════════════
static char convert_utf8_to_ascii(uint8_t c) {
  switch (c) {
    case 0xE1: return 'a';
    case 0xBA: return 'o';
    case 0xE0: return 'a';
    default:   return c;
  }
}

void printer_print_vietnamese(const char *utf8_text) {
  while (*utf8_text) {
    uint8_t c = (uint8_t)(*utf8_text);
    if (c >= 0xC0) {
      char mapped = convert_utf8_to_ascii(c);
      send_byte(mapped);
      utf8_text += 2;
    } else {
      send_byte(c);
      utf8_text++;
    }
  }
}
