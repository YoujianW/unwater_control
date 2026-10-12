#include "app_button_task.h"
#include "app_task.h"
#include "button.h"
#include "led.h"

static void KEY1_PressDown(Button *handle, void *user_data)
{
    (void)handle;
    (void)user_data;
    LED1_ON();
}

static void KEY1_PressUp(Button *handle, void *user_data)
{
    (void)handle;
    (void)user_data;
    LED1_OFF();
}

static void KEY2_PressDown(Button *handle, void *user_data)
{
    (void)handle;
    (void)user_data;
    LED2_ON();
}

static void KEY2_PressUp(Button *handle, void *user_data)
{
    (void)handle;
    (void)user_data;
    LED2_OFF();
}

void button_Task_func(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    BUTTON_Init(KEY1_PressDown, KEY1_PressUp, KEY2_PressDown, KEY2_PressUp);

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TICKS_INTERVAL));
        BUTTON_Ticks();
    }
}
