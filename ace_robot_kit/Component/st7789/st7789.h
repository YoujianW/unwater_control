/***************************************************************
 * st7789.h
 * ST7789 LCD 显示组件，使用 BSP SPI 阻塞发送接口
 ***************************************************************/
#ifndef ST7789_H
#define ST7789_H

#include "bsp_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ST7789_WIDTH             240U
#define ST7789_HEIGHT            240U
#define ST7789_FONT_WIDTH        6U
#define ST7789_FONT_HEIGHT       8U

/* 方向 0 时可见区域在 240 x 320 显存中的起点，按当前 1.54 英寸模组配置。 */
#ifndef ST7789_X_OFFSET
#define ST7789_X_OFFSET          0U
#endif

#ifndef ST7789_Y_OFFSET
#define ST7789_Y_OFFSET          80U
#endif

#ifndef ST7789_COLOR_BGR
#define ST7789_COLOR_BGR         0U
#endif

#ifndef ST7789_DEFAULT_INVERT
#define ST7789_DEFAULT_INVERT    1U
#endif

#define ST7789_COLOR_BLACK       0x0000U
#define ST7789_COLOR_WHITE       0xFFFFU
#define ST7789_COLOR_RED         0xF800U
#define ST7789_COLOR_GREEN       0x07E0U
#define ST7789_COLOR_BLUE        0x001FU
#define ST7789_COLOR_YELLOW      0xFFE0U
#define ST7789_COLOR_CYAN        0x07FFU
#define ST7789_COLOR_MAGENTA     0xF81FU

typedef struct {
    BSP_SPI_Handle_t *spi;      /* 已初始化的 SPI 句柄，使用期间须保持有效 */
    uint16_t width;             /* 当前方向下的逻辑宽高 */
    uint16_t height;
    uint16_t x_start;           /* 当前方向下的显存起点 */
    uint16_t y_start;
    uint8_t rotation;           /* 0 ~ 3，每档相差 90 度 */
} ST7789_Handle_t;

/* 所有状态接口均返回 1 表示完成，0 表示 SPI 失败或绘制区域为空。
 * 调用方保证句柄、SPI 已初始化，数据指针有效，参数符合接口约定。
 * LCD 无应答，返回成功不代表屏幕在线；失败可能已更新部分内容。
 * 无整屏缓存，绘制立即发送；坐标以左上角为原点，x 向右、y 向下。
 * 须在中断开启的任务或主循环调用，多任务共享时由调用方锁住整个组件操作。
 */

/**
 * @brief 绑定 SPI，复位、退出休眠、设置 RGB565、清黑屏并开启显示和背光
 * @note 调用前须完成 wk_gpio_config、wk_spi1_init、wk_tmr13_init 和 time_init。
 *       使用 WorkBench 的 ST7789_DC/RES/BLK 引脚，背光高电平有效。
 *       支持 8 位、MSB 优先、模式 0 的全双工或半双工发送，不修改外设配置。
 *       初始化失败时清空 SPI 绑定，发送失败时释放 CS 并关闭背光。
 * @example
 *       static BSP_SPI_Handle_t spi;
 *       static ST7789_Handle_t lcd;
 *       BSP_SPI_Init(&spi, SPI1, ST7789_CS_GPIO_PORT, ST7789_CS_PIN);
 *       if (ST7789_Init(&lcd, &spi)) {
 *           ST7789_ShowString(&lcd, 8, 8, "Hello ST7789",
 *               ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
 *       }
 *       平台符号由 APP 自行包含 at32f403a_407_wk_config.h 获得。
 */
uint8_t ST7789_Init(ST7789_Handle_t *handle, BSP_SPI_Handle_t *spi);

/* rotation 为 0 ~ 3，不重绘原有内容；红蓝颠倒时统一修改 ST7789_COLOR_BGR。 */
uint8_t ST7789_SetRotation(ST7789_Handle_t *handle, uint8_t rotation);
uint8_t ST7789_SetInvert(ST7789_Handle_t *handle, uint8_t enable);
uint8_t ST7789_DisplayOn(ST7789_Handle_t *handle);
uint8_t ST7789_DisplayOff(ST7789_Handle_t *handle);
uint8_t ST7789_SetBacklight(ST7789_Handle_t *handle, uint8_t enable);

uint16_t ST7789_RGB565(uint8_t red, uint8_t green, uint8_t blue);
uint8_t ST7789_Clear(ST7789_Handle_t *handle);
uint8_t ST7789_Fill(ST7789_Handle_t *handle, uint16_t color);
uint8_t ST7789_DrawPoint(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t color);

/* width/height 为尺寸，不是终点坐标。右侧、底部超出部分自动裁剪，
 * 起点越界或宽高为 0 返回 0，不发送数据。
 */
uint8_t ST7789_FillRect(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/* RGB565 uint16_t 数组，逐行从左到右排列，至少 width * height 个像素。
 * 自动转换为高字节先发送，不要预先交换字节；裁剪后保留原图行跨度。
 */
uint8_t ST7789_DrawImage(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);

/* 内置 6 x 8 ASCII 字体（空格 ~ 波浪号），指定前景色和背景色。
 * 不支持的字符显示 '?'，字符超出部分裁剪。字符串支持 \r、\n 和自动换行，
 * 换行回到起始 x，仅显示完整字符，超出底部停止，须以 '\0' 结尾。
 */
uint8_t ST7789_ShowChar(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t background);
uint8_t ST7789_ShowString(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t background);

/* 无符号十进制数，length 为 1 ~ 10，前导补零，超长时保留低位。 */
uint8_t ST7789_ShowNum(ST7789_Handle_t *handle, uint16_t x, uint16_t y,
    uint32_t number, uint8_t length, uint16_t color, uint16_t background);

#ifdef __cplusplus
}
#endif

#endif
