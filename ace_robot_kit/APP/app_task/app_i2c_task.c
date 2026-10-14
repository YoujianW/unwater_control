#include "app_i2c_task.h"
#include "app_task.h"

#define I2C_SAMPLE_INTERVAL_MS   10
/* AD0 接地使用 0x68，接高电平时改为 MPU6050_ADDRESS_HIGH。 */
#define I2C_MPU6050_ADDRESS      MPU6050_ADDRESS_LOW

MPU6050_Data_t mpu6050_data;

void i2c_Task_func(void *pvParameters)
{
    BSP_I2C_Handle_t i2c_handle;
    MPU6050_Handle_t mpu6050_handle;
    MPU6050_Data_t data;
    TickType_t last_wake_time;

    BSP_I2C_Init(&i2c_handle, I2C1);
    while (!MPU6050_Init(&mpu6050_handle, &i2c_handle, I2C_MPU6050_ADDRESS))
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        MPU6050_ReadData(&mpu6050_handle, &data);
        /* 临界区应尽量短，只保护这几行数据复制，防止出现原子性问题 */
        taskENTER_CRITICAL();
        mpu6050_data = data;
        taskEXIT_CRITICAL();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(I2C_SAMPLE_INTERVAL_MS));
    }
}
