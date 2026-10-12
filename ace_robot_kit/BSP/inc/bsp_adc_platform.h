/***************************************************************
 * bsp_adc_platform.h
 * ADC 平台适配宏 — 所有 AT32 相关操作集中于此
 ***************************************************************/
#ifndef BSP_ADC_PLATFORM_H
#define BSP_ADC_PLATFORM_H

#include "at32f403a_407_conf.h"

/* ====== 类型定义（跨平台统一） ====== */
#define BSP_ADC_TypeDef                 adc_type
#define BSP_ADC_DMA_ChannelType         dma_channel_type

/* ====== ADC 软件触发 ====== */
#define BSP_ADC_SOFTWARE_TRIGGER(hadc)  \
    adc_ordinary_software_trigger_enable((BSP_ADC_TypeDef*)(hadc), TRUE)

/* ====== ADC 数据寄存器地址（供 DMA 外设地址使用） ====== */
#define BSP_ADC_GET_ODT_ADDR(hadc)      \
    ((uint32_t)&((BSP_ADC_TypeDef*)(hadc))->odt)

/* ====== ADC 单次读取（非 DMA 场景） ====== */
#define BSP_ADC_READ_ODT(hadc)          \
    adc_ordinary_conversion_data_get((BSP_ADC_TypeDef*)(hadc))

/* ====== DMA 通道运行时配置 ====== */
#define BSP_ADC_DMA_SET_PERIPH_ADDR(ch, addr) \
    (((BSP_ADC_DMA_ChannelType*)(ch))->paddr = (uint32_t)(addr))

#define BSP_ADC_DMA_SET_MEM_ADDR(ch, addr) \
    (((BSP_ADC_DMA_ChannelType*)(ch))->maddr = (uint32_t)(addr))

#define BSP_ADC_DMA_SET_BUFFER_SIZE(ch, size) \
    (((BSP_ADC_DMA_ChannelType*)(ch))->dtcnt = (uint16_t)(size))

#define BSP_ADC_DMA_ENABLE(ch, en) \
    dma_channel_enable((BSP_ADC_DMA_ChannelType*)(ch), (confirm_state)(en))

#define BSP_ADC_DMA_DISABLE(ch) \
    dma_channel_enable((BSP_ADC_DMA_ChannelType*)(ch), FALSE)

#endif
