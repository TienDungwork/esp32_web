#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "driver_config.h"

extern size_t CycleTime;
extern size_t CycleTime_2;
extern size_t CycleTime_3;
extern size_t CycleTime_4;
extern size_t CycleTime_5;
extern size_t CycleTime_6;
extern size_t CycleTime_7;
extern uint8_t Image_Flag;

extern uint8_t Current_Rotation;


extern Arduino_GFX *gfx;

void Rotation_Trigger(void (*func)());                         // Đăng ký callback xoay


void GFX_Print_Touch_Info_Loop();                              // Hiển thị thông tin cảm ứng
void GFX_Print_Time_Info_Loop();                               // Hiển thị thời gian

void GFX_Print_1();                                            // Vẽ giao diện 1
int8_t GFX_Print_1_Trigger(int32_t x, int32_t y);              // Xử lý chạm giao diện 1
void GFX_Print_2();                                            // Vẽ giao diện 2
void GFX_Print_TEST(String s);                                 // Hiển thị chuỗi test
void GFX_Print_FINISH();                                       // Hiển thị hoàn thành
void GFX_Print_START();                                        // Hiển thị bắt đầu


#endif  // TFT_DISPLAY_H