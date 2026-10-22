/***************************************************************
 * oled.c
 * SSD1306 初始化及显存绘制，所有 I2C 操作通过 bsp_i2c.h
 ***************************************************************/
#include "oled.h"
#include "time_core.h"
#include <stddef.h>
#include <string.h>

/* BSP 的 reg 参数用于发送 OLED 控制字节，不是普通寄存器地址。
 * SSD1306 手册：https://newhavendisplay.com/content/app_notes/SSD1306.pdf
 */
#define OLED_CONTROL_COMMAND    0x00U
#define OLED_CONTROL_DATA       0x40U
#define OLED_CMD_DISPLAY_OFF    0xAEU
#define OLED_CMD_DISPLAY_ON     0xAFU
#define OLED_CMD_CONTRAST       0x81U
#define OLED_CMD_NORMAL         0xA6U
#define OLED_CMD_INVERT         0xA7U

#if OLED_HEIGHT == 64U
#define OLED_COM_CONFIG         0x12U
#else
#define OLED_COM_CONFIG         0x02U
#endif

/* 5 x 7 字形，额外一列和一行留空组成 6 x 8 字符，按 ASCII 0x20 ~ 0x7E 排列。 */
static const uint8_t OLED_Font[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* 空格 */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ; */
    {0x08, 0x14, 0x22, 0x41, 0x00}, /* < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* = */
    {0x00, 0x41, 0x22, 0x14, 0x08}, /* > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* F */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* Z */
    {0x00, 0x7F, 0x41, 0x41, 0x00}, /* [ */
    {0x02, 0x04, 0x08, 0x10, 0x20}, /* 反斜杠 */
    {0x00, 0x41, 0x41, 0x7F, 0x00}, /* ] */
    {0x04, 0x02, 0x01, 0x02, 0x04}, /* ^ */
    {0x40, 0x40, 0x40, 0x40, 0x40}, /* _ */
    {0x00, 0x01, 0x02, 0x04, 0x00}, /* ` */
    {0x20, 0x54, 0x54, 0x54, 0x78}, /* a */
    {0x7F, 0x48, 0x44, 0x44, 0x38}, /* b */
    {0x38, 0x44, 0x44, 0x44, 0x20}, /* c */
    {0x38, 0x44, 0x44, 0x48, 0x7F}, /* d */
    {0x38, 0x54, 0x54, 0x54, 0x18}, /* e */
    {0x08, 0x7E, 0x09, 0x01, 0x02}, /* f */
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, /* g */
    {0x7F, 0x08, 0x04, 0x04, 0x78}, /* h */
    {0x00, 0x44, 0x7D, 0x40, 0x00}, /* i */
    {0x20, 0x40, 0x44, 0x3D, 0x00}, /* j */
    {0x7F, 0x10, 0x28, 0x44, 0x00}, /* k */
    {0x00, 0x41, 0x7F, 0x40, 0x00}, /* l */
    {0x7C, 0x04, 0x18, 0x04, 0x78}, /* m */
    {0x7C, 0x08, 0x04, 0x04, 0x78}, /* n */
    {0x38, 0x44, 0x44, 0x44, 0x38}, /* o */
    {0x7C, 0x14, 0x14, 0x14, 0x08}, /* p */
    {0x08, 0x14, 0x14, 0x18, 0x7C}, /* q */
    {0x7C, 0x08, 0x04, 0x04, 0x08}, /* r */
    {0x48, 0x54, 0x54, 0x54, 0x20}, /* s */
    {0x04, 0x3F, 0x44, 0x40, 0x20}, /* t */
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, /* u */
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, /* v */
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, /* w */
    {0x44, 0x28, 0x10, 0x28, 0x44}, /* x */
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, /* y */
    {0x44, 0x64, 0x54, 0x4C, 0x44}, /* z */
    {0x00, 0x08, 0x36, 0x41, 0x00}, /* { */
    {0x00, 0x00, 0x7F, 0x00, 0x00}, /* | */
    {0x00, 0x41, 0x36, 0x08, 0x00}, /* } */
    {0x08, 0x04, 0x08, 0x10, 0x08}  /* ~ */
};

