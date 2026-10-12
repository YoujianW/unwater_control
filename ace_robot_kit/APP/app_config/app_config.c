#include "app_config.h"
#include "app_task.h"

void app_config_init(void)
{
    /* 在任务启动之前初始化一些组件。*/
    time_init(); // 系统时间组件

    app_freertos_init();
}

// 组件相关
void APP_TMR13_IRQHandler(void)
{
    time_overflow_handler();
}
