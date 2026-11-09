#include "app_low_pwc_task.h"
#include "app_task.h"
#include "bsp_pwc.h"

#define LOW_POWER_STANDBY_DELAY_S   (8UL * 60UL * 60UL)

void low_power_Task_func(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    (void)pvParameters;

    BSP_PWC_Init();
    BSP_PWC_ClearStandbyFlag();
    BSP_PWC_ClearWakeupFlag();
    BSP_PWC_WakeupPinEnable(1);

    /* 每次启动运行 8 小时；会进入standby 模式，WakeUp 引脚有上升沿时，standby 被唤醒，程序从头重新开始。 */
    /* 按秒转换为 tick，避免长延时使用 pdMS_TO_TICKS 时发生整数溢出。 */
    vTaskDelayUntil(&last_wake_time, LOW_POWER_STANDBY_DELAY_S * configTICK_RATE_HZ);

    while (1)
    {
        /* 三种低功耗模式的区别（AT32F403A）：
         * Sleep（睡眠）：只暂停 CPU，外设和时钟继续运行，变量和寄存器保留；
         *               中断或事件唤醒后，程序从睡眠入口继续执行。
         * Deepsleep（深睡眠）：CPU 和大部分外设时钟停止，变量和寄存器保留；
         *                     可由 EXINT（外部中断线）、RTC 闹钟等唤醒，继续执行；
         *                     唤醒后先使用内部 HICK 时钟，需恢复原系统时钟再运行原业务。
         * Standby（待机）：关闭 CPU 供电、大部分外设和主时钟，功耗最低；
         *                 普通 SRAM 中的变量和大部分寄存器丢失，RTC 和备份寄存器保留；
         *                 PA0/WKUP 上升沿、RTC 闹钟或复位等可退出，程序从头重新启动。
         * Sleep模式可以直接烧录，Deepsleep和standby模式需要先唤醒再烧录，或使用“复位下连接”烧录。
         */
        BSP_PWC_EnterStandby();
    }
}
