/***************************************************************
 * bsp_software_i2c.c
 * 软件 I2C 主机实现，所有平台操作通过 bsp_software_i2c_platform.h
 ***************************************************************/
#include "bsp_software_i2c.h"
#include "bsp_software_i2c_platform.h"
#include <stddef.h>

static uint8_t BSP_SOFTWARE_I2C_ValidPin(uint16_t pin)
{
    return pin != 0 && (pin & (pin - 1U)) == 0;
}

static uint8_t BSP_SOFTWARE_I2C_Valid(const BSP_SOFTWARE_I2C_Handle_t *handle)
{
    return handle != NULL && BSP_SOFTWARE_I2C_VALID_PORT(handle->scl_port)
        && BSP_SOFTWARE_I2C_VALID_PORT(handle->sda_port)
        && BSP_SOFTWARE_I2C_ValidPin(handle->scl_pin)
        && BSP_SOFTWARE_I2C_ValidPin(handle->sda_pin)
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

/* 每个高电平阶段先等待 SCL 实际变高，支持从机时钟拉伸。
 * 无符号时间差允许微秒时间戳回绕；不依赖未初始化的循环计数。
 */
static BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_RaiseScl(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    uint32_t start_us = BSP_SOFTWARE_I2C_GET_US();
    BSP_SOFTWARE_I2C_RELEASE(handle->scl_port, handle->scl_pin);
    while (!BSP_SOFTWARE_I2C_READ(handle->scl_port, handle->scl_pin)) {
        if ((uint32_t)(BSP_SOFTWARE_I2C_GET_US() - start_us) >= handle->timeout_us) {
            BSP_SOFTWARE_I2C_Release(handle);
            return BSP_SOFTWARE_I2C_TIMEOUT;
        }
        BSP_SOFTWARE_I2C_DELAY_US(1U);
    }
    BSP_SOFTWARE_I2C_Delay(handle);
    return BSP_SOFTWARE_I2C_OK;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Init(BSP_SOFTWARE_I2C_Handle_t *handle,
    void *scl_port, uint16_t scl_pin, void *sda_port, uint16_t sda_pin)
{
    if (handle == NULL) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    handle->scl_port = NULL;
    handle->sda_port = NULL;
    if (!BSP_SOFTWARE_I2C_VALID_PORT(scl_port) || !BSP_SOFTWARE_I2C_VALID_PORT(sda_port)
        || !BSP_SOFTWARE_I2C_ValidPin(scl_pin) || !BSP_SOFTWARE_I2C_ValidPin(sda_pin)
        || (scl_port == sda_port && scl_pin == sda_pin)
        || BSP_SOFTWARE_I2C_HALF_PERIOD_US == 0 || BSP_SOFTWARE_I2C_TIMEOUT_US == 0) {
        return BSP_SOFTWARE_I2C_INVALID_PARAM;
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
    return BSP_SOFTWARE_I2C_OK;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Start(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    status = BSP_SOFTWARE_I2C_RaiseScl(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    if (!BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin)) {
        BSP_SOFTWARE_I2C_Release(handle);
        return BSP_SOFTWARE_I2C_BUS_BUSY;
    }
    BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    return BSP_SOFTWARE_I2C_OK;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Stop(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    status = BSP_SOFTWARE_I2C_RaiseScl(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    return BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin)
        ? BSP_SOFTWARE_I2C_OK : BSP_SOFTWARE_I2C_BUS_BUSY;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_WaitAck(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    /* 在第 9 个时钟上升沿之前释放 SDA，避免与从机应答冲突。 */
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    status = BSP_SOFTWARE_I2C_RaiseScl(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    status = BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin)
        ? BSP_SOFTWARE_I2C_NACK : BSP_SOFTWARE_I2C_OK;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    return status;
}

static BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_SendAck(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t ack)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    if (ack) BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
    else BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    BSP_SOFTWARE_I2C_Delay(handle);
    status = BSP_SOFTWARE_I2C_RaiseScl(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    return BSP_SOFTWARE_I2C_OK;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Ack(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    return BSP_SOFTWARE_I2C_SendAck(handle, 1);
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_NAck(BSP_SOFTWARE_I2C_Handle_t *handle)
{
    return BSP_SOFTWARE_I2C_SendAck(handle, 0);
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_SendByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t data)
{
    uint8_t bit;
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle)) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    for (bit = 0; bit < 8; bit++) {
        if (data & 0x80U) BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
        else BSP_SOFTWARE_I2C_LOW(handle->sda_port, handle->sda_pin);
        BSP_SOFTWARE_I2C_Delay(handle);
        status = BSP_SOFTWARE_I2C_RaiseScl(handle);
        if (status != BSP_SOFTWARE_I2C_OK) return status;
        BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
        data <<= 1;
    }
    return BSP_SOFTWARE_I2C_OK;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_ReadByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t ack, uint8_t *data)
{
    uint8_t bit, value = 0;
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle) || data == NULL) return BSP_SOFTWARE_I2C_INVALID_PARAM;
    BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    BSP_SOFTWARE_I2C_RELEASE(handle->sda_port, handle->sda_pin);
    for (bit = 0; bit < 8; bit++) {
        BSP_SOFTWARE_I2C_Delay(handle);
        status = BSP_SOFTWARE_I2C_RaiseScl(handle);
        if (status != BSP_SOFTWARE_I2C_OK) return status;
        value = (uint8_t)((value << 1) | BSP_SOFTWARE_I2C_READ(handle->sda_port, handle->sda_pin));
        BSP_SOFTWARE_I2C_LOW(handle->scl_port, handle->scl_pin);
    }
    status = BSP_SOFTWARE_I2C_SendAck(handle, ack);
    if (status == BSP_SOFTWARE_I2C_OK) *data = value;
    return status;
}

static BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_WriteByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t value)
{
    BSP_SOFTWARE_I2C_Status_t status = BSP_SOFTWARE_I2C_SendByte(handle, value);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    return BSP_SOFTWARE_I2C_WaitAck(handle);
}

/* 保留最初的通信错误；若通信成功，返回 STOP 的结果。 */
static BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_End(BSP_SOFTWARE_I2C_Handle_t *handle,
    BSP_SOFTWARE_I2C_Status_t status)
{
    BSP_SOFTWARE_I2C_Status_t stop_status = BSP_SOFTWARE_I2C_Stop(handle);
    return status == BSP_SOFTWARE_I2C_OK ? stop_status : status;
}

static BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_SelectReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg)
{
    BSP_SOFTWARE_I2C_Status_t status = BSP_SOFTWARE_I2C_WriteByte(handle, (uint8_t)(address << 1));
    if (status == BSP_SOFTWARE_I2C_OK) status = BSP_SOFTWARE_I2C_WriteByte(handle, reg);
    return status;
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_MemWrite(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, const uint8_t *data, uint16_t size)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle) || address > 0x7f || data == NULL || size == 0) {
        return BSP_SOFTWARE_I2C_INVALID_PARAM;
    }
    status = BSP_SOFTWARE_I2C_Start(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    status = BSP_SOFTWARE_I2C_SelectReg(handle, address, reg);
    while (status == BSP_SOFTWARE_I2C_OK && size > 0) {
        status = BSP_SOFTWARE_I2C_WriteByte(handle, *data++);
        size--;
    }
    return BSP_SOFTWARE_I2C_End(handle, status);
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_MemRead(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *data, uint16_t size)
{
    BSP_SOFTWARE_I2C_Status_t status;
    if (!BSP_SOFTWARE_I2C_Valid(handle) || address > 0x7f || data == NULL || size == 0) {
        return BSP_SOFTWARE_I2C_INVALID_PARAM;
    }
    status = BSP_SOFTWARE_I2C_Start(handle);
    if (status != BSP_SOFTWARE_I2C_OK) return status;
    status = BSP_SOFTWARE_I2C_SelectReg(handle, address, reg);
    if (status == BSP_SOFTWARE_I2C_OK) status = BSP_SOFTWARE_I2C_Start(handle);
    if (status == BSP_SOFTWARE_I2C_OK) {
        status = BSP_SOFTWARE_I2C_WriteByte(handle, (uint8_t)((address << 1) | 1U));
    }
    while (status == BSP_SOFTWARE_I2C_OK && size > 0) {
        status = BSP_SOFTWARE_I2C_ReadByte(handle, size > 1, data++);
        size--;
    }
    return BSP_SOFTWARE_I2C_End(handle, status);
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_WriteReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t value)
{
    return BSP_SOFTWARE_I2C_MemWrite(handle, address, reg, &value, 1);
}

BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_ReadReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *value)
{
    return BSP_SOFTWARE_I2C_MemRead(handle, address, reg, value, 1);
}
