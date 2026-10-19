#ifndef APP_SPI_TASK_H
#define APP_SPI_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* 启动时依次读 ID、擦写测试数据、读回数据，并通过 USART1 打印，完成后只延时等待。
 * 测试扇区须预留，地址可在 app_spi_task.c 中调整。
 */
void spi_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
