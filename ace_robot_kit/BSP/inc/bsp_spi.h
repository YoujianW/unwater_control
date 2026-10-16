/***************************************************************
 * bsp_spi.h
 * SPI 主机阻塞收发通用接口（平台无关）
 ***************************************************************/
#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SPI_TIMEOUT_MS
#define BSP_SPI_TIMEOUT_MS   20U
#endif

#ifndef BSP_SPI_DUMMY_BYTE
#define BSP_SPI_DUMMY_BYTE   0xFFU
#endif

typedef struct {
    void *spi_handle;        /* SPI 外设基地址，由 APP 传入 */
    void *cs_port;           /* 低电平有效的片选 GPIO 端口 */
    uint16_t cs_pin;         /* 单个 GPIO 引脚位掩码，不是引脚序号 */
} BSP_SPI_Handle_t;

/**
 * @brief 绑定 SPI 和片选引脚，并释放片选，不修改硬件配置
 * @param handle   句柄指针
 * @param spi      SPI 外设基地址（如 SPI2）
 * @param cs_port  片选 GPIO 端口（如 GPIOB）
 * @param cs_pin   片选引脚位掩码（如 GPIO_PINS_12）
 * @note 调用前由 wk_spi / wk_gpio 完成 8 位全双工主机和片选输出配置。
 *       收发前须初始化时间组件及其硬件定时器，仅在中断开启的任务或主循环调用。
 *       不支持与同一 SPI 的 DMA、中断收发混用；多任务共享总线时，
 *       调用方须锁住从 CS_Low 到 CS_High 的整个事务。
 * @example
 *       BSP_SPI_Handle_t spi;
 *       BSP_SPI_Init(&spi, SPI2, W25Q16_CS_GPIO_PORT, W25Q16_CS_PIN);
 *       BSP_SPI_CS_Low(&spi);
 *       uint8_t ok = BSP_SPI_Transfer(&spi, tx_data, rx_data, size);
 *       BSP_SPI_CS_High(&spi);  // 无论收发是否成功，事务结束均需释放片选
 *       平台符号由 APP 自行包含 at32f403a_407_wk_config.h 获得。
 */
void BSP_SPI_Init(BSP_SPI_Handle_t *handle,
    void *spi, void *cs_port, uint16_t cs_pin);

/**
 * @brief 拉低片选，开始一次从机事务
 * @note 收发接口不会自动操作片选，可连续发送命令、地址和数据。
 *       从机要求的片选建立、保持及间隔时间由调用方保证。
 */
void BSP_SPI_CS_Low(BSP_SPI_Handle_t *handle);

/**
 * @brief 拉高片选，结束或中止一次从机事务
 * @note 收发成功时已等待最后一位发送结束；失败时也须调用本接口释放片选。
 */
void BSP_SPI_CS_High(BSP_SPI_Handle_t *handle);

/**
 * @brief 同时发送和接收数据块（阻塞），不操作片选
 * @param handle   已初始化的 SPI 句柄指针
 * @param tx_data  待发送的数据，NULL 时发送 BSP_SPI_DUMMY_BYTE 产生时钟
 * @param rx_data  接收缓冲区，NULL 时丢弃接收数据，避免接收溢出
 * @param size     收发字节数，必须大于 0；两个缓冲区不能同时为 NULL
 * @return 1：全部收发完成；0：参数或硬件模式无效、超时、硬件错误
 * @note 支持 tx_data == rx_data 原地交换。BSP_SPI_TIMEOUT_MS 限制整个数据块，
 *       超时计算支持毫秒计数回绕；失败时缓冲区可能已包含部分数据。
 *       成功前会等待发送缓冲区为空、SPI 总线空闲。
 */
uint8_t BSP_SPI_Transfer(BSP_SPI_Handle_t *handle,
    const uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/**
 * @brief 发送数据块（阻塞），同步读出并丢弃接收数据，不操作片选
 * @return 1：发送完成；0：参数无效、超时或硬件错误
 */
uint8_t BSP_SPI_SendBuffer(BSP_SPI_Handle_t *handle,
    const uint8_t *data, uint16_t size);

/**
 * @brief 接收数据块（阻塞），发送 BSP_SPI_DUMMY_BYTE 产生时钟，不操作片选
 * @return 1：接收完成；0：参数无效、超时或硬件错误
 */
uint8_t BSP_SPI_ReceiveBuffer(BSP_SPI_Handle_t *handle,
    uint8_t *data, uint16_t size);

/**
 * @brief 交换一个字节（阻塞），不操作片选
 * @return 接收到的字节；失败时返回 0，0 也可能是有效数据
 * @note 需要区分通信失败时，使用 size = 1 的 BSP_SPI_Transfer。
 */
uint8_t BSP_SPI_ReadWriteByte(BSP_SPI_Handle_t *handle, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif
