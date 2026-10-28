/***************************************************************
 * bsp_pwc_platform.h
 * PWC 平台适配宏 — 所有 AT32 相关操作集中于此
 ***************************************************************/
#ifndef BSP_PWC_PLATFORM_H
#define BSP_PWC_PLATFORM_H

#include "at32f403a_407_conf.h"

/* ====== 只开启 PWC 外设时钟，不复位 PWC，也不重置 RTC 时间和备份寄存器数据 ====== */
#define BSP_PWC_CLOCK_ENABLE() \
    crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE)

/* 进入低功耗前需要满足以下条件，否则低功耗接口直接返回 0：
 * 在 main 或普通任务中调用，不要在中断函数里调用。
 * 调用时不要关闭中断，也不要放在 taskENTER_CRITICAL() 临界区内。
 * SLEEPONEXIT（中断处理完后自动睡眠）必须关闭，通常默认已关闭。
 */
#define BSP_PWC_CAN_ENTER() \
    (__get_IPSR() == 0U && __get_PRIMASK() == 0U && \
     __get_BASEPRI() == 0U && __get_FAULTMASK() == 0U && \
     (SCB->SCR & SCB_SCR_SLEEPONEXIT_Msk) == 0U)

#define BSP_PWC_DATA_SYNC()             __DSB()
#define BSP_PWC_INSTRUCTION_SYNC()      __ISB()

/* ====== 睡眠入口，WFE 的事件寄存器处理由底层驱动完成 ====== */
#define BSP_PWC_SLEEP_WFI()             pwc_sleep_mode_enter(PWC_SLEEP_ENTER_WFI)
#define BSP_PWC_SLEEP_WFE()             pwc_sleep_mode_enter(PWC_SLEEP_ENTER_WFE)

/* ====== 深睡眠选择及内部稳压器 ====== */
#define BSP_PWC_SELECT_DEEP_SLEEP()     (PWC->ctrl_bit.lpsel = FALSE)
#define BSP_PWC_REGULATOR_SET_NORMAL()  pwc_voltage_regulate_set(PWC_REGULATOR_ON)
#define BSP_PWC_REGULATOR_SET_LOW_POWER() \
    pwc_voltage_regulate_set(PWC_REGULATOR_LOW_POWER)

#define BSP_PWC_DEEP_SLEEP_WFI() \
    pwc_deep_sleep_mode_enter(PWC_DEEP_SLEEP_ENTER_WFI)
#define BSP_PWC_DEEP_SLEEP_WFE() \
    pwc_deep_sleep_mode_enter(PWC_DEEP_SLEEP_ENTER_WFE)
#define BSP_PWC_UPDATE_CORE_CLOCK()     system_core_clock_update()

/* ====== 待机入口及专用唤醒引脚（WKUP1 = PA0，上升沿） ====== */
#define BSP_PWC_STANDBY_ENTER()         pwc_standby_mode_enter()
#define BSP_PWC_WAKEUP_PIN_ENABLE(en) \
    pwc_wakeup_pin_enable(PWC_WAKEUP_PIN_1, ((en) != 0U) ? TRUE : FALSE)

/* ====== 状态标志，不在初始化或读取时自动清除 ====== */
#define BSP_PWC_WAKEUP_FLAG_GET()       (pwc_flag_get(PWC_WAKEUP_FLAG) != RESET)
#define BSP_PWC_STANDBY_FLAG_GET()      (pwc_flag_get(PWC_STANDBY_FLAG) != RESET)
#define BSP_PWC_WAKEUP_FLAG_CLEAR()     pwc_flag_clear(PWC_WAKEUP_FLAG)
#define BSP_PWC_STANDBY_FLAG_CLEAR()    pwc_flag_clear(PWC_STANDBY_FLAG)

#endif
