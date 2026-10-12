#include "app_task.h"
#include "led.h"

void Debug_Task_func(void *pvParameters)
{
    int32_t temperature_centi_c;
    uint32_t temperature_abs;

    printf("软件平台通用架构包V0.0.1\r\n");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        LED0_TOGGLE();
        printf("ADC VREF raw: %u\r\n", (unsigned int)adc_vref_raw);
        if (adc_vref_raw != 0)
        {
            temperature_centi_c = adc_temperature_centi_c;
            temperature_abs = (uint32_t)(temperature_centi_c < 0
                              ? -temperature_centi_c : temperature_centi_c);
            printf("ADC temperature: %s%lu.%02lu C\r\n",
                   temperature_centi_c < 0 ? "-" : "",
                   (unsigned long)(temperature_abs / 100),
                   (unsigned long)(temperature_abs % 100));
        }
    }
}
