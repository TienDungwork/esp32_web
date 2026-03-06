#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ══════════════════════════════════════════════════
//  PIN CONFIGURATION cho Serial2 (UART → Máy in)
// ══════════════════════════════════════════════════
#ifndef PRINTER_TX_PIN
  #define PRINTER_TX_PIN  4   // RS232/RS485 TX (WeighAll gốc: 4)
#endif
#ifndef PRINTER_RX_PIN
  #define PRINTER_RX_PIN  5   // RS232/RS485 RX (WeighAll gốc: 5)
#endif
#ifndef PRINTER_BAUD
  #define PRINTER_BAUD    9600
#endif

// ── Setup ──
void printerSetup();

// ── Cơ bản ──
void printer_init(void);
void printer_reset(void);
void printer_set_line_spacing(uint8_t n);
void printer_set_char_spacing(uint8_t n);
void printer_set_alignment(uint8_t align);  // 0=left 1=center 2=right
void printer_set_bold(bool enable);
void printer_set_underline(bool enable);
void printer_set_reverse(bool enable);
void printer_set_text_size(uint8_t width, uint8_t height);

// ── In văn bản ──
void printer_print_text(const char *text);
void printer_println(const char *text);
void printer_new_line(uint8_t lines);

// ── Mã vạch 1D ──
void printer_set_barcode_height(uint8_t height);
void printer_set_barcode_width(uint8_t width);
void printer_set_barcode_text_position(uint8_t pos);
void printer_print_barcode(const char *data, uint8_t type);

// ── QR Code ──
void printer_print_qrcode(const char *data, uint8_t version, uint8_t error_level, uint8_t zoom);

// ── Bitmap ──
void printer_print_bitmap(const uint8_t *bitmap_data, uint16_t width, uint16_t height);

// ── Tiếng Việt ──
void printer_print_vietnamese(const char *utf8_text);

#ifdef __cplusplus
}
#endif
