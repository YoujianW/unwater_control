/***************************************************************
 * sbus.h
 * HT-10A SBUS 接收组件，使用 BSP USART 接口
 ***************************************************************/
#ifndef SBUS_H
#define SBUS_H

#include "bsp_usart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SBUS_FRAME_SIZE          25U
#define SBUS_CHANNEL_COUNT       16U
#define SBUS_HT10A_CHANNEL_COUNT 10U
#define SBUS_CHANNEL_MAX         0x07FFU

typedef struct {
    uint16_t channel[SBUS_CHANNEL_COUNT]; /* CH1 ~ CH16，原始值 0 ~ 2047 */
    uint8_t channel17;                    /* 数字通道，0 或 1 */
    uint8_t channel18;
    uint8_t frame_lost;                   /* 接收机报告丢帧 */
    uint8_t failsafe;                     /* 接收机进入失控保护 */
} SBUS_Data_t;

typedef struct {
    BSP_USART_Handle_t usart;             /* 组件内部持有的 USART 句柄 */
    uint8_t rx_buffer[SBUS_FRAME_SIZE];
    uint8_t rx_index;
    volatile uint8_t data_ready;          /* 有一帧尚未读取的新数据 */
    SBUS_Data_t data;                     /* 最近一帧解包结果 */
} SBUS_Handle_t;

/* HT-10A 使用前 10 个通道，数组下标 0 对应 CH1。
 * 默认映射：CH1/2 右摇杆左右/上下，CH3/4 左摇杆上下/左右，
 * CH5 ~ CH8 为 SWA ~ SWD，CH9/10 为 VRA/VRB；以遥控器实际映射为准。
 * 保留接收原始值，不归一化、不设定摇杆端点，不改变通道顺序。
 */

/**
 * @brief 初始化内部 BSP USART 句柄并清空接收状态，不修改硬件配置
 * @param usart USART 外设基地址，由 APP 选择
 * @note 硬件须配置为 100000、8 位有效数据、偶校验、2 停止位。
 *       AT32 对应 9 位字长（包含校验位）；标准 SBUS 的反相信号
 *       须在进入 USART RX 前由硬件适配为普通 UART 电平逻辑。
 * @example
 *       static SBUS_Handle_t sbus;
 *       SBUS_Init(&sbus, USART2);
 *       USART2 等平台符号由 APP 自行包含 wk_usart.h 获得。
 *       在使接收中断调用组件之前完成初始化。
 */
void SBUS_Init(SBUS_Handle_t *handle, void *usart);

/**
 * @brief 串口接收中断入口，内部完成组帧和解包
 * @note 仅供对应 USART 中断转交已读取的字节，任务通过 SBUS_GetData 获取结果。
 */
void SBUS_IRQHandler(SBUS_Handle_t *handle, uint8_t byte);

/**
 * @brief 获取最新完整帧的通道和状态，收到新数据返回 1，否则返回 0
 * @note 新帧覆盖旧帧，不排队；无新数据时保留输出原值。
 *       返回 1 不代表遥控链路正常，须同时检查帧内状态。
 *       任务调用时须用临界区保护整个调用，避免与接收中断冲突。
 */
uint8_t SBUS_GetData(SBUS_Handle_t *handle, SBUS_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif
