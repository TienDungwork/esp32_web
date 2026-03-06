#ifndef TOUCH_H
#define TOUCH_H


#include "driver_config.h"

extern Arduino_DataBus *bus;

extern std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
extern std::unique_ptr<Arduino_IIC> CST226SE;

extern bool Skip_Current_Test;
extern int32_t Current_Test;

void Skip_Test_Loop();
void Arduino_IIC_Touch_Interrupt(void);
bool Touch_Rotation_Convert(int32_t *x, int32_t *y);


#endif  // TOUCH_H