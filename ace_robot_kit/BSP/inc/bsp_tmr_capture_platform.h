/***************************************************************
 * bsp_tmr_capture_platform.h
 * tmr_capture 平台适配宏 — 所有 AT32 相关操作集中于此
***************************************************************/
#ifndef BSP_TMR_CAPTURE_PLATFORM_H
#define BSP_TMR_CAPTURE_PLATFORM_H

#include "at32f403a_407_conf.h"

/* ====== 定时器类型定义（跨平台统一） ====== */
#define BSP_TIM_TypeDef                 tmr_type

/* ====== 中断标志操作 ====== */
#define BSP_TIM_GET_OVF_FLAG(htim)      tmr_flag_get((BSP_TIM_TypeDef*)(htim), TMR_OVF_FLAG)
#define BSP_TIM_CLR_OVF_FLAG(htim)      tmr_flag_clear((BSP_TIM_TypeDef*)(htim), TMR_OVF_FLAG)

#define BSP_TIM_GET_CC_FLAG(htim, ch)   tmr_flag_get((BSP_TIM_TypeDef*)(htim),         \
    ((ch) == 1) ? TMR_C1_FLAG : ((ch) == 2) ? TMR_C2_FLAG :                            \
    ((ch) == 3) ? TMR_C3_FLAG : TMR_C4_FLAG)
#define BSP_TIM_CLR_CC_FLAG(htim, ch)   tmr_flag_clear((BSP_TIM_TypeDef*)(htim),        \
    ((ch) == 1) ? TMR_C1_FLAG : ((ch) == 2) ? TMR_C2_FLAG :                            \
    ((ch) == 3) ? TMR_C3_FLAG : TMR_C4_FLAG)

/* ====== 读取捕获寄存器 ====== */
#define BSP_TIM_GET_CCR(htim, ch)       tmr_channel_value_get((BSP_TIM_TypeDef*)(htim), \
    ((ch) == 1) ? TMR_SELECT_CHANNEL_1 : ((ch) == 2) ? TMR_SELECT_CHANNEL_2 :           \
    ((ch) == 3) ? TMR_SELECT_CHANNEL_3 : TMR_SELECT_CHANNEL_4)

/* ====== 获取自动重装载值（从寄存器直接读取，无需硬编码） ====== */
#define BSP_TIM_GET_ARR(htim)           (((BSP_TIM_TypeDef*)(htim))->pr)

/* ====== 设置捕获寄存器 ====== */
#define BSP_TIM_SET_CCR(htim, ch, val)  tmr_channel_value_set((BSP_TIM_TypeDef*)(htim), \
    ((ch) == 1) ? TMR_SELECT_CHANNEL_1 : ((ch) == 2) ? TMR_SELECT_CHANNEL_2 :           \
    ((ch) == 3) ? TMR_SELECT_CHANNEL_3 : TMR_SELECT_CHANNEL_4, (val))

#endif