static uint8_t OLED_IsValid(const OLED_Handle_t *handle)
{
    return handle != NULL && handle->i2c != NULL
        && handle->i2c->i2c_handle != NULL
        && (handle->address == OLED_ADDRESS_LOW || handle->address == OLED_ADDRESS_HIGH);
}

static void OLED_WriteCommand(OLED_Handle_t *handle, uint8_t command)
{
    if (!OLED_IsValid(handle)) return;
    BSP_I2C_WriteReg(handle->i2c, handle->address, OLED_CONTROL_COMMAND, command);
}

void OLED_Init(OLED_Handle_t *handle, BSP_I2C_Handle_t *i2c, uint8_t address)
{
    static const uint8_t config[] = {
        OLED_CMD_DISPLAY_OFF,
        0xD5U, 0x80U,               /* 显示时钟分频及振荡频率 */
        0xA8U, OLED_HEIGHT - 1U,    /* 多路复用行数 */
        0xD3U, 0x00U,               /* 显示偏移为 0 */
        0x40U,                      /* 起始行为 0 */
        0x8DU, 0x14U,               /* 开启内部电荷泵 */
        0x20U, 0x02U,               /* 页寻址模式 */
        0xA1U, 0xC8U,               /* SEG 重映射及 COM 反向扫描 */
        0xDAU, OLED_COM_CONFIG,      /* COM 引脚配置 */
        OLED_CMD_CONTRAST, 0x7FU,
        0xD9U, 0xF1U,               /* 内部电荷泵预充电周期 */
        0xDBU, 0x20U,               /* VCOMH 为 0.77 x VCC */
        0xA4U, OLED_CMD_NORMAL,      /* 使用显存内容，正常显示 */
        0x2EU                       /* 关闭滚动 */
    };

    if (handle == NULL) return;
    handle->i2c = NULL;
    if (i2c == NULL || i2c->i2c_handle == NULL
        || (address != OLED_ADDRESS_LOW && address != OLED_ADDRESS_HIGH)) return;

    handle->i2c = i2c;
    handle->address = address;
    time_delay_ms(100);
    BSP_I2C_MemWrite(i2c, address, OLED_CONTROL_COMMAND, config, sizeof(config));
    OLED_Clear(handle);
    OLED_Update(handle);
    OLED_DisplayOn(handle);
}

void OLED_Update(OLED_Handle_t *handle)
{
    uint8_t page;
    uint8_t command[3] = {
        0xB0U,
        (uint8_t)(OLED_COLUMN_OFFSET & 0x0FU),
        (uint8_t)(0x10U | (OLED_COLUMN_OFFSET >> 4))
    };

    if (!OLED_IsValid(handle)) return;

    for (page = 0; page < OLED_PAGE_COUNT; page++)
    {
        /* 按可见区域的实际起始列定位，避免左侧裁切及右侧残留亮线。 */
        command[0] = (uint8_t)(0xB0U | page);
        BSP_I2C_MemWrite(handle->i2c, handle->address, OLED_CONTROL_COMMAND,
                        command, sizeof(command));
        BSP_I2C_MemWrite(handle->i2c, handle->address, OLED_CONTROL_DATA,
                        handle->buffer[page], OLED_WIDTH);
    }
}

void OLED_Clear(OLED_Handle_t *handle)
{
    OLED_Fill(handle, OLED_COLOR_BLACK);
}

void OLED_Fill(OLED_Handle_t *handle, uint8_t color)
{
    if (handle == NULL) return;
    memset(handle->buffer, color ? 0xFF : 0x00, sizeof(handle->buffer));
}

void OLED_DisplayOn(OLED_Handle_t *handle)
{
    OLED_WriteCommand(handle, OLED_CMD_DISPLAY_ON);
}

void OLED_DisplayOff(OLED_Handle_t *handle)
{
    OLED_WriteCommand(handle, OLED_CMD_DISPLAY_OFF);
}

