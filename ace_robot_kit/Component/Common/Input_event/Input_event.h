#ifndef __INPUT_EVENT_H
#define __INPUT_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 边沿触发回调(无参数) */
typedef void (*InputEvent_EdgeHandler)(void);
/* 持续触发回调(参数为保持时长, 单位ms) */
typedef void (*InputEvent_ActiveHandler)(uint32_t duration_ms);

typedef struct {
    uint8_t last_state;                           /* 上一次输入状态 0/1 */
    uint32_t last_change_ms;                      /* 状态变化时刻(ms) */

    InputEvent_EdgeHandler rising_handler;        /* 上升沿回调 */
    InputEvent_EdgeHandler falling_handler;       /* 下降沿回调 */
    InputEvent_ActiveHandler active_high_handler; /* 高电平持续回调 */
    InputEvent_ActiveHandler active_low_handler;  /* 低电平持续回调 */

    InputEvent_EdgeHandler delayed_handler;       /* 延时触发回调 */
    uint32_t delay_ms;                            /* 延时时间(ms) */
    uint8_t is_rising_trigger;                    /* 1=上升沿启动延时, 0=下降沿启动延时 */
    uint8_t trigger_pending;                      /* 延时触发挂起标志 */
    uint32_t trigger_deadline_ms;                 /* 延时触发截止时刻(ms) */
} InputEvent_t;

void InputEvent_Init(InputEvent_t* event);
uint8_t InputEvent_GetState(InputEvent_t* event);
// 设置上升沿触发 函数形式 void (*InputEvent_EdgeHandler)(void);
void InputEvent_SetRising(InputEvent_t* event, InputEvent_EdgeHandler handler);
// 设置下降沿触发 函数形式 void (*InputEvent_EdgeHandler)(void);
void InputEvent_SetFalling(InputEvent_t* event, InputEvent_EdgeHandler handler);
// 设置边沿触发 函数形式 void (*InputEvent_EdgeHandler)(void);
void InputEvent_SetEdge(InputEvent_t* event, InputEvent_EdgeHandler rising_handler, InputEvent_EdgeHandler falling_handler);
// 设置持续高电平触发 函数形式 void (*InputEvent_ActiveHandler)(uint32_t duration_ms);
void InputEvent_SetActiveHigh(InputEvent_t* event, InputEvent_ActiveHandler handler);
// 设置持续低电平电平触发 函数形式 void (*InputEvent_ActiveHandler)(uint32_t duration_ms);
void InputEvent_SetActiveLow(InputEvent_t* event, InputEvent_ActiveHandler handler);
// 设置持续高低电平都触发 函数形式 void (*InputEvent_ActiveHandler)(uint32_t duration_ms);
void InputEvent_SetActiveEdge(InputEvent_t* event, InputEvent_ActiveHandler high_handler, InputEvent_ActiveHandler low_handler);
// is_rising_trigger 置1为上升沿 置0为下降沿 函数形式 void (*InputEvent_EdgeHandler)(void);
void InputEvent_SetDelayTriggered(InputEvent_t* event, InputEvent_EdgeHandler delay_handler, uint32_t delay_ms, uint8_t is_rising_trigger);
// 更新电平状态
void InputEvent_Update(InputEvent_t* event, uint8_t state);

#ifdef __cplusplus
}
#endif

#endif
