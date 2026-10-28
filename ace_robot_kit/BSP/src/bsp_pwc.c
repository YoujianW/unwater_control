/***************************************************************
 * bsp_pwc.c
 * PWC 低功耗 BSP 层实现
 ***************************************************************/
#include "bsp_pwc.h"
#include "bsp_pwc_platform.h"

/* 参数无效时不操作硬件，避免误入低功耗模式。 */
static uint8_t BSP_PWC_EntryValid(BSP_PWC_Entry_t entry)
{
    return entry == BSP_PWC_ENTRY_WFI || entry == BSP_PWC_ENTRY_WFE;
}

void BSP_PWC_Init(void)
{
    BSP_PWC_CLOCK_ENABLE();
}

uint8_t BSP_PWC_EnterSleep(BSP_PWC_Entry_t entry)
{
    if (!BSP_PWC_EntryValid(entry) || !BSP_PWC_CAN_ENTER()) return 0;

    BSP_PWC_DATA_SYNC();
    if (entry == BSP_PWC_ENTRY_WFI) {
        BSP_PWC_SLEEP_WFI();
    } else {
        BSP_PWC_SLEEP_WFE();
    }
    BSP_PWC_INSTRUCTION_SYNC();
    return 1;
}

uint8_t BSP_PWC_EnterDeepSleep(BSP_PWC_Entry_t entry,
    BSP_PWC_Regulator_t regulator)
{
    if (!BSP_PWC_EntryValid(entry) || !BSP_PWC_CAN_ENTER()) return 0;
    if (regulator != BSP_PWC_REGULATOR_NORMAL &&
        regulator != BSP_PWC_REGULATOR_LOW_POWER) return 0;

    BSP_PWC_Init();

    /* 明确选择深睡眠，避免残留的待机选择位影响本次进入。 */
    BSP_PWC_SELECT_DEEP_SLEEP();
    if (regulator == BSP_PWC_REGULATOR_LOW_POWER) {
        BSP_PWC_REGULATOR_SET_LOW_POWER();
    } else {
        BSP_PWC_REGULATOR_SET_NORMAL();
    }

    BSP_PWC_DATA_SYNC();
    if (entry == BSP_PWC_ENTRY_WFI) {
        BSP_PWC_DEEP_SLEEP_WFI();
    } else {
        BSP_PWC_DEEP_SLEEP_WFE();
    }
    BSP_PWC_INSTRUCTION_SYNC();

    /* 反映唤醒后的实际频率，原系统时钟和时间基准由调用方恢复。 */
    BSP_PWC_UPDATE_CORE_CLOCK();
    return 1;
}

uint8_t BSP_PWC_EnterStandby(void)
{
    if (!BSP_PWC_CAN_ENTER()) return 0;

    BSP_PWC_Init();
    BSP_PWC_DATA_SYNC();
    BSP_PWC_STANDBY_ENTER();
    return 0;
}

void BSP_PWC_WakeupPinEnable(uint8_t enable)
{
    BSP_PWC_Init();
    BSP_PWC_WAKEUP_PIN_ENABLE(enable);
}

uint8_t BSP_PWC_GetWakeupFlag(void)
{
    BSP_PWC_Init();
    return BSP_PWC_WAKEUP_FLAG_GET();
}

uint8_t BSP_PWC_GetStandbyFlag(void)
{
    BSP_PWC_Init();
    return BSP_PWC_STANDBY_FLAG_GET();
}

void BSP_PWC_ClearWakeupFlag(void)
{
    BSP_PWC_Init();
    BSP_PWC_WAKEUP_FLAG_CLEAR();
}

void BSP_PWC_ClearStandbyFlag(void)
{
    BSP_PWC_Init();
    BSP_PWC_STANDBY_FLAG_CLEAR();
}
