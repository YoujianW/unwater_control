#include "app_spi_task.h"
#include "app_task.h"
#include "at32f403a_407_wk_config.h"
#include "w25q16.h"
#include <stdio.h>

#define SPI_INIT_RETRY_MS        1000
/* 末尾 4 KB 扇区预留给测试，每次启动仅擦写一次。 */
#define SPI_TEST_SECTOR_ADDRESS  (W25Q16_CAPACITY - W25Q16_SECTOR_SIZE)
#define SPI_TEST_DATA_ADDRESS    (SPI_TEST_SECTOR_ADDRESS + W25Q16_PAGE_SIZE - 8U)

static const uint8_t spi_test_data[] = "Hello SPI2 W25Q16";

void spi_Task_func(void *pvParameters)
{
    BSP_SPI_Handle_t spi_handle;
    W25Q16_Handle_t flash_handle;
    static uint8_t read_data[sizeof(spi_test_data)];
    uint32_t id = 0;

    (void)pvParameters;
    BSP_SPI_Init(&spi_handle, SPI2, W25Q16_CS_GPIO_PORT, W25Q16_CS_PIN);
    while (!W25Q16_Init(&flash_handle, &spi_handle))
    {
        printf("[SPI] W25Q16 init failed, retry in %u ms\r\n", (unsigned int)SPI_INIT_RETRY_MS);
        vTaskDelay(pdMS_TO_TICKS(SPI_INIT_RETRY_MS));
    }

    if (W25Q16_ReadID(&flash_handle, &id))
    {
        printf("[SPI] JEDEC ID=0x%06lX\r\n", (unsigned long)id);
    }
    else
    {
        printf("[SPI] read JEDEC ID failed\r\n");
    }

    if (W25Q16_EraseSector(&flash_handle, SPI_TEST_SECTOR_ADDRESS))
    {
        printf("[SPI] sector erase OK\r\n");
        if (W25Q16_Write(&flash_handle, SPI_TEST_DATA_ADDRESS, spi_test_data, sizeof(spi_test_data)))
        {
            printf("[SPI] write OK: \"%s\"\r\n", (const char *)spi_test_data);
        }
        else
        {
            printf("[SPI] write failed\r\n");
        }
    }
    else
    {
        printf("[SPI] sector erase failed, skip write\r\n");
    }

    if (W25Q16_Read(&flash_handle, SPI_TEST_DATA_ADDRESS, read_data, sizeof(read_data)))
    {
        printf("[SPI] read OK: \"%.*s\"\r\n",
               (int)(sizeof(read_data) - 1U), (const char *)read_data);
    }
    else
    {
        printf("[SPI] read failed\r\n");
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
