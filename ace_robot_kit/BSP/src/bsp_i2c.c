/***************************************************************
 * bsp_i2c.c
 * I2C 寄存器读写，所有平台操作通过 bsp_i2c_platform.h
 ***************************************************************/
#include "bsp_i2c.h"
#include "bsp_i2c_platform.h"
#include <stddef.h>


/* 等待硬件标志，返回是否就绪，不返回通信状态码。 */
static uint8_t BSP_I2C_WaitFlag(void *bus, uint32_t flag, uint8_t expected, uint32_t start_ms)
{
    while (BSP_I2C_FLAG(bus, flag) != expected) {
        if ((uint32_t)(BSP_I2C_GET_MS() - start_ms) >= BSP_I2C_TIMEOUT_MS) {
            return 0;
        }
    }
    return 1;
}

static uint8_t BSP_I2C_SendAddress(void *bus, uint8_t address, uint8_t read, uint32_t start_ms)
{
    BSP_I2C_START(bus);
    if (!BSP_I2C_WaitFlag(bus, BSP_I2C_START_FLAG, 1, start_ms)) return 0;
    BSP_I2C_SEND_ADDRESS(bus, address, read);
    return BSP_I2C_WaitFlag(bus, BSP_I2C_ADDRESS_FLAG, 1, start_ms);
}

/* 指定寄存器，读写接口共用。 */
static uint8_t BSP_I2C_SelectReg(void *bus, uint8_t address, uint8_t reg, uint32_t start_ms)
{
    if (!BSP_I2C_WaitFlag(bus, BSP_I2C_BUSY_FLAG, 0, start_ms)) return 0;
    BSP_I2C_ACK_NEXT(bus, 0);
    BSP_I2C_ACK_ENABLE(bus, 1);
    if (!BSP_I2C_SendAddress(bus, address, 0, start_ms)) return 0;
    BSP_I2C_CLEAR_ADDRESS(bus);
    if (!BSP_I2C_WaitFlag(bus, BSP_I2C_TX_EMPTY_FLAG, 1, start_ms)) return 0;
    BSP_I2C_SEND_BYTE(bus, reg);
    return BSP_I2C_WaitFlag(bus, BSP_I2C_TX_DONE_FLAG, 1, start_ms);
}

static void BSP_I2C_ReadData(void *bus, uint8_t *data, uint16_t size, uint32_t start_ms)
{
    uint32_t primask = BSP_I2C_ENTER_CRITICAL();
    /* 1、2 字节需在清除地址标志前配置 ACK。 */
    if (size <= 2) {
        BSP_I2C_ACK_NEXT(bus, size == 2);
        BSP_I2C_ACK_ENABLE(bus, 0);
    }
    BSP_I2C_CLEAR_ADDRESS(bus);
    if (size == 1) BSP_I2C_STOP(bus);
    BSP_I2C_EXIT_CRITICAL(primask);

    while (size > 3) {
        if (!BSP_I2C_WaitFlag(bus, BSP_I2C_RX_READY_FLAG, 1, start_ms)) return;
        *data++ = BSP_I2C_READ_BYTE(bus);
        size--;
    }
    if (size == 1) {
        if (BSP_I2C_WaitFlag(bus, BSP_I2C_RX_READY_FLAG, 1, start_ms)) {
            *data = BSP_I2C_READ_BYTE(bus);
        }
        return;
    }

    if (size == 3) {
        if (!BSP_I2C_WaitFlag(bus, BSP_I2C_TX_DONE_FLAG, 1, start_ms)) return;
        primask = BSP_I2C_ENTER_CRITICAL();
        BSP_I2C_ACK_ENABLE(bus, 0);
        *data++ = BSP_I2C_READ_BYTE(bus);
        BSP_I2C_EXIT_CRITICAL(primask);
    }
    /* 最后两个字节就绪后，发 STOP 并连续读出。 */
    if (!BSP_I2C_WaitFlag(bus, BSP_I2C_TX_DONE_FLAG, 1, start_ms)) return;
    primask = BSP_I2C_ENTER_CRITICAL();
    BSP_I2C_STOP(bus);
    *data++ = BSP_I2C_READ_BYTE(bus);
    *data = BSP_I2C_READ_BYTE(bus);
    BSP_I2C_EXIT_CRITICAL(primask);
}

static void BSP_I2C_End(void *bus, uint32_t start_ms)
{
    uint32_t primask = BSP_I2C_ENTER_CRITICAL();
    BSP_I2C_ACK_ENABLE(bus, 0);
    if (BSP_I2C_IS_MASTER(bus) && !BSP_I2C_STOP_PENDING(bus)) {
        BSP_I2C_STOP(bus);
    }
    BSP_I2C_EXIT_CRITICAL(primask);
    while (BSP_I2C_STOP_PENDING(bus)) {
        if ((uint32_t)(BSP_I2C_GET_MS() - start_ms) >= BSP_I2C_TIMEOUT_MS) break;
    }
    BSP_I2C_CLEAR_FLAGS(bus);
    if (!BSP_I2C_STOP_PENDING(bus)) {
        BSP_I2C_ACK_NEXT(bus, 0);
        BSP_I2C_ACK_ENABLE(bus, 1);
    }
}

void BSP_I2C_Init(BSP_I2C_Handle_t *handle, void *i2c)
{
    if (handle == NULL) return;
    handle->i2c_handle = BSP_I2C_VALID_BUS(i2c) ? i2c : NULL;
}

void BSP_I2C_MemWrite(BSP_I2C_Handle_t *handle, uint8_t address,
                      uint8_t reg, const uint8_t *data, uint16_t size)
{
    if (handle == NULL || handle->i2c_handle == NULL || data == NULL
        || size == 0 || address > 0x7f || !BSP_I2C_CAN_TRANSFER()) return;

    void *bus = handle->i2c_handle;
    uint32_t start_ms = BSP_I2C_GET_MS();
    if (BSP_I2C_SelectReg(bus, address, reg, start_ms)) {
        while (size > 0 && BSP_I2C_WaitFlag(bus, BSP_I2C_TX_EMPTY_FLAG, 1, start_ms)) {
            BSP_I2C_SEND_BYTE(bus, *data++);
            size--;
        }
        if (size == 0) {
            BSP_I2C_WaitFlag(bus, BSP_I2C_TX_DONE_FLAG, 1, start_ms);
        }
    }
    BSP_I2C_End(bus, start_ms);
}

void BSP_I2C_MemRead(BSP_I2C_Handle_t *handle, uint8_t address,
                     uint8_t reg, uint8_t *data, uint16_t size)
{
    if (handle == NULL || handle->i2c_handle == NULL || data == NULL
        || size == 0 || address > 0x7f || !BSP_I2C_CAN_TRANSFER()) return;

    void *bus = handle->i2c_handle;
    uint32_t start_ms = BSP_I2C_GET_MS();
    if (BSP_I2C_SelectReg(bus, address, reg, start_ms)
        && BSP_I2C_SendAddress(bus, address, 1, start_ms)) {
        BSP_I2C_ReadData(bus, data, size, start_ms);
    }
    BSP_I2C_End(bus, start_ms);
}

void BSP_I2C_WriteReg(BSP_I2C_Handle_t *handle, uint8_t address, uint8_t reg, uint8_t value)
{
    BSP_I2C_MemWrite(handle, address, reg, &value, 1);
}

void BSP_I2C_ReadReg(BSP_I2C_Handle_t *handle, uint8_t address, uint8_t reg, uint8_t *value)
{
    BSP_I2C_MemRead(handle, address, reg, value, 1);
}
