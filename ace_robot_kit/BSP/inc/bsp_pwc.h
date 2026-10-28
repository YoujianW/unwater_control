/***************************************************************
 * bsp_pwc.h
 * PWC 低功耗通用接口（平台无关）
 ***************************************************************/
#ifndef BSP_PWC_H
#define BSP_PWC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 低功耗等待方式 */
typedef enum {
    BSP_PWC_ENTRY_WFI = 0,       /* 等待中断 */
    BSP_PWC_ENTRY_WFE            /* 等待事件 */
} BSP_PWC_Entry_t;

/* 深睡眠时内部稳压器的工作方式 */
typedef enum {
    BSP_PWC_REGULATOR_NORMAL = 0,
    BSP_PWC_REGULATOR_LOW_POWER
} BSP_PWC_Regulator_t;

/**
 * @brief 使能 PWC 外设时钟，不复位外设，不清除状态标志
 * @note 操作 PWC 寄存器的接口会自动调用本函数，可重复调用。
 *       唤醒源、GPIO、RTC 和中断配置由调用方完成。
 */
void BSP_PWC_Init(void);

/**
 * @brief 进入睡眠模式，唤醒后从本函数返回
 * @param entry 等待中断 WFI 或等待事件 WFE
 * @return 1：完成一次等待；0：参数无效或调用上下文不允许
 * @note 仅在主循环或任务上下文、未屏蔽中断时调用，不支持 SLEEPONEXIT。
 *       调用前须配置并使能对应的唤醒中断或事件；已有待处理请求时可能立即返回。
 *       保留 SRAM 和外设配置，不停止 SysTick，不修改系统时钟。
 *       RTOS 应在确认没有就绪业务任务后进入睡眠，BSP 不代替调度器判断。
 * @example
 *       BSP_PWC_EnterSleep(BSP_PWC_ENTRY_WFI);
 */
uint8_t BSP_PWC_EnterSleep(BSP_PWC_Entry_t entry);

/**
 * @brief 进入深睡眠模式，保留 SRAM 和寄存器
 * @param entry     等待中断 WFI 或等待事件 WFE
 * @param regulator 内部稳压器使用正常模式或低功耗模式
 * @return 1：完成一次等待；0：参数无效或调用上下文不允许
 * @note 调用上下文要求与 BSP_PWC_EnterSleep 相同。
 *       调用方须提前配置 EXINT/RTC 等有效唤醒源，完成外设事务，
 *       并处理 SysTick、看门狗、RTOS 调度及睡眠期间的时间补偿。
 *       AT32 唤醒后使用 HICK，接口只更新系统频率变量，不恢复原系统时钟；
 *       调用方须恢复系统时钟及相关时间基准后再恢复业务。
 *       唤醒中断可能在函数返回前执行，中断处理须允许临时使用 HICK。
 */
uint8_t BSP_PWC_EnterDeepSleep(BSP_PWC_Entry_t entry,
    BSP_PWC_Regulator_t regulator);

/**
 * @brief 进入待机模式，有效调用后不返回，唤醒后重新启动
 * @return 0：调用上下文不允许，或平台未能进入待机
 * @note 调用上下文要求与 BSP_PWC_EnterSleep 相同。
 *       调用前须配置 WKUP 或 RTC 闹钟等有效唤醒源，并完成数据保存及外设事务。
 *       普通 SRAM 和大部分寄存器不保留；本接口不自动使能 WKUP 或配置 RTC。
 *       底层进入待机时会清除旧唤醒标志，启动后应先读取待机标志再清除。
 */
uint8_t BSP_PWC_EnterStandby(void);

/**
 * @brief 使能或关闭专用待机唤醒引脚
 * @param enable 0：关闭；非 0：使能
 * @note AT32F403A/407 对应 WKUP1（PA0），以引脚上升沿唤醒待机。
 *       引脚初始电平和外部电路由调用方保证，不配置普通 GPIO 的 EXINT。
 */
void BSP_PWC_WakeupPinEnable(uint8_t enable);

/**
 * @brief 读取 PWC 硬件唤醒标志，1：置位；0：未置位
 * @note 此标志不代表所有 Sleep/Deepsleep 中断的唤醒原因。
 */
uint8_t BSP_PWC_GetWakeupFlag(void);

/**
 * @brief 读取待机标志，1：置位；0：未置位，不自动清除
 * @note 启动时读取并清除，避免把上次未清除的标志当作本次复位原因。
 */
uint8_t BSP_PWC_GetStandbyFlag(void);

/**
 * @brief 清除 PWC 硬件唤醒标志
 */
void BSP_PWC_ClearWakeupFlag(void);

/**
 * @brief 清除待机标志
 */
void BSP_PWC_ClearStandbyFlag(void);

#ifdef __cplusplus
}
#endif

#endif
