/***************************************************************
 * bsp_tmr_capture.h
 * tmr_capture 通用接口（平台无关）
***************************************************************/
#ifndef BSP_TMR_CAPTURE_H
#define BSP_TMR_CAPTURE_H

#include <stdint.h>

/* 定时器输入捕获实例结构体 */
typedef struct {
    void       *tim_handle;              /* 定时器外设基地址（如 TMR2, TMR3...） */
    uint8_t     channel;                 /* 通道号 (1-4) */
    
    /* 内部状态（BSP层维护） */
    volatile uint32_t last_ccr;          /* 上次捕获值 */
    volatile uint32_t overflow_cnt;      /* 溢出计数 */
    volatile uint32_t period_ticks;      /* 最新捕获周期（定时器时钟周期数） */
    volatile uint8_t  first_capture;     /* 首次捕获标志 */
} BSP_TMR_Capture_t;

/**
 * @brief 初始化捕获实例（仅初始化内部状态，不操作硬件寄存器）
 * @param inst       捕获实例指针
 * @param tim_handle 定时器外设基地址
 * @param channel    通道号 (1-4)
 */
void BSP_TMR_Capture_Init(BSP_TMR_Capture_t *inst, void *tim_handle, uint8_t channel);

/**
 * @brief 定时器捕获中断处理函数
 * @param inst 捕获实例指针
 * @note 在 at32f403a_407_int.c 的中断函数中调用
 */
void BSP_TMR_Capture_IRQHandler(BSP_TMR_Capture_t *inst);

/**
 * @brief 获取最新捕获周期（单位：定时器时钟周期数）
 * @param inst 捕获实例指针
 * @return 周期值，0表示尚无有效捕获
 */
uint32_t BSP_TMR_Capture_GetPeriodTicks(const BSP_TMR_Capture_t *inst);

/**
 * @brief 重置捕获状态
 * @param inst 捕获实例指针
 */
void BSP_TMR_Capture_Reset(BSP_TMR_Capture_t *inst);

#endif
