/***************************************************************
 * time_config.h
 * 时间组件 — 集中参数配置
***************************************************************/
#ifndef TIME_CONFIG_H
#define TIME_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================
 *  硬件定时器标识（与 wk_tmr13_init 保持一致）
 *==================================================================*/
#define TIME_TMR                TMR13

/*==================================================================
 *  定时器基础参数（必须与 wk_tmr13_init 中的配置完全一致）
 *==================================================================*/
#define TIME_TMR_PERIOD         999U    /*!< 自动重装载值 (tmr_pr)     */
#define TIME_TMR_PRESCALER      239U    /*!< 预分频器值   (tmr_div)    */

/*==================================================================
 *  定时器时钟源频率 [Hz]（用户必须根据实际 PCLK 修改）
 *
 *  典型值（AT32F403A）：
 *    - 系统时钟 240MHz, APB1 = 120MHz, 定时器时钟 = 240MHz  → 填 240000000
 *    - 系统时钟 120MHz, APB1 =  60MHz, 定时器时钟 = 120MHz  → 填 120000000
 *
 *  计算公式：
 *    if (APB_prescaler == 1)  TimerClock = APB_Clock
 *    else                     TimerClock = 2 × APB_Clock
 *==================================================================*/
#define TIME_TMR_CLOCK_HZ       240000000UL

/*==================================================================
 *  以下为自动推导常量 —— 无需修改
 *
 *  注意：所有分子/分母均使用 ULL 后缀，
 *        确保预处理器 #if 和 C 编译阶段均使用 64 位算术。
 *==================================================================*/

/** 每 tick 的微秒数（分子/分母拆分以保留精度） */
#define TIME_TICK_US_NUM        ((TIME_TMR_PRESCALER + 1ULL) * 1000000ULL)
#define TIME_TICK_US_DEN        (TIME_TMR_CLOCK_HZ)

/** 每次溢出的微秒数（分子/分母拆分） */
#define TIME_OVERFLOW_US_NUM    ((TIME_TMR_PRESCALER + 1ULL) * \
                                 (TIME_TMR_PERIOD   + 1ULL) * 1000000ULL)
#define TIME_OVERFLOW_US_DEN    (TIME_TMR_CLOCK_HZ)

/* 编译期整除性检查（预处理器常量表达式，不包含任何 C 类型转换） */
#if (TIME_OVERFLOW_US_NUM % TIME_OVERFLOW_US_DEN == 0)
  #define TIME_OVERFLOW_US      ((uint32_t)(TIME_OVERFLOW_US_NUM / TIME_OVERFLOW_US_DEN))
#else
  #error "TIME_OVERFLOW_US not an integer: adjust prescaler/period or disable this check"
#endif

/*==================================================================
 *  FreeRTOS 集成开关
 *==================================================================*/
#ifndef TIME_USE_FREERTOS
  #define TIME_USE_FREERTOS     1   /*!< 1 = enable vTaskDelay in time_delay_ms */
#endif

#ifdef __cplusplus
}
#endif

#endif /* TIME_CONFIG_H */
