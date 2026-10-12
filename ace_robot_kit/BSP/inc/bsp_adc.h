/***************************************************************
 * bsp_adc.h
 * ADC 通用接口（平台无关）
 ***************************************************************/
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#define BSP_ADC_NULL ((void *)0)

/* ADC 句柄 */
typedef struct {
    void     *adc_handle;      /* ADC 外设基地址（如 ADC1）    */
    void     *dma_channel;     /* DMA 通道（如 DMA1_CHANNEL1） */
    uint16_t *buffer;          /* DMA 缓冲区基地址             */
    uint16_t  buffer_size;     /* 缓冲区长度（uint16_t 个数） */
} BSP_ADC_Handle_t;

/**
 * @brief 初始化 ADC 句柄
 * @param handle    句柄指针
 * @param adc       ADC 外设基地址
 * @param dma_ch    DMA 通道指针
 * @param buf       接收缓冲区指针
 * @param buf_size  缓冲区大小（= ADC 通道数）
 */
void BSP_ADC_Init(BSP_ADC_Handle_t *handle,
                  void *adc, void *dma_ch,
                  uint16_t *buf, uint16_t buf_size);

/**
 * @brief 启动 ADC DMA 转换（单次、连续由 wk_adc1_init 配置）
 *        配置 DMA 传输参数并使能，触发 ADC 转换
 */
void BSP_ADC_Start(BSP_ADC_Handle_t *handle);

/**
 * @brief 停止 ADC DMA 转换
 */
void BSP_ADC_Stop(BSP_ADC_Handle_t *handle);

/**
 * @brief 读取指定通道索引的最新 ADC 值（从 DMA 缓冲区）
 * @param channel_index  通道索引（0 ~ buffer_size-1）
 * @return ADC 原始值（12bit，右对齐）
 */
uint16_t BSP_ADC_ReadChannel(const BSP_ADC_Handle_t *handle,
                             uint8_t channel_index);

/**
 * @brief 批量读取全部 ADC 缓冲区
 * @param dst  目标缓冲区
 * @param len  要拷贝的长度
 */
void BSP_ADC_ReadBuffer(const BSP_ADC_Handle_t *handle,
                        uint16_t *dst, uint16_t len);

#endif
