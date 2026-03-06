#include "tft_display.h"

//#if DEVICE_HAS_DISPLAY
size_t CycleTime = 0;
size_t CycleTime_2 = 0;
size_t CycleTime_3 = 0;
size_t CycleTime_4 = 0;
size_t CycleTime_5 = 0;
size_t CycleTime_6 = 0;
size_t CycleTime_7 = 0;
uint8_t Image_Flag = 0;

uint8_t Current_Rotation = 1;


#if DEVICE_HAS_DISPLAY
Arduino_GFX *gfx = new Arduino_ST7796(

    bus, SCREEN_RST /* RST */, Current_Rotation /* rotation */, true /* IPS */,
    SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */,
    49 /* col offset 1 */, 0 /* row offset 1 */, 49 /* col_offset2 */, 0 /* row_offset2 */);


void Rotation_Trigger(void (*func)())
{
    if (digitalRead(0) == LOW)
    {
        delay(300);
        Current_Rotation++;
        if (Current_Rotation > 3)
        {
            Current_Rotation = 0;
        }

        gfx->setRotation(Current_Rotation);
        func();
    }
}



void GFX_Print_Touch_Info_Loop()
{
    int32_t touch_x_1 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH1_COORDINATE_X);
    int32_t touch_y_1 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH1_COORDINATE_Y);
    int32_t touch_x_2 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH2_COORDINATE_X);
    int32_t touch_y_2 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH2_COORDINATE_Y);
    int32_t touch_x_3 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH3_COORDINATE_X);
    int32_t touch_y_3 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH3_COORDINATE_Y);
    int32_t touch_x_4 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH4_COORDINATE_X);
    int32_t touch_y_4 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH4_COORDINATE_Y);
    int32_t touch_x_5 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH5_COORDINATE_X);
    int32_t touch_y_5 = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH5_COORDINATE_Y);
    uint8_t fingers_number = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);

    Touch_Rotation_Convert(&touch_x_1, &touch_y_1);
    Touch_Rotation_Convert(&touch_x_2, &touch_y_2);
    Touch_Rotation_Convert(&touch_x_3, &touch_y_3);
    Touch_Rotation_Convert(&touch_x_4, &touch_y_4);
    Touch_Rotation_Convert(&touch_x_5, &touch_y_5);

    gfx->fillRect(0, 40, SCREEN_WIDTH, SCREEN_HEIGHT / 2, WHITE);
    gfx->setTextSize(1);
    gfx->setTextColor(BLACK);

    gfx->setCursor(20, 50);
    gfx->printf("ID: %#X ", (int32_t)CST226SE->IIC_Device_ID());

    gfx->setCursor(20, 65);
    gfx->printf("Fingers Number:%d ", fingers_number);

    gfx->setCursor(20, 80);
    gfx->printf("Touch X1:%d Y1:%d ", touch_x_1, touch_y_1);
    gfx->setCursor(20, 95);
    gfx->printf("Touch X2:%d Y2:%d ", touch_x_2, touch_y_2);
    gfx->setCursor(20, 110);
    gfx->printf("Touch X3:%d Y3:%d ", touch_x_3, touch_y_3);
    gfx->setCursor(20, 125);
    gfx->printf("Touch X4:%d Y4:%d ", touch_x_4, touch_y_4);
    gfx->setCursor(20, 140);
    gfx->printf("Touch X5:%d Y5:%d ", touch_x_5, touch_y_5);
}

