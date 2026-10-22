#include "app_oled_task.h"
#include "app_task.h"
#include "at32f403a_407_wk_config.h"
#include "oled.h"
#include "time_core.h"

#define OLED_REFRESH_INTERVAL_MS  200

void oled_Task_func(void *pvParameters)
{
    /* 本任务独占 I2C2，显存使用静态存储，避免占用任务栈。 */
    static BSP_I2C_Handle_t i2c_handle;
    static OLED_Handle_t oled_handle;
    time_runtime_t runtime;
    TickType_t last_wake_time;

    (void)pvParameters;
    BSP_I2C_Init(&i2c_handle, I2C2);
    OLED_Init(&oled_handle, &i2c_handle, OLED_ADDRESS_LOW);

    OLED_ShowString(&oled_handle, 0, 0, "SSD1306 128x64");
    OLED_ShowString(&oled_handle, 0, 8, "I2C2 ADDR:0x3C");
    OLED_ShowString(&oled_handle, 0, 24, "Days:");
    OLED_ShowString(&oled_handle, 0, 40, "Time:");
    OLED_ShowChar(&oled_handle, 48, 40, ':');
    OLED_ShowChar(&oled_handle, 66, 40, ':');
    OLED_ShowChar(&oled_handle, 84, 40, '.');
    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        /* 时间字段由同一次采样换算，避免秒、分进位时显示不一致。 */
        time_get_runtime(&runtime);
        OLED_ShowNum(&oled_handle, 36, 24, runtime.days, 10);
        OLED_ShowNum(&oled_handle, 36, 40, runtime.hours, 2);
        OLED_ShowNum(&oled_handle, 54, 40, runtime.minutes, 2);
        OLED_ShowNum(&oled_handle, 72, 40, runtime.seconds, 2);
        OLED_ShowNum(&oled_handle, 90, 40, runtime.millis, 3);
        OLED_Update(&oled_handle);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(OLED_REFRESH_INTERVAL_MS));
    }
}
