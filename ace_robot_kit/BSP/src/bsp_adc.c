/***************************************************************
 * bsp_adc.c
 * ADC BSP 层实现
 ***************************************************************/
#include "bsp_adc.h"
#include "bsp_adc_platform.h"

void BSP_ADC_Init(BSP_ADC_Handle_t *handle,
                  void *adc, void *dma_ch,
                  uint16_t *buf, uint16_t buf_size)
{
    if (handle == BSP_ADC_NULL) return;

    handle->adc_handle  = adc;
    handle->dma_channel = dma_ch;
    handle->buffer      = buf;
    handle->buffer_size = buf_size;
}

void BSP_ADC_Start(BSP_ADC_Handle_t *handle)
{
    if (handle == BSP_ADC_NULL || handle->adc_handle == BSP_ADC_NULL || handle->dma_channel == BSP_ADC_NULL ||
        handle->buffer == BSP_ADC_NULL || handle->buffer_size == 0) {
        return;
    }

    /* 修改 DMA 地址和数量前必须关闭通道。 */
    BSP_ADC_DMA_DISABLE(handle->dma_channel);

    /*
     * 配置 DMA 传输参数：
     *   外设地址 = ADC1->odt（ADC 数据寄存器）
     *   内存地址 = 用户提供的缓冲区
     *   传输数量 = ADC 通道数（每次扫描完成后 DMA 自动循环）
     * 
     * 注意：DMA 已在 wk_dma1_channel1_init() 中初始化了 loop_mode，
     * 这里只需设置运行时变化的三个参数即可。
     */
    BSP_ADC_DMA_SET_PERIPH_ADDR(handle->dma_channel,
                                BSP_ADC_GET_ODT_ADDR(handle->adc_handle));
    BSP_ADC_DMA_SET_MEM_ADDR(handle->dma_channel,
                             (uint32_t)handle->buffer);
    BSP_ADC_DMA_SET_BUFFER_SIZE(handle->dma_channel,
                                handle->buffer_size);

    /* 使能 DMA 通道，准备接收 ADC 数据 */
    BSP_ADC_DMA_ENABLE(handle->dma_channel, TRUE);

    /* 触发 ADC 转换；单次/连续转换由 wk_adc1_init 的 repeat_mode 决定。 */
    BSP_ADC_SOFTWARE_TRIGGER(handle->adc_handle);
}

void BSP_ADC_Stop(BSP_ADC_Handle_t *handle)
{
    if (handle == BSP_ADC_NULL || handle->dma_channel == BSP_ADC_NULL) {
        return;
    }
    BSP_ADC_DMA_DISABLE(handle->dma_channel);
}

// 读取指定通道的 ADC 值（从 DMA 缓冲区）
uint16_t BSP_ADC_ReadChannel(const BSP_ADC_Handle_t *handle,
                             uint8_t channel_index)
{
    if (handle == BSP_ADC_NULL || handle->buffer == BSP_ADC_NULL) {
        return 0;
    }
    if (channel_index >= handle->buffer_size) {
        return 0;
    }
    return ((volatile uint16_t *)handle->buffer)[channel_index];
}

// 批量读取 ADC 缓冲区
void BSP_ADC_ReadBuffer(const BSP_ADC_Handle_t *handle,
                        uint16_t *dst, uint16_t len)
{
    if (handle == BSP_ADC_NULL || handle->buffer == BSP_ADC_NULL || dst == BSP_ADC_NULL) {
        return;
    }

    uint16_t copy_len = (len < handle->buffer_size) ? len : handle->buffer_size;
    for (uint16_t i = 0; i < copy_len; i++) {
        dst[i] = ((volatile uint16_t *)handle->buffer)[i];
    }
}