/* void GFX_Print_Time_Info_Loop()
{
    gfx->fillRoundRect(35, 35, 152, 95, 10, WHITE);
    gfx->setTextSize(1);

    if (Wifi_Connection_Flag == true)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 1000))
        {
            PrintDebugLn("Failed to obtain time");
            gfx->setCursor(50, 45);
            gfx->setTextColor(RED);
            gfx->print("Time error");
            return;
        }
        PrintDebugLn("Get time success");
        PrintDebugLn(&timeinfo, "%A,%B %d %Y %H:%M:%S"); // Format Output
        gfx->setCursor(50, 45);
        gfx->setTextColor(ORANGE);
        gfx->print(&timeinfo, " %Y");
        gfx->setCursor(50, 60);
        gfx->print(&timeinfo, "%B %d");
        gfx->setCursor(50, 75);
        gfx->print(&timeinfo, "%H:%M:%S");
    }
    else
    {
        gfx->setCursor(50, 45);
        gfx->setTextColor(RED);
        gfx->print("Network error");
    }

    gfx->setCursor(50, 90);
    gfx->printf("SYS Time:%d", (uint32_t)millis() / 1000);
}
 */


void GFX_Print_1()
{
    switch (Current_Rotation)
    {
    case 0:
    case 2:
        gfx->fillRect(20, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, ORANGE);
        gfx->drawRect(20, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, PURPLE);
        gfx->fillRect(120, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, PURPLE);
        gfx->drawRect(120, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4), 80, 40, ORANGE);
        gfx->setTextSize(1);
        gfx->setTextColor(WHITE);
        gfx->setCursor(35, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 15);
        gfx->printf("Try Again");
        gfx->setCursor(135, SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 15);
        gfx->printf("Next Test");
        break;

    case 1:
    case 3:
        gfx->fillRect(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20, 20, 80, 40, ORANGE);
        gfx->drawRect(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20, 20, 80, 40, PURPLE);
        gfx->fillRect(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20, 80, 80, 40, PURPLE);
        gfx->drawRect(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20, 80, 80, 40, ORANGE);
        gfx->setTextSize(1);
        gfx->setTextColor(WHITE);
        gfx->setCursor(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 35, 35);
        gfx->printf("Try Again");
        gfx->setCursor(SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 35, 95);
        gfx->printf("Next Test");
        break;

    default:
        break;
    }
}

int8_t GFX_Print_1_Trigger(int32_t x, int32_t y)
{
    if ((x == -1) && (y == -1))
    {
        return -1;
    }

    switch (Current_Rotation)
    {
    case 0:
    case 2:
        if (x > 20 && x < 100 &&
            y > (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4)) && y < (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 40))
        {
            return 1; // 触发按键1
        }
        if (x > 120 && x < 200 &&
            y > (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4)) && y < (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 40))
        {
            return 2; // 触发按键2
        }
        break;

    case 1:
    case 3:
        if (x > (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20) && x < (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20) + 80 &&
            y > 20 && y < 60)
        {
            return 1; // 触发按键1
        }
        if (x > (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20) && x < (SCREEN_HEIGHT - (SCREEN_HEIGHT / 4) + 20) + 80 &&
            y > 80 && y < 120)
        {
            return 2; // 触发按键2
        }
        break;

    default:
        break;
    }

    return -1;
}

void GFX_Print_2()
{
    switch (Current_Rotation)
    {
    case 0:
    case 2:
        gfx->fillRect(40, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3), 140, 40, RED);
        gfx->drawRect(40, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3), 140, 40, CYAN);

        gfx->setTextSize(1);
        gfx->setTextColor(WHITE);
        gfx->setCursor(60, SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) + 15);
        gfx->printf("Skip Current Test");
        break;
    case 1:
    case 3:
        gfx->fillRect(SCREEN_HEIGHT / 3 + 10, 155, 140, 40, RED);
        gfx->drawRect(SCREEN_HEIGHT / 3 + 10, 155, 140, 40, CYAN);

        gfx->setTextSize(1);
        gfx->setTextColor(WHITE);
        gfx->setCursor(SCREEN_HEIGHT / 3 + 25, 170);
        gfx->printf("Skip Current Test");
        break;

    default:
        break;
    }
}

