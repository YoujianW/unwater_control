#ifndef APP_OLED_TASK_H
#define APP_OLED_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* 使用 I2C2 驱动 128 x 64 SSD1306（7 位地址 0x3C），每 200 ms 刷新运行时间。
 * 调用前须完成 I2C2、TMR13 和时间组件初始化，不修改底层外设配置。
 */
void oled_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
