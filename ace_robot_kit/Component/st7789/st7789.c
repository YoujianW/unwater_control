/***************************************************************
 * st7789.c
 * ST7789 初始化及 RGB565 绘制，所有 SPI 操作通过 bsp_spi.h
 ***************************************************************/
#include "st7789.h"
#include "at32f403a_407_wk_config.h"
#include "time_core.h"
#include <stddef.h>

#define ST7789_CMD_SWRESET       0x01U
#define ST7789_CMD_SLPOUT        0x11U
#define ST7789_CMD_NORON         0x13U
#define ST7789_CMD_INVOFF        0x20U
#define ST7789_CMD_INVON         0x21U
#define ST7789_CMD_DISPOFF       0x28U
#define ST7789_CMD_DISPON        0x29U
#define ST7789_CMD_CASET         0x2AU
#define ST7789_CMD_RASET         0x2BU
#define ST7789_CMD_RAMWR         0x2CU
#define ST7789_CMD_MADCTL        0x36U
#define ST7789_CMD_COLMOD        0x3AU
#define ST7789_CHUNK_PIXELS      128U

/* 5 x 7 ASCII 字形，与项目 OLED 组件一致，补空白列和行组成 6 x 8 字符。 */
static const uint8_t ST7789_Font[95][5] = {
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

/* 调用方持有 CS；BSP 成功返回前已等待最后一位发送结束，可以切换 DC。 */
static uint8_t ST7789_CommandSelected(ST7789_Handle_t *handle,
    uint8_t command, const uint8_t *data, uint16_t size)
{
    gpio_bits_reset(ST7789_DC_GPIO_PORT, ST7789_DC_PIN);
    uint8_t ok = BSP_SPI_SendBuffer(handle->spi, &command, 1);
    gpio_bits_set(ST7789_DC_GPIO_PORT, ST7789_DC_PIN);
    if (ok && size > 0) ok = BSP_SPI_SendBuffer(handle->spi, data, size);
    return ok;
}

static uint8_t ST7789_WriteCommand(ST7789_Handle_t *handle,
    uint8_t command, const uint8_t *data, uint16_t size)
{
    BSP_SPI_CS_Low(handle->spi);
    uint8_t ok = ST7789_CommandSelected(handle, command, data, size);
    BSP_SPI_CS_High(handle->spi);
    return ok;
}

/* 设置显存窗口并发送 RAMWR，成功时保持片选，随后由调用方发送像素并释放。 */
static uint8_t ST7789_BeginWrite(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint16_t x0 = (uint16_t)(x + handle->x_start);
    uint16_t y0 = (uint16_t)(y + handle->y_start);
    uint16_t x1 = (uint16_t)(x0 + width - 1U);
    uint16_t y1 = (uint16_t)(y0 + height - 1U);
    uint8_t columns[4] = {(uint8_t)(x0 >> 8), (uint8_t)x0,
                         (uint8_t)(x1 >> 8), (uint8_t)x1};
    uint8_t rows[4] = {(uint8_t)(y0 >> 8), (uint8_t)y0,
                      (uint8_t)(y1 >> 8), (uint8_t)y1};

    BSP_SPI_CS_Low(handle->spi);
    if (!ST7789_CommandSelected(handle, ST7789_CMD_CASET, columns, sizeof(columns))
        || !ST7789_CommandSelected(handle, ST7789_CMD_RASET, rows, sizeof(rows))
        || !ST7789_CommandSelected(handle, ST7789_CMD_RAMWR, NULL, 0)) {
        BSP_SPI_CS_High(handle->spi);
        return 0;
    }
    return 1;
}

static uint8_t ST7789_ClipRect(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t *width, uint16_t *height)
{
    if (*width == 0 || *height == 0 || x >= handle->width || y >= handle->height) return 0;
    if (*width > handle->width - x) *width = (uint16_t)(handle->width - x);
    if (*height > handle->height - y) *height = (uint16_t)(handle->height - y);
    return 1;
}

uint8_t ST7789_Init(ST7789_Handle_t *handle, BSP_SPI_Handle_t *spi)
{
    const uint8_t pixel_format = 0x55U; /* 16 位 RGB565，高字节先发送 */

    handle->spi = spi;
    BSP_SPI_CS_High(spi);
    gpio_bits_set(ST7789_DC_GPIO_PORT, ST7789_DC_PIN);
    ST7789_SetBacklight(handle, 0);
    gpio_bits_set(ST7789_RES_GPIO_PORT, ST7789_RES_PIN);
    time_delay_ms(10);
    gpio_bits_reset(ST7789_RES_GPIO_PORT, ST7789_RES_PIN);
    time_delay_ms(10);
    gpio_bits_set(ST7789_RES_GPIO_PORT, ST7789_RES_PIN);
    time_delay_ms(150);

    uint8_t ok = ST7789_WriteCommand(handle, ST7789_CMD_SWRESET, NULL, 0);
    if (ok) {
        time_delay_ms(150);
        ok = ST7789_WriteCommand(handle, ST7789_CMD_SLPOUT, NULL, 0);
    }
    if (ok) {
        time_delay_ms(120);
        ok = ST7789_WriteCommand(handle, ST7789_CMD_COLMOD, &pixel_format, 1)
            && ST7789_SetRotation(handle, 0)
            && ST7789_SetInvert(handle, ST7789_DEFAULT_INVERT)
            && ST7789_WriteCommand(handle, ST7789_CMD_NORON, NULL, 0)
            && ST7789_Clear(handle)
            && ST7789_DisplayOn(handle);
    }
    if (!ok) {
        BSP_SPI_CS_High(spi);
        ST7789_SetBacklight(handle, 0);
        handle->spi = NULL;
    }
    return ok;
}

uint8_t ST7789_SetRotation(ST7789_Handle_t *handle, uint8_t rotation)
{
    /* MADCTL：MY=0x80，MX=0x40，MV=0x20，BGR=0x08。 */
    static const uint8_t directions[4] = {0xC0U, 0xA0U, 0x00U, 0x60U};
    static const uint16_t offsets[4][2] = {
        {ST7789_X_OFFSET, ST7789_Y_OFFSET},
        {ST7789_Y_OFFSET, 240U - ST7789_WIDTH - ST7789_X_OFFSET},
        {240U - ST7789_WIDTH - ST7789_X_OFFSET, 320U - ST7789_HEIGHT - ST7789_Y_OFFSET},
        {320U - ST7789_HEIGHT - ST7789_Y_OFFSET, ST7789_X_OFFSET}
    };

    uint8_t madctl = directions[rotation] | (ST7789_COLOR_BGR ? 0x08U : 0U);
    if (!ST7789_WriteCommand(handle, ST7789_CMD_MADCTL, &madctl, 1)) return 0;
    handle->rotation = rotation;
    handle->width = (rotation & 1U) ? ST7789_HEIGHT : ST7789_WIDTH;
    handle->height = (rotation & 1U) ? ST7789_WIDTH : ST7789_HEIGHT;
    handle->x_start = offsets[rotation][0];
    handle->y_start = offsets[rotation][1];
    return 1;
}

uint8_t ST7789_SetInvert(ST7789_Handle_t *handle, uint8_t enable)
{
    return ST7789_WriteCommand(handle, enable ? ST7789_CMD_INVON : ST7789_CMD_INVOFF, NULL, 0);
}

uint8_t ST7789_SetBacklight(ST7789_Handle_t *handle, uint8_t enable)
{
    (void)handle;
    if (enable) gpio_bits_set(ST7789_BLK_GPIO_PORT, ST7789_BLK_PIN);
    else gpio_bits_reset(ST7789_BLK_GPIO_PORT, ST7789_BLK_PIN);
    return 1;
}

uint8_t ST7789_DisplayOn(ST7789_Handle_t *handle)
{
    if (!ST7789_WriteCommand(handle, ST7789_CMD_DISPON, NULL, 0)) return 0;
    time_delay_ms(20);
    return ST7789_SetBacklight(handle, 1);
}

uint8_t ST7789_DisplayOff(ST7789_Handle_t *handle)
{
    if (!ST7789_WriteCommand(handle, ST7789_CMD_DISPOFF, NULL, 0)) return 0;
    return ST7789_SetBacklight(handle, 0);
}

uint16_t ST7789_RGB565(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)(((uint16_t)(red & 0xF8U) << 8)
        | ((uint16_t)(green & 0xFCU) << 3) | (blue >> 3));
}

