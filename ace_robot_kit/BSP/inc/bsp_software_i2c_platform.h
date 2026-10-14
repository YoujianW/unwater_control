/***************************************************************
 * bsp_software_i2c_platform.h
 * 软件 I2C 平台适配 — GPIO、时间基准及延时集中于此
 ***************************************************************/
#ifndef BSP_SOFTWARE_I2C_PLATFORM_H
#define BSP_SOFTWARE_I2C_PLATFORM_H

#include "at32f403a_407_conf.h"
#include "time_core.h"

#define BSP_SOFTWARE_I2C_VALID_PORT(port) \
    ((port) == GPIOA || (port) == GPIOB || (port) == GPIOC || \
     (port) == GPIOD || (port) == GPIOE)

/* 开漏：写 1 是释放总线，写 0 才主动拉低；读值必须来自输入寄存器。
 * SDA 始终保持开漏输出，释放后即可读取，不需要切换输入/输出模式。
 */
#define BSP_SOFTWARE_I2C_RELEASE(port, pin) gpio_bits_set((gpio_type *)(port), (pin))
#define BSP_SOFTWARE_I2C_LOW(port, pin)     gpio_bits_reset((gpio_type *)(port), (pin))
#define BSP_SOFTWARE_I2C_READ(port, pin) \
    (gpio_input_data_bit_read((gpio_type *)(port), (pin)) != RESET)
#define BSP_SOFTWARE_I2C_DELAY_US(us)       time_delay_us(us)
#define BSP_SOFTWARE_I2C_GET_US()           ((uint32_t)time_get_us())

/* 移植约定：InitPin 在进入开漏模式前预置输出为释放状态。 */
static void BSP_SOFTWARE_I2C_PlatformInitPin(void *port, uint16_t pin)
{
    gpio_init_type config;
    crm_periph_clock_type clock;

    if (port == GPIOA) clock = CRM_GPIOA_PERIPH_CLOCK;
    else if (port == GPIOB) clock = CRM_GPIOB_PERIPH_CLOCK;
    else if (port == GPIOC) clock = CRM_GPIOC_PERIPH_CLOCK;
    else if (port == GPIOD) clock = CRM_GPIOD_PERIPH_CLOCK;
    else clock = CRM_GPIOE_PERIPH_CLOCK; /* 调用前已校验端口 */

    crm_periph_clock_enable(clock, TRUE);
    gpio_bits_set((gpio_type *)port, pin);
    gpio_default_para_init(&config);
    config.gpio_pins = pin;
    config.gpio_mode = GPIO_MODE_OUTPUT;
    config.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
    config.gpio_pull = GPIO_PULL_NONE;
    config.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init((gpio_type *)port, &config);
}

#define BSP_SOFTWARE_I2C_INIT_PIN(port, pin) BSP_SOFTWARE_I2C_PlatformInitPin((port), (pin))

#endif
