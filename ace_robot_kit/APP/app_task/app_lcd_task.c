#include "app_lcd_task.h"
#include "app_task.h"
#include "at32f403a_407_wk_config.h"
#include "st7789.h"
#include "time_core.h"
#include "lvgl.h"

#define LCD_INIT_RETRY_MS         1000
#define LCD_TASK_INTERVAL_MS      5
#define LCD_BUFFER_ROWS           10

static ST7789_Handle_t lcd_handle;
static lv_obj_t *time_label;
static lv_obj_t *progress_bar;
static lv_obj_t *value_label;

static void lcd_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color)
{
    ST7789_DrawImage(&lcd_handle, area->x1, area->y1,
        area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (const uint16_t *)color);
    lv_disp_flush_ready(disp_drv);
}

static void lcd_demo_update(lv_timer_t *timer)
{
    uint32_t now = time_get_ms();
    uint32_t seconds = now / 1000U;
    uint32_t value = (now / 100U) % 101U;

    (void)timer;
    lv_label_set_text_fmt(time_label, "Time  %02u:%02u:%02u",
        (unsigned int)(seconds / 3600U % 24U),
        (unsigned int)(seconds / 60U % 60U), (unsigned int)(seconds % 60U));
    lv_bar_set_value(progress_bar, value, LV_ANIM_OFF);
    lv_label_set_text_fmt(value_label, "%u%%", (unsigned int)value);
}

void lcd_Task_func(void *pvParameters)
{
    static BSP_SPI_Handle_t spi_handle;
    static lv_color_t buffer[ST7789_WIDTH * LCD_BUFFER_ROWS];
    static lv_disp_draw_buf_t draw_buf;
    static lv_disp_drv_t disp_drv;

    (void)pvParameters;
    BSP_SPI_Init(&spi_handle, SPI1, ST7789_CS_GPIO_PORT, ST7789_CS_PIN);
    while (!ST7789_Init(&lcd_handle, &spi_handle))
    {
        vTaskDelay(pdMS_TO_TICKS(LCD_INIT_RETRY_MS));
    }

    ST7789_SetRotation(&lcd_handle, 2);
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buffer, NULL, ST7789_WIDTH * LCD_BUFFER_ROWS);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ST7789_WIDTH;
    disp_drv.ver_res = ST7789_HEIGHT;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = lcd_flush;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101827), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "LVGL DEMO");
    lv_obj_set_style_text_color(title, lv_color_hex(0x38BDF8), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "ST7789 240x240");
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 52);

    time_label = lv_label_create(screen);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 96);

    progress_bar = lv_bar_create(screen);
    lv_obj_set_size(progress_bar, 200, 16);
    lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 144);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x25344B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x22C55E), LV_PART_INDICATOR);

    value_label = lv_label_create(screen);
    lv_obj_align(value_label, LV_ALIGN_TOP_MID, 0, 176);
    lcd_demo_update(NULL);
    lv_timer_create(lcd_demo_update, 100, NULL);

    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(LCD_TASK_INTERVAL_MS));
    }
}
