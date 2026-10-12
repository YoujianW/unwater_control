#ifndef ACE_APP_CONFIG_H
#define ACE_APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "time_core.h"

/* 在芯片外设全部初始化完成后调用此函数。启动 FreeRTOS。 */
void app_config_init(void);

// 组件相关
void APP_TMR13_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
