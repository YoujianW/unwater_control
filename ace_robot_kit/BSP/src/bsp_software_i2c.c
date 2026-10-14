/***************************************************************
 * bsp_software_i2c.c
 * 软件 I2C 主机实现，所有平台操作通过 bsp_software_i2c_platform.h
 ***************************************************************/
#include "bsp_software_i2c.h"
#include "bsp_software_i2c_platform.h"
#include <stddef.h>

static uint8_t BSP_SOFTWARE_I2C_Valid(const BSP_SOFTWARE_I2C_Handle_t *handle)
{
    return handle != NULL && BSP_SOFTWARE_I2C_VALID_PORT(handle->scl_port)
        && BSP_SOFTWARE_I2C_VALID_PORT(handle->sda_port)
        && BSP_SOFTWARE_I2C_VALID_PIN(handle->scl_pin)
        && BSP_SOFTWARE_I2C_VALID_PIN(handle->sda_pin)
        && (handle->scl_port != handle->sda_port || handle->scl_pin != handle->sda_pin)
        && handle->half_period_us != 0 && handle->timeout_us != 0;
}

static void BSP_SOFTWARE_I2C_Delay(const BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_DELAY_US(handle->half_period_us);
}

static void BSP_SOFTWARE_I2C_Release(const BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_RELEASE(handle->scl_port, handle->scl_pin);
}

/* 等待 SCL 实际变高，支持时钟拉伸；返回 1 表示就绪，0 表示超时。
 * 无符号时间差允许微秒时间戳回绕。
 */
static uint8_t BSP_SOFTWARE_I2C_RaiseScl(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    uint32_t start_us = BSP_SOFTWARE_I2C_GET_US();
    BSP_SOFTWARE_I2C_RELEASE(handle->scl_port, handle->scl_pin);
    while (!BSP_SOFTWARE_I2C_READ(handle->scl_port, handle->scl_pin)) {
        if ((uint32_t)(BSP_SOFTWARE_I2C_GET_US() - start_us) >= handle->timeout_us) {
            BSP_SOFTWARE_I2C_Release(handle);
            return 0;
        }
        BSP_SOFTWARE_I2C_DELAY_US(1U);
    }
    BSP_SOFTWARE_I2C_Delay(handle);
    return 1;
}

void BSP_SOFTWARE_I2C_Init(BSP_SOFTWARE_I2C_Handle_t *handle,
    void *scl_port, uint16_t scl_pin, void *sda_port, uint16_t sda_pin)
{
    if (handle == NULL) return;
    handle->scl_port = NULL;
    handle->sda_port = NULL;
    if (!BSP_SOFTWARE_I2C_VALID_PORT(scl_port) || !BSP_SOFTWARE_I2C_VALID_PORT(sda_port)
        || !BSP_SOFTWARE_I2C_VALID_PIN(scl_pin) || !BSP_SOFTWARE_I2C_VALID_PIN(sda_pin)
        || (scl_port == sda_port && scl_pin == sda_pin)
        || BSP_SOFTWARE_I2C_HALF_PERIOD_US == 0 || BSP_SOFTWARE_I2C_TIMEOUT_US == 0) {
        return;
    }
    handle->scl_port = scl_port;
    handle->sda_port = sda_port;
    handle->scl_pin = scl_pin;
    handle->sda_pin = sda_pin;
    handle->half_period_us = BSP_SOFTWARE_I2C_HALF_PERIOD_US;
    handle->timeout_us = BSP_SOFTWARE_I2C_TIMEOUT_US;
    BSP_SOFTWARE_I2C_INIT_PIN(scl_port, scl_pin);
    BSP_SOFTWARE_I2C_INIT_PIN(sda_port, sda_pin);
    BSP_SOFTWARE_I2C_Release(handle);
}

/* 内部返回是否产生 START，供寄存器接口在总线异常时中止事务。 */
static uint8_t BSP_SOFTWARE_I2C_StartBus(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return 0;
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return 0;
    if (!BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin)) {
        BSP_SOFTWARE_I2C_Release(handle);
        return 0;
    }
    BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    return 1;
}

void BSP_SOFTWARE_I2C_Start(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_StartBus(handle);
}

void BSP_SOFTWARE_I2C_Stop(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return;
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
}

