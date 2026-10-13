#include "app_vofa_task.h"
#include "app_task.h"
#include "vofa.h"
#include "wk_usart.h"

#define VOFA_SEND_INTERVAL_MS   1000

void vofa_Task_func(void *pvParameters)
{
    static BSP_USART_Handle_t vofa_usart;
    static VofaHandle_t vofa_handle;
    TickType_t last_wake_time = xTaskGetTickCount();

    BSP_USART_Init(&vofa_usart, USART1);
    VOFA_Init(&vofa_handle, &vofa_usart, VOFA_FORMAT_JUSTFLOAT);
    VOFA_SetChannelName(&vofa_handle, 0, "VREF_raw");
    VOFA_SetChannelName(&vofa_handle, 1, "Temperature_C");

    while (1)
    {
        /* 固定执行周期用 vTaskDelayUntil()；执行完再等待用 vTaskDelay()。 */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(VOFA_SEND_INTERVAL_MS));

        /* 通道 0 保留原 Debug 中的参考电压 ADC 原始值，通道 1 为摄氏温度。 */
        VOFA_SetChannelValue(&vofa_handle, 0, (float)adc_vref_raw);
        VOFA_SetChannelValue(&vofa_handle, 1, (float)adc_temperature_centi_c / 100.0f);

        BSP_USART_TxLock();
        VOFA_SendData(&vofa_handle);
        BSP_USART_TxUnlock();
    }
}