void OLED_SetContrast(OLED_Handle_t *handle, uint8_t contrast)
{
    uint8_t command[2] = {OLED_CMD_CONTRAST, contrast};

    if (!OLED_IsValid(handle)) return;
    BSP_I2C_MemWrite(handle->i2c, handle->address, OLED_CONTROL_COMMAND,
                    command, sizeof(command));
}

void OLED_SetInvert(OLED_Handle_t *handle, uint8_t enable)
{
    OLED_WriteCommand(handle, enable ? OLED_CMD_INVERT : OLED_CMD_NORMAL);
}

void OLED_DrawPoint(OLED_Handle_t *handle, uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t mask;

    if (handle == NULL || x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    mask = (uint8_t)(1U << (y & 7U));
    if (color) {
        handle->buffer[y >> 3][x] |= mask;
    } else {
        handle->buffer[y >> 3][x] &= (uint8_t)~mask;
    }
}

void OLED_ShowChar(OLED_Handle_t *handle, uint8_t x, uint8_t y, char ch)
{
    uint8_t column;
    uint8_t row;
    uint8_t value;
    uint8_t index = (uint8_t)ch;

    if (handle == NULL || x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (index < 0x20U || index > 0x7EU) index = (uint8_t)'?';

    for (column = 0; column < OLED_FONT_WIDTH; column++)
    {
        value = column < OLED_FONT_WIDTH - 1U ? OLED_Font[index - 0x20U][column] : 0;
        for (row = 0; row < OLED_FONT_HEIGHT; row++)
        {
            OLED_DrawPoint(handle, (uint8_t)(x + column), (uint8_t)(y + row),
                           (uint8_t)((value >> row) & 1U));
        }
    }
}

void OLED_ShowString(OLED_Handle_t *handle, uint8_t x, uint8_t y, const char *text)
{
    uint8_t start_x = x;

    if (handle == NULL || text == NULL
        || x > OLED_WIDTH - OLED_FONT_WIDTH || y > OLED_HEIGHT - OLED_FONT_HEIGHT) return;

    while (*text != '\0')
    {
        if (*text == '\r') {
            x = start_x;
            text++;
            continue;
        }
        if (*text == '\n') {
            x = start_x;
            y += OLED_FONT_HEIGHT;
            text++;
        } else {
            if (x > OLED_WIDTH - OLED_FONT_WIDTH) {
                x = start_x;
                y += OLED_FONT_HEIGHT;
            }
            if (y > OLED_HEIGHT - OLED_FONT_HEIGHT) return;
            OLED_ShowChar(handle, x, y, *text++);
            x += OLED_FONT_WIDTH;
        }
        if (y > OLED_HEIGHT - OLED_FONT_HEIGHT) return;
    }
}

void OLED_ShowNum(OLED_Handle_t *handle, uint8_t x, uint8_t y,
    uint32_t number, uint8_t length)
{
    char text[11];
    uint8_t i;

    if (handle == NULL || length == 0 || length > 10U) return;

    text[length] = '\0';
    for (i = length; i > 0; i--)
    {
        text[i - 1U] = (char)('0' + number % 10U);
        number /= 10U;
    }
    OLED_ShowString(handle, x, y, text);
}

void OLED_DrawBitmap(OLED_Handle_t *handle, uint8_t x, uint8_t y,
    uint8_t width, uint8_t height, const uint8_t *data)
{
    uint16_t column;
    uint16_t row;
    uint8_t value;

    if (handle == NULL || data == NULL || width == 0 || height == 0
        || x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    for (row = 0; row < height && row < OLED_HEIGHT - y; row++)
    {
        for (column = 0; column < width && column < OLED_WIDTH - x; column++)
        {
            value = data[(row >> 3) * width + column];
            OLED_DrawPoint(handle, (uint8_t)(x + column), (uint8_t)(y + row),
                           (uint8_t)((value >> (row & 7U)) & 1U));
        }
    }
}
