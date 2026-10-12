#include "app_adc_task.h"
#include "app_task.h"
#include "bsp_adc.h"
#include "wk_dma.h"

#define ADC1_DMA_BUFFER_SIZE   2

/* AT32F403A 数据手册 5.3.21/5.3.22：典型参数，绝对温度需校准。 */
#define ADC_VINTRV_MV          1200
#define ADC_TEMP_V25_MV        1280
#define ADC_TEMP_AVG_SLOPE_UV  (-4260)

volatile uint16_t adc_vref_raw;
volatile uint16_t adc_temperature_raw;
volatile int32_t adc_temperature_centi_c;

void adc_Task_func(void *pvParameters)
{
    static uint16_t adc1_dma_buffer[ADC1_DMA_BUFFER_SIZE];
    BSP_ADC_Handle_t adc_handle;
    uint16_t vref_raw;
    uint16_t temperature_raw;

    wk_dma1_channel1_init();
    wk_dma_channel_config(DMA1_CHANNEL1,
                          (uint32_t)&ADC1->odt,
                          (uint32_t)adc1_dma_buffer,
                          ADC1_DMA_BUFFER_SIZE);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);

    BSP_ADC_Init(&adc_handle, ADC1, DMA1_CHANNEL1,
                 adc1_dma_buffer, ADC1_DMA_BUFFER_SIZE);

    while (1)
    {
        BSP_ADC_Start(&adc_handle);
        vTaskDelay(pdMS_TO_TICKS(1));
        /* 扫描顺序：ADC_CHANNEL_17 为内部参考，ADC_CHANNEL_16 为温度。 */
        vref_raw = BSP_ADC_ReadChannel(&adc_handle, 0);
        temperature_raw = BSP_ADC_ReadChannel(&adc_handle, 1);
        adc_vref_raw = vref_raw;
        adc_temperature_raw = temperature_raw;

        if (vref_raw != 0)
        {
            /* VTS = VINTRV * temperature_raw / vref_raw。
             * T = (V25 - VTS) / Avg_Slope + 25，结果保存为 0.01 摄氏度。
             * 100000 = mV 转 uV 的 1000 倍 * 摄氏度转百分之一度的 100 倍。 */
            adc_temperature_centi_c = 2500 + (int32_t)(
                ((int64_t)ADC_TEMP_V25_MV * vref_raw
                 - (int64_t)ADC_VINTRV_MV * temperature_raw) * 100000LL
                / ((int64_t)vref_raw * ADC_TEMP_AVG_SLOPE_UV));
        }
    }
}