uint8_t ST7789_Clear(ST7789_Handle_t *handle)
{
    return ST7789_Fill(handle, ST7789_COLOR_BLACK);
}

uint8_t ST7789_Fill(ST7789_Handle_t *handle, uint16_t color)
{
    return ST7789_FillRect(handle, 0, 0, handle->width, handle->height, color);
}

uint8_t ST7789_DrawPoint(ST7789_Handle_t *handle, uint16_t x, uint16_t y, uint16_t color)
{
    return ST7789_FillRect(handle, x, y, 1, 1, color);
}

uint8_t ST7789_FillRect(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint8_t buffer[ST7789_CHUNK_PIXELS * 2U];
    uint8_t ok = 1;

    if (!ST7789_ClipRect(handle, x, y, &width, &height)) return 0;
    for (uint16_t i = 0; i < ST7789_CHUNK_PIXELS; i++) {
        buffer[i * 2U] = (uint8_t)(color >> 8);
        buffer[i * 2U + 1U] = (uint8_t)color;
    }
    if (!ST7789_BeginWrite(handle, x, y, width, height)) return 0;
    uint32_t remaining = (uint32_t)width * height;
    while (ok && remaining > 0) {
        uint16_t count = remaining > ST7789_CHUNK_PIXELS ? ST7789_CHUNK_PIXELS : (uint16_t)remaining;
        ok = BSP_SPI_SendBuffer(handle->spi, buffer, (uint16_t)(count * 2U));
        remaining -= count;
    }
    BSP_SPI_CS_High(handle->spi);
    return ok;
}

