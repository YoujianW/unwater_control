#ifndef _CIRCLE_QUEUE_H
#define _CIRCLE_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#define USART1_TX_BUFFER_SIZE 8U

#include <string.h>
#include "stdint.h"

uint16_t Que_GetRxSize(void);
uint8_t queue_push(uint8_t _data);
uint8_t Que_ReadByte(uint8_t* pIByte);
uint16_t Que_ReadMultiBytes(uint8_t* pIByte, uint16_t len);
void queue_reset(void);

#ifdef __cplusplus
}
#endif

#endif
