# 使用说明手册

```c
#include "Input_event.h"
#include "Key.h"   
#include "task.h"

static InputEvent_t btn1;
static uint8_t btn1_init = 0;

/* 按下瞬间(下降沿 1->0), 只触发一次 */
static void Btn1_OnPress(void)
{
    /* 例如: 立即触发一个动作 */
}

/* 松开瞬间(上升沿 0->1), 只触发一次 */
static void Btn1_OnRelease(void)
{
    /* 例如: 记录松手 */
}

/* 按住期间(state=0 保持), 每次循环都会调用, ms = 已按住时长 */
static void Btn1_OnHold(uint32_t ms)
{
    if (ms >= 1000)          /* 按住超过 1 秒 */
    {
        /* 长按逻辑 */
    }
}

/* 按下后延时 500ms 触发一次 */
static void Btn1_OnDelay(void)
{
    /* 延时确认动作 */
}

void Btn_task(void)
{
    uint8_t state;

    if (!btn1_init)
    {
        InputEvent_Init(&btn1);

        InputEvent_SetFalling(&btn1, Btn1_OnPress);              /* 按下瞬间 */
        InputEvent_SetRising(&btn1, Btn1_OnRelease);             /* 松开瞬间 */
        InputEvent_SetActiveLow(&btn1, Btn1_OnHold);             /* 按住保持 */
        InputEvent_SetDelayTriggered(&btn1, Btn1_OnDelay, 500, 0); /* 下降沿(按下)后延时500ms触发 */

        btn1_init = 1;
    }

    /* 读按键: 按下=0, 松开=1 */
    state = (uint8_t)gpio_input_data_bit_read(KEY_1_GPIO_PORT, KEY_1_PIN);
    InputEvent_Update(&btn1, state);

    vTaskDelay(pdMS_TO_TICKS(10));   /* 每 10ms 轮询一次 */
}
```
