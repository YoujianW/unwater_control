/***************************************************************
 * w25q16.h
 * W25Q16 SPI Flash 组件，使用 BSP SPI 阻塞收发接口
 ***************************************************************/
#ifndef W25Q16_H
#define W25Q16_H

#include "bsp_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q16_JEDEC_ID          0xEF4015UL
#define W25Q16_CAPACITY          0x200000UL  /* 2 MB，地址范围 0x000000 ~ 0x1FFFFF */
#define W25Q16_PAGE_SIZE         256U
#define W25Q16_SECTOR_SIZE       4096U
#define W25Q16_SECTOR_COUNT      (W25Q16_CAPACITY / W25Q16_SECTOR_SIZE)
#define W25Q16_STATUS_BUSY       0x01U
#define W25Q16_STATUS_WEL        0x02U

#ifndef W25Q16_BUSY_TIMEOUT_MS
#define W25Q16_BUSY_TIMEOUT_MS   30000U
#endif

#ifndef W25Q16_PROGRAM_TIMEOUT_MS
#define W25Q16_PROGRAM_TIMEOUT_MS 10U
#endif

#ifndef W25Q16_SECTOR_TIMEOUT_MS
#define W25Q16_SECTOR_TIMEOUT_MS 1000U
#endif

typedef struct {
    BSP_SPI_Handle_t *spi;      /* 已初始化的 SPI 句柄，须在组件使用期间保持有效 */
} W25Q16_Handle_t;

/* 所有接口均为阻塞接口，返回 1 表示完成，0 表示参数无效、通信失败、
 * 身份不匹配、写使能未生效或等待超时；读取失败可能已有部分数据。
 * 写入、擦除不修改芯片写保护设置，也不自动读回校验内容。
 * 须在中断开启的任务或主循环中调用；多任务共享时，由调用方锁住整个组件操作。
 */

/**
 * @brief 绑定 SPI、退出掉电模式、等待空闲并检查 JEDEC ID
 * @param handle  组件句柄指针，初始化失败时清空其 SPI 绑定
 * @param spi     已绑定外设和片选的 BSP SPI 句柄
 * @note 调用前须完成 wk_spi2_init、wk_gpio_config、wk_tmr13_init 和 time_init。
 *       硬件使用 8 位、MSB 优先、SPI 模式 0 或 3，普通读取时 SCK 不超过 50 MHz。
 *       不修改外设配置。
 *       初始化等待 10 ms，兼顾芯片上电写入禁止时间；不复位或擦除 Flash。
 * @example
 *       BSP_SPI_Handle_t spi;
 *       W25Q16_Handle_t flash;
 *       BSP_SPI_Init(&spi, SPI2, W25Q16_CS_GPIO_PORT, W25Q16_CS_PIN);
 *       uint8_t ok = W25Q16_Init(&flash, &spi);
 *       平台符号由 APP 自行包含 at32f403a_407_wk_config.h 获得。
 */
uint8_t W25Q16_Init(W25Q16_Handle_t *handle, BSP_SPI_Handle_t *spi);

/**
 * @brief 读取 24 位 JEDEC ID（厂商、存储器类型、容量），正常为 0xEF4015
 * @param id  存放 ID 的变量指针，通信失败时保留原值
 */
uint8_t W25Q16_ReadID(W25Q16_Handle_t *handle, uint32_t *id);

/**
 * @brief 读取状态寄存器 1，BUSY 为 bit0，WEL 为 bit1
 * @param status  存放状态的变量指针，通信失败时保留原值
 */
uint8_t W25Q16_ReadStatus(W25Q16_Handle_t *handle, uint8_t *status);

/**
 * @brief 等待 BUSY 清零，期间每次查询后释放片选并延时 1 ms
 * @param timeout_ms  忙等待超时时间，必须大于 0
 * @note 采用毫秒差值处理计数回绕；不支持擦写挂起状态下的访问。
 */
uint8_t W25Q16_WaitReady(W25Q16_Handle_t *handle, uint32_t timeout_ms);

/**
 * @brief 从指定字节地址连续读取数据，允许跨页、跨扇区
 * @param address  起始字节地址
 * @param data     接收缓冲区，容量至少为 size 字节
 * @param size     读取字节数，必须大于 0，范围不能超出芯片容量
 */
uint8_t W25Q16_Read(W25Q16_Handle_t *handle,
    uint32_t address, uint8_t *data, uint32_t size);

/**
 * @brief 在单页内编程，并等待完成，不自动擦除
 * @param address  起始字节地址，不要求页对齐
 * @param data     待写入的数据
 * @param size     写入字节数，1 ~ 256，不能跨越当前页的边界
 * @note Flash 编程只能将位从 1 改为 0；需要从 0 改为 1 时须先擦除所在扇区。
 *       失败时可能已写入部分数据，不会回滚。
 */
uint8_t W25Q16_PageProgram(W25Q16_Handle_t *handle,
    uint32_t address, const uint8_t *data, uint16_t size);

/**
 * @brief 连续写入，自动按页边界拆分并逐页等待完成，不自动擦除
 * @param address  起始字节地址
 * @param data     待写入的数据
 * @param size     写入字节数，必须大于 0，范围不能超出芯片容量
 * @note 写入限制及失败后的部分写入行为与 PageProgram 相同。
 */
uint8_t W25Q16_Write(W25Q16_Handle_t *handle,
    uint32_t address, const uint8_t *data, uint32_t size);

/**
 * @brief 擦除一个 4 KB 扇区为 0xFF，并等待完成
 * @param address  扇区起始字节地址，必须是 W25Q16_SECTOR_SIZE 的整数倍
 * @note 参数为字节地址，不是扇区编号；例如第二个扇区传入 4096。
 */
uint8_t W25Q16_EraseSector(W25Q16_Handle_t *handle, uint32_t address);

/**
 * @brief 擦除整片 Flash 为 0xFF，并等待完成，默认最长等待 30 s
 */
uint8_t W25Q16_EraseChip(W25Q16_Handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif
