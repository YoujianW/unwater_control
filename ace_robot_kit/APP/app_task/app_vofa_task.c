#include "app_vofa_task.h"
#include "app_task.h"
#include "vofa.h"
#include "wk_usart.h"

#define VOFA_SEND_INTERVAL_MS   100

void vofa_Task_func(void *pvParameters)
{
    static BSP_USART_Handle_t vofa_usart;
    static VofaHandle_t vofa_handle;
    TickType_t last_wake_time = xTaskGetTickCount();

    BSP_USART_Init(&vofa_usart, USART1);
    VOFA_Init(&vofa_handle, &vofa_usart, VOFA_FORMAT_JUSTFLOAT);
    VOFA_SetChannelName(&vofa_handle, 0, "VREF_raw");
    VOFA_SetChannelName(&vofa_handle, 1, "Temperature_C");
    VOFA_SetChannelName(&vofa_handle, 2, "Accel_X_g");
    VOFA_SetChannelName(&vofa_handle, 3, "Accel_Y_g");
    VOFA_SetChannelName(&vofa_handle, 4, "Accel_Z_g");
    VOFA_SetChannelName(&vofa_handle, 5, "Gyro_X_dps");
    VOFA_SetChannelName(&vofa_handle, 6, "Gyro_Y_dps");
    VOFA_SetChannelName(&vofa_handle, 7, "Gyro_Z_dps");
    VOFA_SetChannelName(&vofa_handle, 8, "MPU_Temp_C");

    while (1)
    {
        /* 固定执行周期用 vTaskDelayUntil()；执行完再等待用 vTaskDelay()。 */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(VOFA_SEND_INTERVAL_MS));

        /* 通道 0 为参考电压 ADC 原始值，通道 1 为摄氏温度。 */
        VOFA_SetChannelValue(&vofa_handle, 0, (float)adc_vref_raw);
        VOFA_SetChannelValue(&vofa_handle, 1, (float)adc_temperature_centi_c / 100.0f);

        VOFA_SetChannelValue(&vofa_handle, 2, mpu6050_data.accel_x);
        VOFA_SetChannelValue(&vofa_handle, 3, mpu6050_data.accel_y);
        VOFA_SetChannelValue(&vofa_handle, 4, mpu6050_data.accel_z);
        VOFA_SetChannelValue(&vofa_handle, 5, mpu6050_data.gyro_x);
        VOFA_SetChannelValue(&vofa_handle, 6, mpu6050_data.gyro_y);
        VOFA_SetChannelValue(&vofa_handle, 7, mpu6050_data.gyro_z);
        VOFA_SetChannelValue(&vofa_handle, 8, mpu6050_data.temperature);

        /* 暂停 VOFA 串口发送，先由 SPI 任务打印 W25Q16 读写结果。 */
        // BSP_USART_TxLock();
        // VOFA_SendData(&vofa_handle);
        // BSP_USART_TxUnlock();
    }
}
