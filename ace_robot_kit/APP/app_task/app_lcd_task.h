#ifndef APP_LCD_TASK_H
#define APP_LCD_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* 使用 SPI1 驱动 240 x 240 ST7789，运行 LVGL 文字和进度条演示。
 * 调用前须完成 SPI1、GPIO、TMR13 和时间组件初始化。
 */
void lcd_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