void GFX_Print_TEST(String s)
{
    Skip_Current_Test = false;

    switch (Current_Rotation)
    {
    case 0:
    case 2:
        gfx->fillScreen(WHITE);
        gfx->setCursor(SCREEN_WIDTH / 4 + 20, SCREEN_HEIGHT / 4);
        gfx->setTextSize(3);
        gfx->setTextColor(PALERED);
        gfx->printf("TEST");

        gfx->setCursor(20, SCREEN_HEIGHT / 4 + 40);
        gfx->setTextSize(2);
        gfx->setTextColor(BLACK);
        gfx->print(s);

        GFX_Print_2();

        gfx->setCursor(SCREEN_WIDTH / 2 - 17, SCREEN_HEIGHT / 2);
        gfx->setTextSize(4);
        gfx->setTextColor(RED);
        gfx->printf("3");
        delay(300);
        gfx->fillRect(SCREEN_WIDTH / 2 - 17, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 + 20, 20, WHITE);
        gfx->setCursor(SCREEN_WIDTH / 2 - 17, SCREEN_HEIGHT / 2);
        gfx->printf("2");
        for (int i = 0; i < 100; i++)
        {
            Skip_Test_Loop();
            delay(10);

            if (Skip_Current_Test == true)
            {
                break;
            }
        }
        gfx->fillRect(SCREEN_WIDTH / 2 - 17, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 + 20, 20, WHITE);
        gfx->setCursor(SCREEN_WIDTH / 2 - 17, SCREEN_HEIGHT / 2);
        gfx->printf("1");
        for (int i = 0; i < 100; i++)
        {
            Skip_Test_Loop();
            delay(10);

            if (Skip_Current_Test == true)
            {
                break;
            }
        }
        break;
    case 1:
    case 3:
        gfx->fillScreen(WHITE);
        gfx->setCursor(SCREEN_HEIGHT / 2 - 50, SCREEN_WIDTH / 8);
        gfx->setTextSize(3);
        gfx->setTextColor(PALERED);
        gfx->printf("TEST");

        gfx->setCursor(20, SCREEN_WIDTH / 8 + 40);
        gfx->setTextSize(2);
        gfx->setTextColor(BLACK);
        gfx->print(s);

        GFX_Print_2();

        gfx->setCursor(SCREEN_HEIGHT / 2 - 17, SCREEN_WIDTH / 3 + 30);
        gfx->setTextSize(4);
        gfx->setTextColor(RED);
        gfx->printf("3");
        delay(300);
        gfx->fillRect(SCREEN_HEIGHT / 2 - 17, SCREEN_WIDTH / 3 + 30, SCREEN_HEIGHT / 3 + 20, 20, WHITE);
        gfx->setCursor(SCREEN_HEIGHT / 2 - 17, SCREEN_WIDTH / 3 + 30);
        gfx->printf("2");
        for (int i = 0; i < 100; i++)
        {
            Skip_Test_Loop();
            delay(10);

            if (Skip_Current_Test == true)
            {
                break;
            }
        }
        gfx->fillRect(SCREEN_HEIGHT / 2 - 17, SCREEN_WIDTH / 3 + 30, SCREEN_HEIGHT / 3 + 20, 20, WHITE);
        gfx->setCursor(SCREEN_HEIGHT / 2 - 17, SCREEN_WIDTH / 3 + 30);
        gfx->printf("1");
        for (int i = 0; i < 100; i++)
        {
            Skip_Test_Loop();
            delay(10);

            if (Skip_Current_Test == true)
            {
                break;
            }
        }
        break;

    default:
        break;
    }
}

void GFX_Print_FINISH()
{
    gfx->setCursor(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4 - 30);
    gfx->setTextSize(3);
    gfx->setTextColor(ORANGE);
    gfx->printf("FINISH");
}

void GFX_Print_START()
{
    switch (Current_Rotation)
    {
    case 0:
    case 2:
        gfx->setCursor(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 50);
        break;
    case 1:
    case 3:
        gfx->setCursor(SCREEN_HEIGHT / 2 - 50, SCREEN_WIDTH / 2 - 20);
        break;

    default:
        break;
    }

    gfx->setTextSize(4);
    gfx->setTextColor(RED);
    gfx->printf("START");
}
#endif  // DEVICE_HAS_DISPLAY
