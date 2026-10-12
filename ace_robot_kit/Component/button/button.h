#ifndef BUTTON_H
#define BUTTON_H

#include "multi_button.h"

#ifdef __cplusplus
extern "C" {
#endif

void BUTTON_Init(BtnCallback key1_press_down, BtnCallback key1_press_up,
                 BtnCallback key2_press_down, BtnCallback key2_press_up);
void BUTTON_Ticks(void);

#ifdef __cplusplus
}
#endif

#endif
