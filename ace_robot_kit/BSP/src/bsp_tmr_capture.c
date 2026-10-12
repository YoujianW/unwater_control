/***************************************************************
Copyright (C), 2024-2026, Your Company Name

FileName    : bsp_tmr_capture.c
Description : 定时器输入捕获 - 通用实现
Version     : V1.0

History:
Author      Date         Version     Description
YourName    2026-05-20   V1.0        创建文件

***************************************************************/
#include "bsp_tmr_capture.h"
#include "bsp_tmr_capture_platform.h"

void BSP_TMR_Capture_Init(BSP_TMR_Capture_t *inst, void *tim_handle, uint8_t channel)
{
    if (inst == 0 || tim_handle == 0 || channel < 1 || channel > 4) return;
    inst->tim_handle    = tim_handle;
    inst->channel       = channel;
    inst->last_ccr      = 0;
    inst->overflow_cnt  = 0;
    inst->period_ticks  = 0;
    inst->first_capture = 1;
}

void BSP_TMR_Capture_IRQHandler(BSP_TMR_Capture_t *inst)
{
    if (inst == 0 || inst->tim_handle == 0 || inst->channel < 1 || inst->channel > 4) return;
    void    *htim = inst->tim_handle;
    uint8_t  ch   = inst->channel;

    /* ---- 处理溢出中断 ---- */
    if (BSP_TIM_GET_OVF_FLAG(htim)) {
        BSP_TIM_CLR_OVF_FLAG(htim);
        inst->overflow_cnt++;
    }

    /* ---- 处理输入捕获中断 ---- */
    if (BSP_TIM_GET_CC_FLAG(htim, ch)) {
        BSP_TIM_CLR_CC_FLAG(htim, ch);

        uint32_t ccr    = BSP_TIM_GET_CCR(htim, ch);
        uint32_t arr    = BSP_TIM_GET_ARR(htim);   /* 从硬件读取重载值 */

        if (!inst->first_capture) {
            uint64_t delta;

            if (inst->overflow_cnt > 0) {
                /* 计数范围为 0 ~ arr，每次溢出包含 arr + 1 个 tick。 */
                delta = (uint64_t)(arr - inst->last_ccr) + 1ULL
                      + (uint64_t)ccr
                      + ((uint64_t)arr + 1ULL) * (inst->overflow_cnt - 1);
            } else {
                /* 无溢出 */
                if (ccr >= inst->last_ccr) {
                    delta = (uint64_t)(ccr - inst->last_ccr);
                } else {
                    /* 极端情况：捕获值小于上次值但未触发溢出中断 */
                    delta = (uint64_t)(arr - inst->last_ccr) + 1ULL + (uint64_t)ccr;
                }
            }

            inst->period_ticks = (delta > 0xFFFFFFFFUL) ? 0xFFFFFFFFUL : (uint32_t)delta;
        } else {
            inst->first_capture = 0;
        }

        inst->last_ccr     = ccr;
        inst->overflow_cnt = 0;
    }
}

uint32_t BSP_TMR_Capture_GetPeriodTicks(const BSP_TMR_Capture_t *inst)
{
    if (inst == 0) return 0;
    return inst->period_ticks;
}

void BSP_TMR_Capture_Reset(BSP_TMR_Capture_t *inst)
{
    if (inst == 0) return;
    inst->last_ccr      = 0;
    inst->overflow_cnt  = 0;
    inst->period_ticks  = 0;
    inst->first_capture = 1;
}
