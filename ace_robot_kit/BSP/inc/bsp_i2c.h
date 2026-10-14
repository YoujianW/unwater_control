/***************************************************************
 * bsp_i2c.h
 * I2C 寄存器读写通用接口（平台无关）
 ***************************************************************/
#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_I2C_TIMEOUT_MS
#define BSP_I2C_TIMEOUT_MS   20U
#endif


typedef struct {
    void *i2c_handle;         /* I2C 外设基地址，由 APP 传入 */
} BSP_I2C_Handle_t;

/**
 * @brief 初始化 I2C 句柄，仅绑定外设，不修改硬件配置
 * @param handle  句柄指针
 * @param i2c     I2C 外设基地址（如 I2C1）
 */
void BSP_I2C_Init(BSP_I2C_Handle_t *handle, void *i2c);

/**
 * @brief 向指定寄存器连续写入数据（阻塞）
 * @param handle   已初始化的 I2C 句柄指针
 * @param address  未左移的 7 位设备地址
 * @param reg      8 位寄存器起始地址
 * @param data     待发送的数据缓冲区
 * @param size     写入字节数，必须大于 0
 * @note 寄存器地址只发送一次，连续写入方式由从机协议决定
 */
void BSP_I2C_MemWrite(BSP_I2C_Handle_t *handle, uint8_t address,
                      uint8_t reg, const uint8_t *data, uint16_t size);

/**
 * @brief 从指定寄存器连续读取数据（阻塞）
 * @param handle   已初始化的 I2C 句柄指针
 * @param address  未左移的 7 位设备地址
 * @param reg      8 位寄存器起始地址
 * @param data     接收数据的缓冲区，容量至少为 size 字节
 * @param size     读取字节数，必须大于 0
 * @note 发送寄存器地址后使用重复 START 切换到读取，支持单字节和多字节
 */
void BSP_I2C_MemRead(BSP_I2C_Handle_t *handle, uint8_t address,
                     uint8_t reg, uint8_t *data, uint16_t size);

/**
 * @brief 向指定寄存器写入单字节数据（阻塞）
 * @param handle   已初始化的 I2C 句柄指针
 * @param address  未左移的 7 位设备地址
 * @param reg      8 位寄存器地址
 * @param value    要写入的单字节值
 */
void BSP_I2C_WriteReg(BSP_I2C_Handle_t *handle, uint8_t address,
                      uint8_t reg, uint8_t value);

/**
 * @brief 从指定寄存器读取单字节数据（阻塞）
 * @param handle   已初始化的 I2C 句柄指针
 * @param address  未左移的 7 位设备地址
 * @param reg      8 位寄存器地址
 * @param value    存放读取结果的单字节变量指针
 */
void BSP_I2C_ReadReg(BSP_I2C_Handle_t *handle, uint8_t address,
                     uint8_t reg, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
