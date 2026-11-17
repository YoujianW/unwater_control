#include "app_lcd_task.h"
#include "app_task.h"
#include "at32f403a_407_wk_config.h"
#include "st7789.h"
#include "time_core.h"
#include "lvgl.h"

#define LCD_INIT_RETRY_MS         1000
#define LCD_TASK_INTERVAL_MS      5
#define LCD_BUFFER_ROWS           10
#define LCD_COLOR_BACKGROUND      0x0B1220
#define LCD_COLOR_CARD            0x162237
#define LCD_COLOR_TEXT            0xF4F7FB
#define LCD_COLOR_MUTED           0x8191AA
#define LCD_COLOR_CYAN            0x38BDF8
#define LCD_COLOR_GREEN           0x35D098
#define LCD_COLOR_AMBER           0xFBBF24

static ST7789_Handle_t lcd_handle;
static lv_obj_t *time_label;
static lv_obj_t *temperature_arc;
static lv_obj_t *temperature_label;
static lv_obj_t *vref_label;
static lv_obj_t *progress_bar;
static lv_obj_t *value_label;
static lv_obj_t *signal_chart;
static lv_chart_series_t *signal_series;

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
    int32_t temperature = adc_temperature_centi_c / 100;
    int32_t value = 50 + (int32_t)lv_trigo_sin((int16_t)(now / 20U % 360U))
        * 35 / LV_TRIGO_SIN_MAX;

    (void)timer;
    lv_label_set_text_fmt(time_label, "%02u:%02u:%02u",
        (unsigned int)(seconds / 3600U % 24U),
        (unsigned int)(seconds / 60U % 60U), (unsigned int)(seconds % 60U));
    lv_arc_set_value(temperature_arc, (int16_t)temperature);
    lv_label_set_text_fmt(temperature_label, "%ld", (long)temperature);
    lv_label_set_text_fmt(vref_label, "%u", (unsigned int)adc_vref_raw);
    lv_bar_set_value(progress_bar, value, LV_ANIM_OFF);
    lv_label_set_text_fmt(value_label, "%ld%%", (long)value);
    lv_chart_set_next_value(signal_chart, signal_series, (lv_coord_t)value);
}

static lv_obj_t *lcd_label(lv_obj_t *parent, const char *text,
    lv_coord_t x, lv_coord_t y, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *lcd_card(lv_obj_t *parent,
    lv_coord_t x, lv_coord_t y, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, width, height);
    lv_obj_set_style_bg_color(card, lv_color_hex(LCD_COLOR_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static void lcd_ui_create(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(LCD_COLOR_BACKGROUND), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(LCD_COLOR_TEXT), 0);

    lcd_label(screen, "UNWATER", 12, 8, &lv_font_montserrat_20, LCD_COLOR_TEXT);
    lcd_label(screen, "CONTROL PANEL", 12, 33, &lv_font_montserrat_12, LCD_COLOR_MUTED);
    time_label = lcd_label(screen, "00:00:00", 174, 33,
        &lv_font_montserrat_12, LCD_COLOR_MUTED);

    lv_obj_t *status = lv_led_create(screen);
    lv_obj_set_pos(status, 184, 14);
    lv_obj_set_size(status, 7, 7);
    lv_obj_set_style_shadow_width(status, 0, 0);
    lv_led_set_color(status, lv_color_hex(LCD_COLOR_GREEN));
    lv_led_on(status);
    lcd_label(screen, "RUN", 197, 10, &lv_font_montserrat_12, LCD_COLOR_GREEN);

    lv_obj_t *card = lcd_card(screen, 10, 54, 220, 108);
    temperature_arc = lv_arc_create(card);
    lv_obj_set_pos(temperature_arc, 9, 9);
    lv_obj_set_size(temperature_arc, 84, 84);
    lv_arc_set_rotation(temperature_arc, 135);
    lv_arc_set_bg_angles(temperature_arc, 0, 270);
    lv_arc_set_range(temperature_arc, 0, 100);
    lv_obj_remove_style(temperature_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(temperature_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(temperature_arc, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_width(temperature_arc, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(temperature_arc, lv_color_hex(0x2B3A52), LV_PART_MAIN);
    lv_obj_set_style_arc_color(temperature_arc, lv_color_hex(LCD_COLOR_CYAN), LV_PART_INDICATOR);

    temperature_label = lcd_label(temperature_arc, "0", 0, 0,
        &lv_font_montserrat_20, LCD_COLOR_TEXT);
    lv_obj_align(temperature_label, LV_ALIGN_CENTER, 0, -6);
    lv_obj_t *unit = lcd_label(temperature_arc, "C", 0, 0,
        &lv_font_montserrat_12, LCD_COLOR_MUTED);
    lv_obj_align(unit, LV_ALIGN_CENTER, 0, 14);
    lcd_label(card, "CHIP TEMP", 16, 89, &lv_font_montserrat_12, LCD_COLOR_MUTED);

    lcd_label(card, "VREF RAW", 108, 13, &lv_font_montserrat_12, LCD_COLOR_MUTED);
    vref_label = lcd_label(card, "0", 108, 31, &lv_font_montserrat_20, LCD_COLOR_TEXT);
    lcd_label(card, "DEMO", 108, 65, &lv_font_montserrat_12, LCD_COLOR_MUTED);
    value_label = lcd_label(card, "0%", 0, 0, &lv_font_montserrat_12, LCD_COLOR_AMBER);
    lv_obj_align(value_label, LV_ALIGN_TOP_RIGHT, -12, 65);

    progress_bar = lv_bar_create(card);
    lv_obj_set_pos(progress_bar, 108, 87);
    lv_obj_set_size(progress_bar, 100, 6);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x2B3A52), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(LCD_COLOR_GREEN), LV_PART_INDICATOR);

    card = lcd_card(screen, 10, 170, 220, 60);
    lcd_label(card, "SIGNAL", 12, 7, &lv_font_montserrat_12, LCD_COLOR_TEXT);
    lv_obj_t *demo = lcd_label(card, "DEMO", 0, 0, &lv_font_montserrat_12, LCD_COLOR_MUTED);
    lv_obj_align(demo, LV_ALIGN_TOP_RIGHT, -12, 7);

    signal_chart = lv_chart_create(card);
    lv_obj_set_pos(signal_chart, 8, 26);
    lv_obj_set_size(signal_chart, 204, 26);
    lv_obj_set_style_bg_opa(signal_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(signal_chart, 0, 0);
    lv_obj_set_style_pad_all(signal_chart, 0, 0);
    lv_obj_set_style_line_color(signal_chart, lv_color_hex(0x2B3A52), LV_PART_MAIN);
    lv_obj_set_style_line_width(signal_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(signal_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(signal_chart, 0, LV_PART_INDICATOR);
    lv_chart_set_type(signal_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(signal_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(signal_chart, 32);
    lv_chart_set_div_line_count(signal_chart, 2, 5);
    signal_series = lv_chart_add_series(signal_chart,
        lv_color_hex(LCD_COLOR_CYAN), LV_CHART_AXIS_PRIMARY_Y);
    for (uint16_t i = 0; i < 32; i++)
    {
        lv_chart_set_next_value(signal_chart, signal_series,
            (lv_coord_t)(50 + (int32_t)lv_trigo_sin((int16_t)(i * 12U)) * 35 / LV_TRIGO_SIN_MAX));
    }
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

    lcd_ui_create();
    lcd_demo_update(NULL);
    lv_timer_create(lcd_demo_update, 200, NULL);

    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(LCD_TASK_INTERVAL_MS));
    }
}
