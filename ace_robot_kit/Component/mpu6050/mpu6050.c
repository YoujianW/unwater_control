/***************************************************************
 * mpu6050.c
 * MPU6050 初始化及三轴加速度、角速度、温度采集
 ***************************************************************/
#include "mpu6050.h"
#include "time_core.h"
#include <stddef.h>

#define MPU6050_SMPLRT_DIV_REG   0x19U
#define MPU6050_ACCEL_XOUT_REG   0x3BU
#define MPU6050_PWR_MGMT_1_REG   0x6BU
#define MPU6050_PWR_MGMT_2_REG   0x6CU
#define MPU6050_WHO_AM_I_REG     0x75U
#define MPU6050_DEVICE_ID        0x68U

static int16_t MPU6050_ReadInt16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

uint8_t MPU6050_Init(MPU6050_Handle_t *handle, BSP_I2C_Handle_t *i2c,
                     uint8_t address)
{
    /* 从 0x19 起依次为采样分频、低通滤波、陀螺仪量程、加速度量程。 */
    static const uint8_t config[4] = {9U, 3U, 0U, 0U};
    uint8_t readback[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    uint8_t value = 0;
    uint8_t i;

    if (handle == NULL || i2c == NULL || i2c->i2c_handle == NULL
        || (address != MPU6050_ADDRESS_LOW && address != MPU6050_ADDRESS_HIGH)) return 0;

    handle->i2c = i2c;
    handle->address = address;
    BSP_I2C_ReadReg(i2c, address, MPU6050_WHO_AM_I_REG, &value);
    if (value != MPU6050_DEVICE_ID) return 0;

    BSP_I2C_WriteReg(i2c, address, MPU6050_PWR_MGMT_1_REG, 0x80U);
    time_delay_ms(100);
    /* 唤醒并选择 X 轴陀螺仪 PLL 时钟，开启全部传感器轴及温度测量。 */
    BSP_I2C_WriteReg(i2c, address, MPU6050_PWR_MGMT_1_REG, 0x01U);
    BSP_I2C_WriteReg(i2c, address, MPU6050_PWR_MGMT_2_REG, 0x00U);
    time_delay_ms(100);
    /* DLPF=3 时陀螺仪输出 1 kHz，采样率为 1000 / (1 + 9) = 100 Hz。 */
    BSP_I2C_MemWrite(i2c, address, MPU6050_SMPLRT_DIV_REG, config, sizeof(config));

    /* BSP 不返回通信状态，通过读回检查唤醒及量程配置。 */
    value = 0xFFU;
    BSP_I2C_ReadReg(i2c, address, MPU6050_PWR_MGMT_1_REG, &value);
    if (value != 0x01U) return 0;
    value = 0xFFU;
    BSP_I2C_ReadReg(i2c, address, MPU6050_PWR_MGMT_2_REG, &value);
    if (value != 0x00U) return 0;
    BSP_I2C_MemRead(i2c, address, MPU6050_SMPLRT_DIV_REG, readback, sizeof(readback));
    for (i = 0; i < sizeof(config); i++)
    {
        if (readback[i] != config[i]) return 0;
    }
    return 1;
}

void MPU6050_ReadData(MPU6050_Handle_t *handle, MPU6050_Data_t *data)
{
    uint8_t buffer[14] = {0};

    if (handle == NULL || handle->i2c == NULL || data == NULL) return;

    /* 连续读取保证加速度、温度和角速度来自同一次传感器采样。 */
    BSP_I2C_MemRead(handle->i2c, handle->address, MPU6050_ACCEL_XOUT_REG,
                    buffer, sizeof(buffer));
    data->accel_x = (float)MPU6050_ReadInt16(&buffer[0]) / 16384.0f;
    data->accel_y = (float)MPU6050_ReadInt16(&buffer[2]) / 16384.0f;
    data->accel_z = (float)MPU6050_ReadInt16(&buffer[4]) / 16384.0f;
    data->temperature = (float)MPU6050_ReadInt16(&buffer[6]) / 340.0f + 36.53f;
    data->gyro_x = (float)MPU6050_ReadInt16(&buffer[8]) / 131.0f;
    data->gyro_y = (float)MPU6050_ReadInt16(&buffer[10]) / 131.0f;
    data->gyro_z = (float)MPU6050_ReadInt16(&buffer[12]) / 131.0f;
}
