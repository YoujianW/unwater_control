/***************************************************************
 * time_core.h
 * 时间组件 — 平台无关对外接口
***************************************************************/
#ifndef TIME_CORE_H
#define TIME_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 系统运行时间，字段均由同一次定时器采样换算得到。 */
typedef struct
{
	uint64_t total_us;  /* 系统启动后的总运行时间，单位 us */
	uint64_t total_ms;  /* 系统启动后的总运行时间，单位 ms */
	uint32_t days;      /* 已运行天数 */
	uint8_t hours;      /* 当前天内的小时数，0~23 */
	uint8_t minutes;    /* 当前小时内的分钟数，0~59 */
	uint8_t seconds;    /* 当前分钟内的秒数，0~59 */
	uint16_t millis;    /* 当前秒内的毫秒数，0~999 */
} time_runtime_t;

/*==================================================================
 *  初始化
 *==================================================================*/

/**
 * @brief  初始化时间组件
 * @note   调用前必须完成 TMR13 的硬件初始化（wk_tmr13_init）
 *         内部重置硬件计数器、溢出标志和软件累计计数
 *
 * 调用示例：
 *   wk_tmr13_init();        // BSP 层硬件初始化
 *   time_init();            // Component 层软件初始化
 */
void time_init(void);

/*==================================================================
 *  时间获取（ISR 安全）
 *==================================================================*/

/**
 * @brief  获取系统运行微秒数
 * @return 64-bit 微秒时间戳，溢出周期 ≈ 584942 年
 * @note   ISR 安全，采用短临界区取快照；溢出 ISR 需在 1ms 内得到处理
 */
uint64_t time_get_us(void);

/**
 * @brief  获取系统运行毫秒数
 * @return 32-bit 毫秒时间戳，溢出周期 ≈ 49.7 天
 * @note   ISR 安全
 */
uint32_t time_get_ms(void);

/**
 * @brief  获取系统运行时间结构体
 * @param  runtime  输出结构体指针
 * @note   ISR 安全；runtime 不能为 NULL
 */
void time_get_runtime(time_runtime_t *runtime);

/**
 * @brief  获取定时器原始 tick 累计值（含硬件计数器当前值）
 * @return 64-bit tick 计数
 * @note   ISR 安全；1 tick = (prescaler+1) / TimerClock 秒
 */
uint64_t time_get_ticks(void);

/*==================================================================
 *  延迟函数
 *==================================================================*/

/**
 * @brief  微秒级忙等待延迟
 * @param  us  延迟微秒数
 * @note   ISR 安全（忙等待），适用于短延迟（< 数毫秒）
 *         长时间延迟请用 time_delay_ms（会 yield CPU）
 */
void time_delay_us(uint32_t us);

/**
 * @brief  毫秒级延迟
 * @param  ms  延迟毫秒数
 * @note   在 FreeRTOS 任务上下文中调用 vTaskDelay 交出 CPU；
 *         在中断上下文或裸机模式下回退为忙等待
 *
 * 行为：
 *   if (FreeRTOS 调度器已启动 && 在任务上下文)
 *       vTaskDelay(pdMS_TO_TICKS(ms));   // 让出 CPU
 *   else
 *       time_delay_us(ms * 1000);        // 忙等待
 */
void time_delay_ms(uint32_t ms);

/*==================================================================
 *  中断服务函数（必须由 BSP 层调用）
 *==================================================================*/

/**
 * @brief  定时器溢出处理函数
 * @note   在 TMR8_OVF_TMR13_IRQHandler 中调用
 *
 * 典型用法：
 *   void TMR8_OVF_TMR13_IRQHandler(void)
 *   {
 *       // ... TMR8 处理（如有）...
 *       time_overflow_handler();   // TMR13 时间基准
 *   }
 */
void time_overflow_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* TIME_CORE_H */
