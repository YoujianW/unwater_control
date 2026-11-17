/***************************************************************
 * lv_conf.h
 * LVGL 8.3 配置，使用系统时间组件和独立内存池
 ***************************************************************/
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH                 16
#define LV_COLOR_16_SWAP               0

#define LV_MEM_CUSTOM                  0
#define LV_MEM_SIZE                    (48U * 1024U)

#define LV_TICK_CUSTOM                 1
#define LV_TICK_CUSTOM_INCLUDE         "time_core.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR   (time_get_ms())

#endif
