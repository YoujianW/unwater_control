/***************************************************************
 * bsp_i2c_platform.h
 * I2C 平台适配宏 — 硬件、时间基准及临界区操作集中于此
 ***************************************************************/
#ifndef BSP_I2C_PLATFORM_H
#define BSP_I2C_PLATFORM_H

#include "at32f403a_407_conf.h"
#include "time_core.h"

#define BSP_I2C_TypeDef                i2c_type
#define BSP_I2C_VALID_BUS(bus)         ((bus) == I2C1 || (bus) == I2C2 || (bus) == I2C3)
#define BSP_I2C_CAN_TRANSFER()         (__get_IPSR() == 0 && __get_PRIMASK() == 0 && __get_BASEPRI() == 0)
#define BSP_I2C_GET_MS()               time_get_ms()
#define BSP_I2C_FLAG(bus, flag)        (i2c_flag_get((BSP_I2C_TypeDef *)(bus), (flag)) != RESET)
#define BSP_I2C_START_FLAG             I2C_STARTF_FLAG
#define BSP_I2C_ADDRESS_FLAG           I2C_ADDR7F_FLAG
#define BSP_I2C_TX_EMPTY_FLAG          I2C_TDBE_FLAG
#define BSP_I2C_TX_DONE_FLAG           I2C_TDC_FLAG
#define BSP_I2C_RX_READY_FLAG          I2C_RDBF_FLAG
#define BSP_I2C_BUSY_FLAG              I2C_BUSYF_FLAG
#define BSP_I2C_START(bus)             i2c_start_generate((BSP_I2C_TypeDef *)(bus))
#define BSP_I2C_STOP(bus)              i2c_stop_generate((BSP_I2C_TypeDef *)(bus))
#define BSP_I2C_STOP_PENDING(bus)      (((BSP_I2C_TypeDef *)(bus))->ctrl1_bit.genstop != 0)
#define BSP_I2C_IS_MASTER(bus)         BSP_I2C_FLAG((bus), I2C_TRMODE_FLAG)
#define BSP_I2C_SEND_ADDRESS(bus, addr, read) \
    i2c_7bit_address_send((BSP_I2C_TypeDef *)(bus), (uint8_t)((addr) << 1), \
                         (read) ? I2C_DIRECTION_RECEIVE : I2C_DIRECTION_TRANSMIT)
#define BSP_I2C_SEND_BYTE(bus, value)  i2c_data_send((BSP_I2C_TypeDef *)(bus), (value))
#define BSP_I2C_READ_BYTE(bus)         i2c_data_receive((BSP_I2C_TypeDef *)(bus))
#define BSP_I2C_CLEAR_ADDRESS(bus)     i2c_flag_clear((BSP_I2C_TypeDef *)(bus), I2C_ADDR7F_FLAG)
#define BSP_I2C_ACK_ENABLE(bus, en)    i2c_ack_enable((BSP_I2C_TypeDef *)(bus), (en) ? TRUE : FALSE)
#define BSP_I2C_ACK_NEXT(bus, next) \
    i2c_master_receive_ack_set((BSP_I2C_TypeDef *)(bus), \
                              (next) ? I2C_MASTER_ACK_NEXT : I2C_MASTER_ACK_CURRENT)
#define BSP_I2C_CLEAR_FLAGS(bus) \
    i2c_flag_clear((BSP_I2C_TypeDef *)(bus), I2C_ACKFAIL_FLAG | I2C_BUSERR_FLAG | \
                   I2C_ARLOST_FLAG | I2C_OUF_FLAG | I2C_TMOUT_FLAG)

static uint32_t BSP_I2C_PlatformEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

#define BSP_I2C_ENTER_CRITICAL()       BSP_I2C_PlatformEnterCritical()
#define BSP_I2C_EXIT_CRITICAL(state)   __set_PRIMASK(state)

#endif
