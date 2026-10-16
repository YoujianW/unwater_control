/***************************************************************
 * bsp_spi.c
 * SPI 主机阻塞收发，所有平台操作通过 bsp_spi_platform.h
 ***************************************************************/
#include "bsp_spi.h"
#include "bsp_spi_platform.h"
#include <stddef.h>


static uint8_t BSP_SPI_IsValid(const BSP_SPI_Handle_t *handle)
{
    return handle != NULL && BSP_SPI_VALID_BUS(handle->spi_handle)
        && BSP_SPI_VALID_PORT(handle->cs_port) && BSP_SPI_VALID_PIN(handle->cs_pin);
}

/* 整个数据块共用起始时间，硬件错误和超时均提前结束。 */
static uint8_t BSP_SPI_WaitFlag(void *bus, uint32_t flag, uint8_t expected, uint32_t start_ms)
{
    while (1) {
        if (BSP_SPI_FLAG(bus, BSP_SPI_ERROR_FLAGS)
            || (uint32_t)(BSP_SPI_GET_MS() - start_ms) >= BSP_SPI_TIMEOUT_MS) {
            return 0;
        }
        if (BSP_SPI_FLAG(bus, flag) == expected) return 1;
    }
}

void BSP_SPI_Init(BSP_SPI_Handle_t *handle,
    void *spi, void *cs_port, uint16_t cs_pin)
{
    if (handle == NULL) return;
    handle->spi_handle = NULL;
    handle->cs_port = NULL;
    handle->cs_pin = 0;
    if (!BSP_SPI_VALID_BUS(spi) || !BSP_SPI_VALID_PORT(cs_port)
        || !BSP_SPI_VALID_PIN(cs_pin)) return;

    handle->spi_handle = spi;
    handle->cs_port = cs_port;
    handle->cs_pin = cs_pin;
    BSP_SPI_CS_HIGH(cs_port, cs_pin);
}

void BSP_SPI_CS_Low(BSP_SPI_Handle_t *handle)
{
    if (!BSP_SPI_IsValid(handle)) return;
    BSP_SPI_CS_LOW(handle->cs_port, handle->cs_pin);
}

void BSP_SPI_CS_High(BSP_SPI_Handle_t *handle)
{
    if (!BSP_SPI_IsValid(handle)) return;
    BSP_SPI_CS_HIGH(handle->cs_port, handle->cs_pin);
}

uint8_t BSP_SPI_Transfer(BSP_SPI_Handle_t *handle,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    if (!BSP_SPI_IsValid(handle) || (tx_data == NULL && rx_data == NULL)
        || size == 0 || !BSP_SPI_CAN_TRANSFER()) return 0;

    void *bus = handle->spi_handle;
    if (!BSP_SPI_BUS_READY(bus)) return 0;

    uint32_t start_ms = BSP_SPI_GET_MS();
    /* 清除上次中止传输的残留数据，再等待硬件空闲。 */
    BSP_SPI_CLEAR_RX(bus);
    if (!BSP_SPI_WaitFlag(bus, BSP_SPI_BUSY_FLAG, 0, start_ms)) {
        BSP_SPI_CLEAR_RX(bus);
        return 0;
    }
    BSP_SPI_CLEAR_RX(bus);

    while (size > 0) {
        if (!BSP_SPI_WaitFlag(bus, BSP_SPI_TX_EMPTY_FLAG, 1, start_ms)) {
            BSP_SPI_CLEAR_RX(bus);
            return 0;
        }
        BSP_SPI_SEND_BYTE(bus, tx_data != NULL ? *tx_data++ : BSP_SPI_DUMMY_BYTE);
        if (!BSP_SPI_WaitFlag(bus, BSP_SPI_RX_READY_FLAG, 1, start_ms)) {
            BSP_SPI_CLEAR_RX(bus);
            return 0;
        }

        /* 全双工每发送一字节都必须读取一字节，纯发送时也不能省略。 */
        uint8_t value = BSP_SPI_READ_BYTE(bus);
        if (rx_data != NULL) *rx_data++ = value;
        size--;
    }
    if (!BSP_SPI_WaitFlag(bus, BSP_SPI_TX_EMPTY_FLAG, 1, start_ms)
        || !BSP_SPI_WaitFlag(bus, BSP_SPI_BUSY_FLAG, 0, start_ms)) {
        BSP_SPI_CLEAR_RX(bus);
        return 0;
    }
    return 1;
}

uint8_t BSP_SPI_SendBuffer(BSP_SPI_Handle_t *handle, const uint8_t *data, uint16_t size)
{
    if (data == NULL) return 0;
    return BSP_SPI_Transfer(handle, data, NULL, size);
}

uint8_t BSP_SPI_ReceiveBuffer(BSP_SPI_Handle_t *handle, uint8_t *data, uint16_t size)
{
    if (data == NULL) return 0;
    return BSP_SPI_Transfer(handle, NULL, data, size);
}

uint8_t BSP_SPI_ReadWriteByte(BSP_SPI_Handle_t *handle, uint8_t data)
{
    uint8_t value = 0;
    if (!BSP_SPI_Transfer(handle, &data, &value, 1)) return 0;
    return value;
}
