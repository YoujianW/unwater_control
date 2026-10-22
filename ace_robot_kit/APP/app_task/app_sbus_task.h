#ifndef APP_SBUS_TASK_H
#define APP_SBUS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void APP_SBUS_IRQHandler(uint8_t byte);
void sbus_Task_func(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
