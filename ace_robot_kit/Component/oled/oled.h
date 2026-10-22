/***************************************************************
 * oled.h
 * SSD1306 OLED 显示组件，使用 BSP I2C 寄存器接口
 ***************************************************************/
#ifndef OLED_H
#define OLED_H

#include "bsp_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_ADDRESS_LOW        0x3CU
#define OLED_ADDRESS_HIGH       0x3DU
#define OLED_WIDTH              128U

#ifndef OLED_HEIGHT
#define OLED_HEIGHT             64U
#endif

#if OLED_HEIGHT != 32U && OLED_HEIGHT != 64U
#error "OLED_HEIGHT must be 32 or 64"
#endif

/* 可见区域相对显存的起始列：标准 SSD1306 为 0，常见 SH1106 模块为 2。
 * 当前屏幕先按 2 列补偿配置；若确认使用标准 SSD1306，请改为 0。
 */
#ifndef OLED_COLUMN_OFFSET
#define OLED_COLUMN_OFFSET      2U
#endif

#if OLED_COLUMN_OFFSET > 4U
#error "OLED_COLUMN_OFFSET must be between 0 and 4"
#endif

#define OLED_PAGE_COUNT         (OLED_HEIGHT / 8U)
#define OLED_FONT_WIDTH         6U
#define OLED_FONT_HEIGHT        8U
#define OLED_COLOR_BLACK        0U
#define OLED_COLOR_WHITE        1U

typedef struct {
    BSP_I2C_Handle_t *i2c;      /* 已初始化的 I2C 句柄，使用期间须保持有效 */
    uint8_t address;            /* 未左移的 7 位地址，通常为 0x3C */
    uint8_t buffer[OLED_PAGE_COUNT][OLED_WIDTH];
} OLED_Handle_t;

/* 通信接口为阻塞接口，BSP 无通信状态返回值，不能判断屏幕是否应答。
 * 坐标以左上角为原点，x 向右、y 向下，单位均为像素。
 * 清屏和绘制只修改句柄内的显存，调用 OLED_Update 后显示生效。
 * 须在中断开启的任务或主循环中调用通信接口；多任务共享总线或句柄时，
 * 由调用方锁住整个组件操作。句柄含显存，建议定义为静态或全局变量。
 */

/**
 * @brief 绑定 I2C，初始化 SSD1306 内部电荷泵、清屏并开启显示
 * @note 调用前须完成 wk_i2c2_init、wk_tmr13_init 和 time_init。
 *       默认 128 x 64，128 x 32 屏须统一配置 OLED_HEIGHT=32。
 *       等待上电稳定 100 ms，不修改 I2C 和 GPIO 配置。
 * @example
 *       static BSP_I2C_Handle_t i2c;
 *       static OLED_Handle_t oled;
 *       BSP_I2C_Init(&i2c, I2C2);
 *       OLED_Init(&oled, &i2c, OLED_ADDRESS_LOW);
 *       OLED_ShowString(&oled, 0, 0, "Hello OLED");
 *       OLED_Update(&oled);
 *       I2C2 符号由 APP 自行包含 at32f403a_407_wk_config.h 获得。
 */
void OLED_Init(OLED_Handle_t *handle, BSP_I2C_Handle_t *i2c, uint8_t address);

/* 按 OLED_COLUMN_OFFSET 定位可见区域，逐页发送显存，每页 128 字节，
 * 避免整屏占满 BSP 单次超时时间；偏移宏须在所有源文件中保持一致。
 */
void OLED_Update(OLED_Handle_t *handle);
void OLED_Clear(OLED_Handle_t *handle);
void OLED_Fill(OLED_Handle_t *handle, uint8_t color);

/* 立即发送显示开关、对比度（0 ~ 255）和反显控制，不修改显存。 */
void OLED_DisplayOn(OLED_Handle_t *handle);
void OLED_DisplayOff(OLED_Handle_t *handle);
void OLED_SetContrast(OLED_Handle_t *handle, uint8_t contrast);
void OLED_SetInvert(OLED_Handle_t *handle, uint8_t enable);

/* 绘制单个像素，color 为 0 时熄灭，非 0 时点亮，超出屏幕范围时忽略。 */
void OLED_DrawPoint(OLED_Handle_t *handle, uint8_t x, uint8_t y, uint8_t color);

/* 内置 6 x 8 ASCII 字体（空格 ~ 波浪号），白字黑底，其他字符显示 '?'。
 * 字符超出右侧或底部的部分裁剪；字符串仅显示完整字符，支持 \r、\n
 * 和自动换行，换行回到起始 x，超出底部停止显示。字符串须以 '\0' 结尾。
 */
void OLED_ShowChar(OLED_Handle_t *handle, uint8_t x, uint8_t y, char ch);
void OLED_ShowString(OLED_Handle_t *handle, uint8_t x, uint8_t y, const char *text);

/* 显示无符号十进制数，length 为 1 ~ 10，前导补零，超长时保留低位。 */
void OLED_ShowNum(OLED_Handle_t *handle, uint8_t x, uint8_t y,
    uint32_t number, uint8_t length);

/**
 * @brief 绘制单色位图，覆盖目标区域，超出屏幕部分裁剪
 * @param data  按页排列、每页从左到右，每字节表示竖向 8 点，bit0 在上
 * @param width 位图宽度，单位像素，必须大于 0
 * @param height 位图高度，单位像素，必须大于 0，不要求为 8 的倍数
 * @note data 至少包含 width * ((height + 7) / 8) 字节。
 */
void OLED_DrawBitmap(OLED_Handle_t *handle, uint8_t x, uint8_t y,
    uint8_t width, uint8_t height, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
