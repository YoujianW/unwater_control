#include "button.h"
#include "wk_gpio.h"

static Button key1_button;
static Button key2_button;

static uint8_t BUTTON_ReadLevel(uint8_t button_id)
{
    switch (button_id)
    {
        case 1:
            return (uint8_t)gpio_input_data_bit_read(KEY1_GPIO_PORT, KEY1_PIN);
        case 2:
            return (uint8_t)gpio_input_data_bit_read(KEY2_GPIO_PORT, KEY2_PIN);
        default:
            return 1;
    }
}

void BUTTON_Init(BtnCallback key1_press_down, BtnCallback key1_press_up,
                 BtnCallback key2_press_down, BtnCallback key2_press_up)
{
    /* KEY1、KEY2 为上拉输入，低电平按下。 */
    button_init(&key1_button, BUTTON_ReadLevel, 0, 1);
    button_init(&key2_button, BUTTON_ReadLevel, 0, 2);

    button_attach(&key1_button, BTN_PRESS_DOWN, key1_press_down, NULL);
    button_attach(&key1_button, BTN_PRESS_UP, key1_press_up, NULL);
    button_attach(&key2_button, BTN_PRESS_DOWN, key2_press_down, NULL);
    button_attach(&key2_button, BTN_PRESS_UP, key2_press_up, NULL);

    button_start(&key1_button);
    button_start(&key2_button);
}

void BUTTON_Ticks(void)
{
    button_ticks();
}
