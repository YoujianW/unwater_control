/***************************************************************
 * time_core.c
 * 时间组件 — 核心实现（融合硬件 TMR13 + FreeRTOS）

设计要点：
  1. 64-bit 微秒计数器 = overflow_cnt × TIME_OVERFLOW_US + 
     current_cnt × (TIME_TICK_US_NUM / TIME_TICK_US_DEN)
  2. 短临界区读取软件计数和待处理中断标志，ISR 安全
  3. 延迟函数自动选择 FreeRTOS vTaskDelay 或忙等待
***************************************************************/

#include "time_config.h"
#include "time_core.h"
#include "at32f403a_407_conf.h"

#if TIME_USE_FREERTOS
  #include "FreeRTOS.h"
  #include "task.h"
#endif

/*==================================================================
 *  内部状态
 *==================================================================*/

/** 硬件溢出累计次数（仅在 ISR 中修改） */
static volatile uint64_t time_overflow_cnt;

/*==================================================================
 *  初始化
 *==================================================================*/

/**
 * @brief  初始化时间组件
 */
void time_init(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    tmr_counter_value_set(TIME_TMR, 0);
    tmr_flag_clear(TIME_TMR, TMR_OVF_FLAG);
    time_overflow_cnt = 0;
    __set_PRIMASK(primask);
}

/*==================================================================
 *  时间获取（无锁，ISR 安全）
 *==================================================================*/

/**
 * @brief  获取 64-bit 原始 tick 累计值
 *
 * 在短临界区内取快照。若硬件已经溢出而 ISR 尚未执行，
 * 补计一次溢出并重新读取计数器，避免时间戳倒退。
 */
uint64_t time_get_ticks(void)
{
    uint32_t primask = __get_PRIMASK();
    uint64_t ovf;
    uint32_t cnt;

    __disable_irq();
    ovf = time_overflow_cnt;
    cnt = tmr_counter_value_get(TIME_TMR);
    if (tmr_flag_get(TIME_TMR, TMR_OVF_FLAG) == SET) {
        ovf++;
        cnt = tmr_counter_value_get(TIME_TMR);
    }
    __set_PRIMASK(primask);

    return ovf * (TIME_TMR_PERIOD + 1UL) + cnt;
}

/**
 * @brief  获取系统运行微秒数
 */
uint64_t time_get_us(void)
{
    uint64_t ticks = time_get_ticks();

    /* ticks × (μs/tick) = ticks × (prescaler+1) × 10^6 / clock */
    /* 先除后乘，避免长时间运行后乘法溢出。 */
    return (ticks / TIME_TICK_US_DEN) * TIME_TICK_US_NUM
         + (ticks % TIME_TICK_US_DEN) * TIME_TICK_US_NUM / TIME_TICK_US_DEN;
}


/**
 * @brief  获取系统运行毫秒数
 */
uint32_t time_get_ms(void)
{
    /* 直接基于 μs 转换，保持精度 */
    return (uint32_t)(time_get_us() / 1000ULL);
}

void time_get_runtime(time_runtime_t *runtime)
{
    uint64_t total_us;
    uint64_t total_ms;
    uint64_t total_seconds;

    if (runtime == NULL) {
        return;
    }

    /* 只读取一次硬件时间，保证所有拆分字段属于同一时刻。 */
    total_us = time_get_us();
    total_ms = total_us / 1000ULL;
    total_seconds = total_ms / 1000ULL;

    runtime->total_us = total_us;
    runtime->total_ms = total_ms;
    runtime->days = (uint32_t)(total_seconds / 86400ULL);
    runtime->hours = (uint8_t)((total_seconds / 3600ULL) % 24ULL);
    runtime->minutes = (uint8_t)((total_seconds / 60ULL) % 60ULL);
    runtime->seconds = (uint8_t)(total_seconds % 60ULL);
    runtime->millis = (uint16_t)(total_ms % 1000ULL);
}

/*==================================================================
 *  延迟函数
 *==================================================================*/

/**
 * @brief  微秒级忙等待
 */
void time_delay_us(uint32_t us)
{
    uint64_t start = time_get_us();

    while ((time_get_us() - start) < us) {
        /* busy loop — 仅适用于短延迟 */
    }
}

/**
 * @brief  毫秒级延迟（自动选择最优方式）
 */
void time_delay_ms(uint32_t ms)
{
#if TIME_USE_FREERTOS
    /*
     * 在 FreeRTOS 任务上下文中使用 vTaskDelay 让出 CPU，
     * 避免空转浪费。在 ISR 或调度器未启动时回退为忙等待。
     */
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        /*
         * 进一步判断是否在 ISR 中：
         * ISR 中调用 vTaskDelay 会导致 assert，所以检查
         * 中断嵌套标志位（Cortex-M4 的 IPSR / ICSR）。
         */
        if (__get_IPSR() == 0) {
            /* 在任务上下文中 */
            vTaskDelay(pdMS_TO_TICKS(ms));
            return;
        }
    }
#endif
    /* 回退：忙等待 */
    uint64_t start = time_get_us();
    while ((time_get_us() - start) < (uint64_t)ms * 1000ULL) {
        /* busy loop */
    }
}

/*==================================================================
 *  中断服务函数
 *==================================================================*/

/**
 * @brief  定时器溢出处理（在 TMR8_OVF_TMR13_IRQHandler 中调用）
 *
 * 职责：
 *  - 检查并清除 TMR13 溢出标志
 *  - 递增 64-bit 软件溢出计数器
 *
 * 注意：
 *  - 本函数不得清除 TMR8 的标志位（由调用方负责）
 *  - 执行时间 < 1μs，对系统实时性无影响
 */
void time_overflow_handler(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (tmr_flag_get(TIME_TMR, TMR_OVF_FLAG) == SET) {
        tmr_flag_clear(TIME_TMR, TMR_OVF_FLAG);
        time_overflow_cnt++;
    }
    __set_PRIMASK(primask);
}
