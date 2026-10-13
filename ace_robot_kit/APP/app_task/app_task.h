#ifndef __APP_TASK_H
#define __APP_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wk_system.h"
#include "at32f403a_407_int.h"
#include "FreeRTOSConfig.h"
#include "freertos_app.h"
#include "app_adc_task.h"
#include "app_button_task.h"
#include "app_vofa_task.h"


void app_task_create(void);
void app_freertos_init(void);
void Debug_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
