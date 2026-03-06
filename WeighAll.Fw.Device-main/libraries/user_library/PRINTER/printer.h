#ifndef PRINTER_H
#define PRINTER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// UART send override
void printer_send_data(const uint8_t *data, size_t len);

// Các hàm máy in
void printer_init(void);
void printer_reset(void);
void printer_set_line_spacing(uint8_t n);
void printer_set_char_spacing(uint8_t n);
void printer_set_alignment(uint8_t align);
void printer_set_bold(bool enable);
void printer_set_underline(bool enable);
void printer_set_reverse(bool enable);
void printer_set_text_size(uint8_t width, uint8_t height);

void printer_print_text(const char *text);
void printer_println(const char *text);
void printer_new_line(uint8_t lines);

void printer_set_barcode_height(uint8_t height);
void printer_set_barcode_width(uint8_t width);
void printer_set_barcode_text_position(uint8_t pos);
void printer_print_barcode(const char *data, uint8_t type);

void printer_print_qrcode(const char *data, uint8_t version, uint8_t error_level, uint8_t zoom);
void printer_print_bitmap(const uint8_t *bitmap_data, uint16_t width, uint16_t height);
void printer_print_vietnamese(const char *utf8_text);

#ifdef __cplusplus
}
#endif

#endif
