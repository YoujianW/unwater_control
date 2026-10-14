/***************************************************************
 * mpu_6050_software_i2c.c
 * MPU6050 软件 I2C 初始化及加速度、角速度、温度采集
 ***************************************************************/
#include "mpu_6050_software_i2c.h"
#include "at32f403a_407_wk_config.h"
#include "time_core.h"
#include <stddef.h>

#define MPU6050_SMPLRT_DIV_REG   0x19U
#define MPU6050_ACCEL_XOUT_REG   0x3BU
#define MPU6050_PWR_MGMT_1_REG   0x6BU
#define MPU6050_PWR_MGMT_2_REG   0x6CU
#define MPU6050_WHO_AM_I_REG     0x75U
#define MPU6050_DEVICE_ID        0x68U

static int16_t MPU6050_SOFTWARE_I2C_ReadInt16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

uint8_t MPU6050_SOFTWARE_I2C_Init(MPU6050_SOFTWARE_I2C_Handle_t *handle,
                                 BSP_I2C_Handle_t *i2c, uint8_t address)
{
    static const uint8_t config[4] = {9U, 3U, 0U, 0U};
    uint8_t readback[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    uint8_t value = 0;
    uint8_t i;

    /* 兼容原任务传入的硬件 I2C 句柄，实际通信仅使用软件总线。 */
    (void)i2c;
    if (handle == NULL) return 0;
    handle->initialized = 0;
    if (address != MPU6050_ADDRESS_LOW && address != MPU6050_ADDRESS_HIGH) return 0;

    BSP_SOFTWARE_I2C_Init(&handle->i2c,
                          soft_i2c_scl_GPIO_PORT, soft_i2c_scl_PIN,
                          soft_i2c_sda_GPIO_PORT, soft_i2c_sda_PIN);
    if (handle->i2c.scl_port == NULL || handle->i2c.sda_port == NULL) return 0;
    handle->address = address;

    BSP_SOFTWARE_I2C_ReadReg(&handle->i2c, address, MPU6050_WHO_AM_I_REG, &value);
    if (value != MPU6050_DEVICE_ID) return 0;

    BSP_SOFTWARE_I2C_WriteReg(&handle->i2c, address, MPU6050_PWR_MGMT_1_REG, 0x80U);
    time_delay_ms(100);
    /* 唤醒并选择 X 轴陀螺仪 PLL 时钟，开启全部传感器轴及温度测量。 */
    BSP_SOFTWARE_I2C_WriteReg(&handle->i2c, address, MPU6050_PWR_MGMT_1_REG, 0x01U);
    BSP_SOFTWARE_I2C_WriteReg(&handle->i2c, address, MPU6050_PWR_MGMT_2_REG, 0x00U);
    time_delay_ms(100);
    /* 从 0x19 起依次为采样分频、低通滤波、陀螺仪量程、加速度量程。 */
    BSP_SOFTWARE_I2C_MemWrite(&handle->i2c, address, MPU6050_SMPLRT_DIV_REG,
                             config, sizeof(config));

    value = 0xFFU;
    BSP_SOFTWARE_I2C_ReadReg(&handle->i2c, address, MPU6050_PWR_MGMT_1_REG, &value);
    if (value != 0x01U) return 0;
    value = 0xFFU;
    BSP_SOFTWARE_I2C_ReadReg(&handle->i2c, address, MPU6050_PWR_MGMT_2_REG, &value);
    if (value != 0x00U) return 0;
    BSP_SOFTWARE_I2C_MemRead(&handle->i2c, address, MPU6050_SMPLRT_DIV_REG,
                            readback, sizeof(readback));
    for (i = 0; i < sizeof(config); i++)
    {
        if (readback[i] != config[i]) return 0;
    }
    handle->initialized = 1;
    return 1;
}

void MPU6050_SOFTWARE_I2C_ReadData(MPU6050_SOFTWARE_I2C_Handle_t *handle,
                                  MPU6050_Data_t *data)
{
    uint8_t buffer[14] = {0};

    if (handle == NULL || !handle->initialized || data == NULL) return;

    BSP_SOFTWARE_I2C_MemRead(&handle->i2c, handle->address, MPU6050_ACCEL_XOUT_REG,
                            buffer, sizeof(buffer));
    data->accel_x = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[0]) / 16384.0f;
    data->accel_y = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[2]) / 16384.0f;
    data->accel_z = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[4]) / 16384.0f;
    data->temperature = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[6]) / 340.0f + 36.53f;
    data->gyro_x = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[8]) / 131.0f;
    data->gyro_y = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[10]) / 131.0f;
    data->gyro_z = (float)MPU6050_SOFTWARE_I2C_ReadInt16(&buffer[12]) / 131.0f;
}
