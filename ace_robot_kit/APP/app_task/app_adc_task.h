#ifndef APP_ADC_TASK_H
#define APP_ADC_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern volatile uint16_t adc_vref_raw;
extern volatile uint16_t adc_temperature_raw;
/* 片内温度，单位为 0.01 摄氏度，例如 2534 表示 25.34 摄氏度。 */
extern volatile int32_t adc_temperature_centi_c;

void adc_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
