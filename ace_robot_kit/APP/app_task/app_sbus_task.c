#include "app_sbus_task.h"
#include "app_task.h"
#include "sbus.h"
#include "wk_usart.h"
#include <stdio.h>

#define SBUS_PRINT_INTERVAL_MS   100
#define SBUS_PRINT_CHANNEL_COUNT 10

static SBUS_Handle_t sbus_handle;

void APP_SBUS_IRQHandler(uint8_t byte)
{
    SBUS_IRQHandler(&sbus_handle, byte);
}

void sbus_Task_func(void *pvParameters)
{
    SBUS_Data_t sbus_data;
    uint8_t data_ready;
    TickType_t last_wake_time = xTaskGetTickCount();

    (void)pvParameters;

    SBUS_Init(&sbus_handle, USART2);

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SBUS_PRINT_INTERVAL_MS));

        /* 中断持续收帧，任务每次只打印最新完整帧，避免串口刷屏。 */
        taskENTER_CRITICAL();
        data_ready = SBUS_GetData(&sbus_handle, &sbus_data);
        taskEXIT_CRITICAL();

        if (data_ready)
        {
            BSP_USART_TxLock();
            printf("[SBUS]");
            for (uint8_t i = 0; i < SBUS_PRINT_CHANNEL_COUNT; i++)
            {
                printf(" CH%u=%u", (unsigned int)(i + 1U),
                       (unsigned int)sbus_data.channel[i]);
            }
            printf("\r\n");
            BSP_USART_TxUnlock();
        }
    }
}