uint8_t BSP_SOFTWARE_I2C_WaitAck(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    uint8_t nack;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return 1;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    /* 在第 9 个时钟上升沿之前释放 SDA，避免与从机应答冲突。 */
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return 1;
    nack = BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin) ? 1 : 0;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    return nack;
}

static uint8_t BSP_SOFTWARE_I2C_SendAck(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t ack)
{
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return 0;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    if (ack) BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    else BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return 0;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    return 1;
}

void BSP_SOFTWARE_I2C_Ack(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_SendAck(handle, 1);
}

void BSP_SOFTWARE_I2C_NAck(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_SendAck(handle, 0);
}

/* 内部返回是否发送完成，防止超时后继续发送后续字节。 */
static uint8_t BSP_SOFTWARE_I2C_SendData(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t data)
{
    uint8_t bit;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return 0;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    for (bit = 0; bit < 8; bit++) {
        if (data & 0x80U) BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
        else BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
        BSP_SOFTWARE_I2C_Delay(handle);
        if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return 0;
        BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
        data <<= 1;
    }
    return 1;
}

void BSP_SOFTWARE_I2C_SendByte(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t data)
{
    BSP_SOFTWARE_I2C_SendData(handle, data);
}

/* 内部返回是否读取完成，仅在完整收到字节并发送应答后写入缓冲区。 */
static uint8_t BSP_SOFTWARE_I2C_ReadData(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t ack, uint8_t *data)
{
    uint8_t bit, value = 0;
    if (!BSP_SOFTWARE_I2C_Valid(handle) || data == NULL) return 0;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    for (bit = 0; bit < 8; bit++) {
        BSP_SOFTWARE_I2C_Delay(handle);
        if (!BSP_SOFTWARE_I2C_RaiseScl(handle)) return 0;
        value = (uint8_t)((value << 1) | BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin));
        BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    }
    if (!BSP_SOFTWARE_I2C_SendAck(handle, ack)) return 0;
    *data = value;
    return 1;
}

uint8_t BSP_SOFTWARE_I2C_ReadByte(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t ack)
{
    uint8_t data = 0;
    BSP_SOFTWARE_I2C_ReadData(handle, ack, &data);
    return data;
}

static uint8_t BSP_SOFTWARE_I2C_WriteByte(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t value)
{
    return BSP_SOFTWARE_I2C_SendData(handle, value) && BSP_SOFTWARE_I2C_WaitAck(handle) == 0;
}

static uint8_t BSP_SOFTWARE_I2C_SelectReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg)
{
    return BSP_SOFTWARE_I2C_WriteByte(handle, (uint8_t)(address << 1))
        && BSP_SOFTWARE_I2C_WriteByte(handle, reg);
}

void BSP_SOFTWARE_I2C_MemWrite(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, const uint8_t *data, uint16_t size)
{
    if (!BSP_SOFTWARE_I2C_Valid(handle) || address > 0x7f || data == NULL || size == 0) return;
    if (!BSP_SOFTWARE_I2C_StartBus(handle)) return;
    if (BSP_SOFTWARE_I2C_SelectReg(handle, address, reg)) {
        while (size > 0) {
            if (!BSP_SOFTWARE_I2C_WriteByte(handle, *data++)) break;
            size--;
        }
    }
    BSP_SOFTWARE_I2C_Stop(handle);
}

void BSP_SOFTWARE_I2C_MemRead(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *data, uint16_t size)
{
    if (!BSP_SOFTWARE_I2C_Valid(handle) || address > 0x7f || data == NULL || size == 0) return;
    if (!BSP_SOFTWARE_I2C_StartBus(handle)) return;
    if (BSP_SOFTWARE_I2C_SelectReg(handle, address, reg)
        && BSP_SOFTWARE_I2C_StartBus(handle)
        && BSP_SOFTWARE_I2C_WriteByte(handle, (uint8_t)((address << 1) | 1U))) {
        while (size > 0) {
            if (!BSP_SOFTWARE_I2C_ReadData(handle, size > 1, data++)) break;
            size--;
        }
    }
    BSP_SOFTWARE_I2C_Stop(handle);
}

void BSP_SOFTWARE_I2C_WriteReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t value)
{
    BSP_SOFTWARE_I2C_MemWrite(handle, address, reg, &value, 1);
}

void BSP_SOFTWARE_I2C_ReadReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *value)
{
    BSP_SOFTWARE_I2C_MemRead(handle, address, reg, value, 1);
}
