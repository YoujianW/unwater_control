#ifndef APP_I2C_TASK_H
#define APP_I2C_TASK_H

#include "mpu_6050_software_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 最近一次采样及就绪标志，读写时使用临界区保护。 */
extern MPU6050_Data_t mpu6050_data;

void i2c_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
