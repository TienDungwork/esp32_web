#ifndef RS232_H
#define RS232_H

#include "driver_config.h"


void UART_Initialization(void);
void Handle_RS485_Data();
void Handle_RS232_Data();

void GFX_Print_UART_Info_Loop(void);  // Cập nhật thông tin RS485



#endif
