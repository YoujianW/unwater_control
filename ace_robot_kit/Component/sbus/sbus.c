/***************************************************************
 * sbus.c
 * SBUS 25 字节组帧与 11 位原始通道解包
 * - 所有串口操作通过 BSP_USART_* 接口
 * - 中断入口与硬件配置由 APP 和 Workbench 管理
 ***************************************************************/
#include "sbus.h"
#include <string.h>

#define SBUS_FRAME_HEADER        0x0FU
#define SBUS_FRAME_FOOTER        0x00U
#define SBUS_FLAG_CHANNEL17      0x01U
#define SBUS_FLAG_CHANNEL18      0x02U
#define SBUS_FLAG_FRAME_LOST     0x04U
#define SBUS_FLAG_FAILSAFE       0x08U

/* ============================ 私有辅助函数 ============================ */

static void sbus_parse_data(SBUS_Handle_t *handle)
{
    uint32_t bits = 0;
    uint8_t bit_count = 0;
    uint8_t index = 1;
    uint8_t flags = handle->rx_buffer[23];

    /* 22 字节负载按低位在前连续存放 16 个 11 位通道。 */
    for (uint8_t ch = 0; ch < SBUS_CHANNEL_COUNT; ch++) {
        while (bit_count < 11U) {
            bits |= (uint32_t)handle->rx_buffer[index++] << bit_count;
            bit_count += 8U;
        }
        handle->data.channel[ch] = (uint16_t)(bits & SBUS_CHANNEL_MAX);
        bits >>= 11U;
        bit_count -= 11U;
    }

    handle->data.channel17 = (flags & SBUS_FLAG_CHANNEL17) != 0U;
    handle->data.channel18 = (flags & SBUS_FLAG_CHANNEL18) != 0U;
    handle->data.frame_lost = (flags & SBUS_FLAG_FRAME_LOST) != 0U;
    handle->data.failsafe = (flags & SBUS_FLAG_FAILSAFE) != 0U;
    handle->data_ready = 1;
}

static void sbus_resync(SBUS_Handle_t *handle)
{
    /* 错帧可能由丢字节引起，保留下一个候选帧头及其后数据。 */
    for (uint8_t i = 1; i < SBUS_FRAME_SIZE; i++) {
        if (handle->rx_buffer[i] == SBUS_FRAME_HEADER) {
            handle->rx_index = (uint8_t)(SBUS_FRAME_SIZE - i);
            memmove(handle->rx_buffer, &handle->rx_buffer[i], handle->rx_index);
            return;
        }
    }
    handle->rx_index = 0;
}

static void sbus_receive_byte(SBUS_Handle_t *handle, uint8_t byte)
{
    if (handle->rx_index >= SBUS_FRAME_SIZE) {
        handle->rx_index = 0;
    }
    if (handle->rx_index == 0 && byte != SBUS_FRAME_HEADER) return;

    /* 负载也可能包含 0x0F，收满一帧之前不能据此重新开始。 */
    handle->rx_buffer[handle->rx_index++] = byte;
    if (handle->rx_index < SBUS_FRAME_SIZE) return;

    /* 标准 SBUS 无校验和，检查帧头、帧尾及标志字节的保留位。 */
    if (handle->rx_buffer[0] == SBUS_FRAME_HEADER &&
        handle->rx_buffer[SBUS_FRAME_SIZE - 1U] == SBUS_FRAME_FOOTER &&
        (handle->rx_buffer[23] & 0xF0U) == 0U) {
        sbus_parse_data(handle);
        handle->rx_index = 0;
        return;
    }

    sbus_resync(handle);
}

/* ============================ API实现 ============================ */

void SBUS_Init(SBUS_Handle_t *handle, void *usart)
{
    if (handle == NULL) return;

    memset(handle, 0, sizeof(SBUS_Handle_t));
    /* 尚未收到有效帧时，不将初始零值视作正常遥控数据。 */
    handle->data.failsafe = 1;
    BSP_USART_Init(&handle->usart, usart);
}

void SBUS_IRQHandler(SBUS_Handle_t *handle, uint8_t byte)
{
    if (handle == NULL || handle->usart.usart_handle == NULL) return;

    sbus_receive_byte(handle, byte);
}

uint8_t SBUS_GetData(SBUS_Handle_t *handle, SBUS_Data_t *data)
{
    if (handle == NULL || data == NULL || !handle->data_ready) return 0;

    *data = handle->data;
    handle->data_ready = 0;
    return 1;
}
