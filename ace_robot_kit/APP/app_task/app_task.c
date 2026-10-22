#include "app_task.h"
#include "app_config.h"

// TaskHandle_t LED_Task_handle;
TaskHandle_t vofa_Task_handle;
TaskHandle_t sbus_Task_handle;
TaskHandle_t adc_Task_handle;
TaskHandle_t i2c_Task_handle;
TaskHandle_t spi_Task_handle;
TaskHandle_t oled_Task_handle;
TaskHandle_t button_Task_handle;
// TaskHandle_t Sonar_Task_handle;

// 如果堆栈空间足够，建议给到512，否则有时串口打印浮点数会卡死
TaskHandle_t Debug_Task_handle;

void app_task_create(void)
{
    // xTaskCreate(LED_Task_func,"LED_Task",128,NULL,0,&LED_Task_handle);
    xTaskCreate(vofa_Task_func,"vofa_Task",256,NULL,0,&vofa_Task_handle);
    xTaskCreate(sbus_Task_func,"sbus_Task",256,NULL,0,&sbus_Task_handle);
    xTaskCreate(adc_Task_func,"adc_Task",128,NULL,0,&adc_Task_handle);
    xTaskCreate(i2c_Task_func,"i2c_Task",256,NULL,0,&i2c_Task_handle);
    xTaskCreate(spi_Task_func,"spi_Task",512,NULL,0,&spi_Task_handle);
    xTaskCreate(oled_Task_func,"oled_Task",256,NULL,0,&oled_Task_handle);
    xTaskCreate(button_Task_func,"button_Task",128,NULL,0,&button_Task_handle);
    // xTaskCreate(Sonar_Task_func,"Sonar_Task",128,NULL,0,&Sonar_Task_handle);
    xTaskCreate(Debug_Task_func,"Test_Task",128,NULL,0,&Debug_Task_handle);
}

void app_freertos_init(void)
{
  taskENTER_CRITICAL();

  app_task_create();

  taskEXIT_CRITICAL();
  vTaskStartScheduler();
}
