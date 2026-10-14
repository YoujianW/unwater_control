/***************************************************************
 * mpu6050.h
 * MPU6050 数据采集组件，使用 BSP I2C 寄存器接口
 ***************************************************************/
#ifndef MPU6050_H
#define MPU6050_H

#include "bsp_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_ADDRESS_LOW     0x68U
#define MPU6050_ADDRESS_HIGH    0x69U

typedef struct {
    BSP_I2C_Handle_t *i2c;
    uint8_t address;             /* 未左移的 7 位地址，由 AD0 电平决定 */
} MPU6050_Handle_t;

typedef struct {
    float accel_x;              /* 三轴加速度，单位 g */
    float accel_y;
    float accel_z;
    float gyro_x;               /* 三轴角速度，单位 度/秒 */
    float gyro_y;
    float gyro_z;
    float temperature;          /* MPU6050 内部温度，单位 摄氏度 */
} MPU6050_Data_t;

/* 初始化为 ±2 g、±250 度/秒、100 Hz；身份及配置读回正确时返回 1。
 * 调用前须完成 I2C 外设和时间组件初始化，复位及唤醒各等待 100 ms。 */
uint8_t MPU6050_Init(MPU6050_Handle_t *handle, BSP_I2C_Handle_t *i2c,
                     uint8_t address);
/* 连续读取 14 字节并换算。BSP 读接口无通信状态返回值。 */
void MPU6050_ReadData(MPU6050_Handle_t *handle, MPU6050_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif
