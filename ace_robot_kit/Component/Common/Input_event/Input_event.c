#include "Input_event.h"
#include "FreeRTOS.h"
#include "task.h"

void InputEvent_Init(InputEvent_t* event)
{
    if (event == NULL)
    {
        return;
    }

    event->last_state = 0;
    event->last_change_ms = 0;
    event->rising_handler = NULL;
    event->falling_handler = NULL;
    event->active_high_handler = NULL;
    event->active_low_handler = NULL;
    event->delayed_handler = NULL;
    event->delay_ms = 0;
    event->is_rising_trigger = 1;
    event->trigger_pending = 0;
    event->trigger_deadline_ms = 0;
}

uint8_t InputEvent_GetState(InputEvent_t* event)
{
    return event->last_state;
}

void InputEvent_SetRising(InputEvent_t* event, InputEvent_EdgeHandler handler)
{
    event->rising_handler = handler;
}

void InputEvent_SetFalling(InputEvent_t* event, InputEvent_EdgeHandler handler)
{
    event->falling_handler = handler;
}

void InputEvent_SetEdge(InputEvent_t* event, InputEvent_EdgeHandler rising_handler, InputEvent_EdgeHandler falling_handler)
{
    event->rising_handler = rising_handler;
    event->falling_handler = falling_handler;
}

void InputEvent_SetActiveHigh(InputEvent_t* event, InputEvent_ActiveHandler handler)
{
    event->active_high_handler = handler;
}

void InputEvent_SetActiveLow(InputEvent_t* event, InputEvent_ActiveHandler handler)
{
    event->active_low_handler = handler;
}

void InputEvent_SetActiveEdge(InputEvent_t* event, InputEvent_ActiveHandler high_handler, InputEvent_ActiveHandler low_handler)
{
    event->active_high_handler = high_handler;
    event->active_low_handler = low_handler;
}

void InputEvent_SetDelayTriggered(InputEvent_t* event, InputEvent_EdgeHandler delay_handler, uint32_t delay_ms, uint8_t is_rising_trigger)
{
    event->delayed_handler = delay_handler;
    event->delay_ms = delay_ms;
    event->is_rising_trigger = is_rising_trigger;
}

void InputEvent_Update(InputEvent_t* event, uint8_t state)
{
    uint32_t now;

    if (event == NULL)
    {
        return;
    }

    now = (uint32_t)xTaskGetTickCount();

    if (state != event->last_state)
    {
        /* 边沿触发 */
        if (state && event->rising_handler)
        {
            event->rising_handler();
        }
        else if (!state && event->falling_handler)
        {
            event->falling_handler();
        }

        /* 边沿到来时启动延时触发 */
        if (event->delayed_handler)
        {
            if ((state && event->is_rising_trigger) || (!state && !event->is_rising_trigger))
            {
                event->trigger_pending = 1;
                event->trigger_deadline_ms = now + event->delay_ms;
            }
        }

        event->last_state = state;
        event->last_change_ms = now;
    }

    /* 延时触发到期 */
    if (event->trigger_pending && event->delayed_handler && ((int32_t)(now - event->trigger_deadline_ms) >= 0))
    {
        event->trigger_pending = 0;
        event->delayed_handler();
    }

    /* 持续触发(保持) */
    if (state && event->active_high_handler)
    {
        event->active_high_handler(now - event->last_change_ms);
    }
    if (!state && event->active_low_handler)
    {
        event->active_low_handler(now - event->last_change_ms);
    }
}
