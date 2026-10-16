/***************************************************************
 * bsp_spi_platform.h
 * SPI 平台适配宏 — 硬件、片选及时间基准集中于此
 ***************************************************************/
#ifndef BSP_SPI_PLATFORM_H
#define BSP_SPI_PLATFORM_H

#include "at32f403a_407_conf.h"
#include "time_core.h"

#define BSP_SPI_TypeDef                spi_type
#define BSP_SPI_VALID_BUS(bus)         ((bus) == SPI1 || (bus) == SPI2 || (bus) == SPI3 || (bus) == SPI4)
#define BSP_SPI_VALID_PORT(port) \
    ((port) == GPIOA || (port) == GPIOB || (port) == GPIOC || \
     (port) == GPIOD || (port) == GPIOE)
#define BSP_SPI_VALID_PIN(pin) \
    ((pin) != 0U && (((pin) & ((pin) - 1U)) == 0U))

#define BSP_SPI_CAN_TRANSFER()         (__get_IPSR() == 0 && __get_PRIMASK() == 0 && __get_BASEPRI() == 0)
#define BSP_SPI_GET_MS()               time_get_ms()
#define BSP_SPI_FLAG(bus, flag)        (spi_i2s_flag_get((BSP_SPI_TypeDef *)(bus), (flag)) != RESET)
#define BSP_SPI_TX_EMPTY_FLAG          SPI_I2S_TDBE_FLAG
#define BSP_SPI_RX_READY_FLAG          SPI_I2S_RDBF_FLAG
#define BSP_SPI_BUSY_FLAG              SPI_I2S_BF_FLAG
#define BSP_SPI_ERROR_FLAGS            (SPI_MMERR_FLAG | SPI_I2S_ROERR_FLAG | SPI_CCERR_FLAG)
#define BSP_SPI_SEND_BYTE(bus, value)  spi_i2s_data_transmit((BSP_SPI_TypeDef *)(bus), (value))
#define BSP_SPI_READ_BYTE(bus)         ((uint8_t)spi_i2s_data_receive((BSP_SPI_TypeDef *)(bus)))

/* 读数据寄存器后读状态寄存器，同时丢弃残留接收数据并清除接收溢出。 */
#define BSP_SPI_CLEAR_RX(bus)          spi_i2s_flag_clear((BSP_SPI_TypeDef *)(bus), SPI_I2S_ROERR_FLAG)
#define BSP_SPI_CS_LOW(port, pin)      gpio_bits_reset((gpio_type *)(port), (pin))
#define BSP_SPI_CS_HIGH(port, pin)     gpio_bits_set((gpio_type *)(port), (pin))

/* 只校验轮询收发所需模式，不重新配置或复位外设。 */
#define BSP_SPI_BUS_READY(bus) \
    (((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.spien != 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.msten != 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.fbn == SPI_FRAME_8BIT && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.slben == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.ora == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl1_bit.ccen == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->i2sctrl_bit.i2smsel == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl2_bit.dmaten == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl2_bit.dmaren == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl2_bit.tdbeie == 0 && \
     ((BSP_SPI_TypeDef *)(bus))->ctrl2_bit.rdbfie == 0)

#endif