uint8_t ST7789_DrawImage(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels)
{
    uint8_t buffer[ST7789_CHUNK_PIXELS * 2U];
    uint16_t stride = width;
    uint8_t ok = 1;

    if (!ST7789_ClipRect(handle, x, y, &width, &height)) return 0;
    if (!ST7789_BeginWrite(handle, x, y, width, height)) return 0;
    for (uint16_t row = 0; ok && row < height; row++) {
        const uint16_t *source = pixels + (uint32_t)row * stride;
        uint16_t remaining = width;
        while (ok && remaining > 0) {
            uint16_t count = remaining > ST7789_CHUNK_PIXELS ? ST7789_CHUNK_PIXELS : remaining;
            for (uint16_t i = 0; i < count; i++) {
                uint16_t pixel = *source++;
                buffer[i * 2U] = (uint8_t)(pixel >> 8);
                buffer[i * 2U + 1U] = (uint8_t)pixel;
            }
            ok = BSP_SPI_SendBuffer(handle->spi, buffer, (uint16_t)(count * 2U));
            remaining = (uint16_t)(remaining - count);
        }
    }
    BSP_SPI_CS_High(handle->spi);
    return ok;
}

uint8_t ST7789_ShowChar(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t background)
{
    uint16_t pixels[ST7789_FONT_WIDTH * ST7789_FONT_HEIGHT];
    uint8_t character = (uint8_t)ch;

    if (character < 0x20U || character > 0x7EU) character = '?';
    for (uint16_t row = 0; row < ST7789_FONT_HEIGHT; row++) {
        for (uint16_t col = 0; col < ST7789_FONT_WIDTH; col++) {
            uint8_t bit = col < 5U ? (uint8_t)((ST7789_Font[character - 0x20U][col] >> row) & 1U) : 0U;
            pixels[row * ST7789_FONT_WIDTH + col] = bit ? color : background;
        }
    }
    return ST7789_DrawImage(handle, x, y, ST7789_FONT_WIDTH, ST7789_FONT_HEIGHT, pixels);
}

uint8_t ST7789_ShowString(ST7789_Handle_t *handle,
    uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t background)
{
    uint16_t start_x = x;

    if (x >= handle->width || y >= handle->height) return 0;
    while (*text != '\0') {
        if (y >= handle->height) break;
        char ch = *text++;
        if (ch == '\r') { x = start_x; continue; }
        if (ch == '\n') { x = start_x; y = (uint16_t)(y + ST7789_FONT_HEIGHT); continue; }
        if ((uint32_t)start_x + ST7789_FONT_WIDTH > handle->width) break;
        if ((uint32_t)x + ST7789_FONT_WIDTH > handle->width) {
            x = start_x;
            y = (uint16_t)(y + ST7789_FONT_HEIGHT);
        }
        if ((uint32_t)y + ST7789_FONT_HEIGHT > handle->height) break;
        if (!ST7789_ShowChar(handle, x, y, ch, color, background)) return 0;
        x = (uint16_t)(x + ST7789_FONT_WIDTH);
    }
    return 1;
}

uint8_t ST7789_ShowNum(ST7789_Handle_t *handle, uint16_t x, uint16_t y,
    uint32_t number, uint8_t length, uint16_t color, uint16_t background)
{
    char text[11];

    text[length] = '\0';
    for (uint8_t i = length; i > 0; i--) {
        text[i - 1U] = (char)('0' + number % 10U);
        number /= 10U;
    }
    return ST7789_ShowString(handle, x, y, text, color, background);
}
