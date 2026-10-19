/***************************************************************
 * w25q16.c
 * W25Q16 读写及擦除，所有 SPI 操作通过 bsp_spi.h
 ***************************************************************/
#include "w25q16.h"
#include "time_core.h"
#include <stddef.h>

#define W25Q16_CMD_WRITE_ENABLE  0x06U
#define W25Q16_CMD_READ_STATUS   0x05U
#define W25Q16_CMD_READ_ID       0x9FU
#define W25Q16_CMD_READ_DATA     0x03U
#define W25Q16_CMD_PAGE_PROGRAM  0x02U
#define W25Q16_CMD_SECTOR_ERASE  0x20U
#define W25Q16_CMD_CHIP_ERASE    0xC7U
#define W25Q16_CMD_RELEASE_POWER 0xABU

static uint8_t W25Q16_IsValid(const W25Q16_Handle_t *handle)
{
    return handle != NULL && handle->spi != NULL
        && handle->spi->spi_handle != NULL && handle->spi->cs_port != NULL
        && handle->spi->cs_pin != 0;
}

/* 先减后比较，避免 address + size 溢出绕过容量检查。 */
static uint8_t W25Q16_IsRangeValid(uint32_t address, uint32_t size)
{
    return size > 0 && address < W25Q16_CAPACITY
        && size <= W25Q16_CAPACITY - address;
}

static void W25Q16_MakeCommand(uint8_t *command, uint8_t instruction, uint32_t address)
{
    command[0] = instruction;
    command[1] = (uint8_t)(address >> 16);
    command[2] = (uint8_t)(address >> 8);
    command[3] = (uint8_t)address;
}

/* 一次片选内发送命令及数据；分段收发避免大块读取占满 BSP 的单次超时时间。 */
static uint8_t W25Q16_Transaction(W25Q16_Handle_t *handle,
    const uint8_t *command, uint16_t command_size,
    const uint8_t *tx_data, uint8_t *rx_data, uint32_t size)
{
    if (!W25Q16_IsValid(handle) || command == NULL || command_size == 0
        || (size > 0 && tx_data == NULL && rx_data == NULL)) return 0;

    BSP_SPI_CS_Low(handle->spi);
    time_delay_us(1);
    uint8_t ok = BSP_SPI_SendBuffer(handle->spi, command, command_size);
    while (ok && size > 0) {
        uint16_t chunk = size > W25Q16_PAGE_SIZE ? W25Q16_PAGE_SIZE : (uint16_t)size;
        ok = BSP_SPI_Transfer(handle->spi, tx_data, rx_data, chunk);
        if (tx_data != NULL) tx_data += chunk;
        if (rx_data != NULL) rx_data += chunk;
        size -= chunk;
    }
    /* 即使命令或数据发送失败，也结束当前指令并释放片选。 */
    BSP_SPI_CS_High(handle->spi);
    time_delay_us(1);
    return ok;
}

static uint8_t W25Q16_WriteEnable(W25Q16_Handle_t *handle)
{
    const uint8_t command = W25Q16_CMD_WRITE_ENABLE;
    uint8_t status = 0;

    if (!W25Q16_WaitReady(handle, W25Q16_BUSY_TIMEOUT_MS)) return 0;
    if (!W25Q16_Transaction(handle, &command, 1, NULL, NULL, 0)) return 0;
    if (!W25Q16_ReadStatus(handle, &status)) return 0;
    return (status & W25Q16_STATUS_WEL) != 0;
}

/* 擦写完成后 WEL 应自动清零，仍为 1 时视为指令未生效。 */
static uint8_t W25Q16_WaitWriteDone(W25Q16_Handle_t *handle, uint32_t timeout_ms)
{
    uint8_t status = 0;

    if (!W25Q16_WaitReady(handle, timeout_ms)) return 0;
    if (!W25Q16_ReadStatus(handle, &status)) return 0;
    return (status & W25Q16_STATUS_WEL) == 0;
}

uint8_t W25Q16_Init(W25Q16_Handle_t *handle, BSP_SPI_Handle_t *spi)
{
    const uint8_t command = W25Q16_CMD_RELEASE_POWER;
    uint32_t id = 0;

    if (handle == NULL) return 0;
    handle->spi = NULL;
    if (spi == NULL || spi->spi_handle == NULL || spi->cs_port == NULL
        || spi->cs_pin == 0) return 0;

    handle->spi = spi;
    BSP_SPI_CS_High(spi);
    time_delay_ms(10);
    uint8_t ok = W25Q16_Transaction(handle, &command, 1, NULL, NULL, 0);
    time_delay_us(5);
    if (!ok || !W25Q16_WaitReady(handle, W25Q16_BUSY_TIMEOUT_MS)
        || !W25Q16_ReadID(handle, &id) || id != W25Q16_JEDEC_ID) {
        handle->spi = NULL;
        return 0;
    }
    return 1;
}

