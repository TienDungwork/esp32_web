#include "touch.h"

#if DEVICE_HAS_DISPLAY
Arduino_DataBus *bus = new Arduino_HWSPI(
    SCREEN_DC /* DC */, SCREEN_CS /* CS */, SCREEN_SCLK /* SCK */,
    SCREEN_MOSI /* MOSI */, SCREEN_MISO /* MISO */);

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> CST226SE(new Arduino_CST2xxSE(IIC_Bus, CST226SE_DEVICE_ADDRESS,
                                                           TOUCH_RST, TOUCH_INT, Arduino_IIC_Touch_Interrupt));

// Biến toàn cục để theo dõi bài kiểm tra hiện tại
int32_t Current_Test = 0; // Bắt đầu từ bài kiểm tra 0
bool Skip_Current_Test = false;

void Arduino_IIC_Touch_Interrupt(void)
{
    CST226SE->IIC_Interrupt_Flag = true;
}


void Skip_Test_Loop()
{
    uint8_t fingers_number = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);

    if (fingers_number > 0)
    {
        int32_t touch_x = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t touch_y = CST226SE->IIC_Read_Device_Value(CST226SE->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

        Touch_Rotation_Convert(&touch_x, &touch_y);

        switch (Current_Rotation)
        {
        case 0:
        case 2:
            if (touch_x > 40 && touch_x < 140 &&
                touch_y > SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) && touch_y < SCREEN_HEIGHT - (SCREEN_HEIGHT / 3) + 40)
            {
                Skip_Current_Test = true;
                Current_Test++; // Tăng chỉ số bài kiểm tra
            }
            break;
        case 1:
        case 3:
            if (touch_x > (SCREEN_HEIGHT / 3 + 10) && touch_x < (SCREEN_HEIGHT / 3 + 10) + 140 &&
                touch_y > 155 && touch_y < 195)
            {
                Skip_Current_Test = true;
                Current_Test++; // Tăng chỉ số bài kiểm tra
            }
            break;

        default:
            break;
        }
    }
}


bool Touch_Rotation_Convert(int32_t *x, int32_t *y)
{
    if ((*x == -1) && (*y == -1))
    {
        return false;
    }

    int32_t x_buffer = *x;
    int32_t y_buffer = *y;

    switch (Current_Rotation)
    {
    case 0:
        break;
    case 1:
        *x = y_buffer;
        *y = SCREEN_WIDTH - x_buffer;
        break;
    case 2:
        *x = SCREEN_WIDTH - x_buffer;
        *y = SCREEN_HEIGHT - y_buffer;
        break;
    case 3:
        *x = SCREEN_HEIGHT - y_buffer;
        *y = x_buffer;
        break;

    default:
        break;
    }
    return true;
}
#endif  // DEVICE_HAS_DISPLAY