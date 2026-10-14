/***************************************************************
 * mpu_6050_software_i2c.h
 * MPU6050 软件 I2C 驱动，兼容原 MPU6050 任务调用
 ***************************************************************/
#ifndef MPU_6050_SOFTWARE_I2C_H
#define MPU_6050_SOFTWARE_I2C_H

#include "mpu6050.h"
#include "bsp_software_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BSP_SOFTWARE_I2C_Handle_t i2c;
    uint8_t address;             /* 未左移的 7 位地址 */
    uint8_t initialized;
} MPU6050_SOFTWARE_I2C_Handle_t;

/* 使用 soft_i2c_scl / soft_i2c_sda 引脚宏初始化软件总线。
 * i2c 参数仅保留原调用形式，不参与软件总线选择，可传 NULL。
 * 量程及采样率沿用 ±2 g、±250 度/秒、100 Hz，成功返回 1。
 * 调用前须初始化时间组件，SCL/SDA 需外部上拉。 */
uint8_t MPU6050_SOFTWARE_I2C_Init(MPU6050_SOFTWARE_I2C_Handle_t *handle,
                                 BSP_I2C_Handle_t *i2c, uint8_t address);
/* 连续读取 14 字节并换算，BSP 读接口无通信状态返回值。 */
void MPU6050_SOFTWARE_I2C_ReadData(MPU6050_SOFTWARE_I2C_Handle_t *handle,
                                  MPU6050_Data_t *data);

#ifdef __cplusplus
}
#endif

/* 应用层替换头文件即可使用软件 I2C，硬件驱动接口仍独立保留。 */
#define MPU6050_Handle_t         MPU6050_SOFTWARE_I2C_Handle_t
#define MPU6050_Init             MPU6050_SOFTWARE_I2C_Init
#define MPU6050_ReadData         MPU6050_SOFTWARE_I2C_ReadData

#endif