uint8_t W25Q16_ReadID(W25Q16_Handle_t *handle, uint32_t *id)
{
    const uint8_t command = W25Q16_CMD_READ_ID;
    uint8_t data[3] = {0};

    if (id == NULL) return 0;
    if (!W25Q16_Transaction(handle, &command, 1, NULL, data, sizeof(data))) return 0;
    *id = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    return 1;
}

uint8_t W25Q16_ReadStatus(W25Q16_Handle_t *handle, uint8_t *status)
{
    const uint8_t command = W25Q16_CMD_READ_STATUS;
    uint8_t value = 0;

    if (status == NULL) return 0;
    if (!W25Q16_Transaction(handle, &command, 1, NULL, &value, 1)) return 0;
    *status = value;
    return 1;
}

uint8_t W25Q16_WaitReady(W25Q16_Handle_t *handle, uint32_t timeout_ms)
{
    uint8_t status = 0;

    if (!W25Q16_IsValid(handle) || timeout_ms == 0) return 0;
    uint32_t start_ms = time_get_ms();
    while (1) {
        if (!W25Q16_ReadStatus(handle, &status)) return 0;
        if ((status & W25Q16_STATUS_BUSY) == 0) return 1;
        if ((uint32_t)(time_get_ms() - start_ms) >= timeout_ms) return 0;
        time_delay_ms(1);
    }
}

uint8_t W25Q16_Read(W25Q16_Handle_t *handle,
    uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t command[4];

    if (!W25Q16_IsValid(handle) || data == NULL || !W25Q16_IsRangeValid(address, size)) return 0;
    if (!W25Q16_WaitReady(handle, W25Q16_BUSY_TIMEOUT_MS)) return 0;
    W25Q16_MakeCommand(command, W25Q16_CMD_READ_DATA, address);
    return W25Q16_Transaction(handle, command, sizeof(command), NULL, data, size);
}

uint8_t W25Q16_PageProgram(W25Q16_Handle_t *handle,
    uint32_t address, const uint8_t *data, uint16_t size)
{
    uint8_t command[4];

    if (!W25Q16_IsValid(handle) || data == NULL || !W25Q16_IsRangeValid(address, size)
        || size > W25Q16_PAGE_SIZE - address % W25Q16_PAGE_SIZE) return 0;
    if (!W25Q16_WriteEnable(handle)) return 0;
    W25Q16_MakeCommand(command, W25Q16_CMD_PAGE_PROGRAM, address);
    if (!W25Q16_Transaction(handle, command, sizeof(command), data, NULL, size)) return 0;
    return W25Q16_WaitWriteDone(handle, W25Q16_PROGRAM_TIMEOUT_MS);
}

uint8_t W25Q16_Write(W25Q16_Handle_t *handle,
    uint32_t address, const uint8_t *data, uint32_t size)
{
    if (!W25Q16_IsValid(handle) || data == NULL || !W25Q16_IsRangeValid(address, size)) return 0;

    while (size > 0) {
        uint16_t chunk = (uint16_t)(W25Q16_PAGE_SIZE - address % W25Q16_PAGE_SIZE);
        if (size < chunk) chunk = (uint16_t)size;
        if (!W25Q16_PageProgram(handle, address, data, chunk)) return 0;
        address += chunk;
        data += chunk;
        size -= chunk;
    }
    return 1;
}

uint8_t W25Q16_EraseSector(W25Q16_Handle_t *handle, uint32_t address)
{
    uint8_t command[4];

    if (!W25Q16_IsValid(handle) || address % W25Q16_SECTOR_SIZE != 0
        || !W25Q16_IsRangeValid(address, W25Q16_SECTOR_SIZE)) return 0;
    if (!W25Q16_WriteEnable(handle)) return 0;
    W25Q16_MakeCommand(command, W25Q16_CMD_SECTOR_ERASE, address);
    if (!W25Q16_Transaction(handle, command, sizeof(command), NULL, NULL, 0)) return 0;
    return W25Q16_WaitWriteDone(handle, W25Q16_SECTOR_TIMEOUT_MS);
}

uint8_t W25Q16_EraseChip(W25Q16_Handle_t *handle)
{
    const uint8_t command = W25Q16_CMD_CHIP_ERASE;

    if (!W25Q16_WriteEnable(handle)) return 0;
    if (!W25Q16_Transaction(handle, &command, 1, NULL, NULL, 0)) return 0;
    return W25Q16_WaitWriteDone(handle, W25Q16_BUSY_TIMEOUT_MS);
}
