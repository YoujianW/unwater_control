/***************************************************************
 * Filter.h
 * 数字滤波算法库（Algorithm Layer）
 * 
 * 设计原则：
 *  - 纯软件算法，不依赖任何硬件平台
 *  - 句柄式管理，支持多实例并发
 *  - 零动态内存分配，buffer 由调用方提供
 *  - 所有函数可重入（reentrant）
***************************************************************/
#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

/*================================================================
 * 1. 一阶低通滤波器（EMA / IIR）
 *
 *    公式：y[n] = α·x[n] + (1-α)·y[n-1]
 *
 *    α 的含义：
 *       α = dt / (τ + dt)
 *       其中 dt = 采样周期，τ = 时间常数
 *
 *    典型 α 参考（dt=20ms）：
 *       τ=0.1s → α≈0.167   (快速响应)
 *       τ=0.5s → α≈0.038   (中等平滑)
 *       τ=1.0s → α≈0.020   (强平滑)
 *================================================================*/

typedef struct {
    float   alpha;          /* 滤波系数 (0 < α ≤ 1)           */
    float   output;         /* 当前滤波输出                    */
    uint8_t initialized;    /* 首次标志：1=已初始化，0=需首值填充 */
} Filter_LowPass1_t;

/**
 * @brief 初始化一阶低通滤波器
 * @param f     滤波器句柄
 * @param alpha 滤波系数 (0 < α ≤ 1)，越大响应越快平滑越弱
 */
void Filter_LowPass1_Init(Filter_LowPass1_t *f, float alpha);

/**
 * @brief 输入新采样值，返回滤波结果
 * @param f     滤波器句柄
 * @param input 当前原始采样值
 * @return      滤波后的值
 * 
 * 第一次调用时自动用 input 填充历史值，避免阶跃
 */
float Filter_LowPass1_Update(Filter_LowPass1_t *f, float input);

/**
 * @brief 重置滤波器状态
 * @param f     滤波器句柄
 * @param value 重置后的初始输出值
 */
void Filter_LowPass1_Reset(Filter_LowPass1_t *f, float value);


/*================================================================
 * 2. 滑动均值滤波器（SMA / Moving Average）
 *
 *    公式：y[n] = (x[n] + x[n-1] + ... + x[n-N+1]) / N
 *
 *    特点：
 *      - 对周期性干扰抑制效果好
 *      - 窗口越大延时越大，平滑越强
 *      - 需要调用方提供 buffer（N × sizeof(float)）
 *================================================================*/

typedef struct {
    float   *buffer;        /* 环形缓冲区（调用方提供）         */
    uint16_t size;          /* 窗口大小 N                      */
    uint16_t index;         /* 当前写入位置                    */
    uint16_t count;         /* 已累计样本数（≤ size）           */
    float    sum;           /* 当前和（用于 O(1) 更新）        */
} Filter_MovingAvg_t;

/**
 * @brief 初始化滑动均值滤波器
 * @param f      滤波器句柄
 * @param buffer 环形缓冲区（外部提供，大小 = size × sizeof(float)）
 * @param size   窗口大小 N（≥ 1）
 */
void Filter_MovingAvg_Init(Filter_MovingAvg_t *f, float *buffer, uint16_t size);

/**
 * @brief 输入新采样值，返回滑动均值
 * @param f     滤波器句柄
 * @param input 当前原始采样值
 * @return      滑动平均后的值
 */
float Filter_MovingAvg_Update(Filter_MovingAvg_t *f, float input);

/**
 * @brief 重置滤波器状态
 * @param f 滤波器句柄
 */
void Filter_MovingAvg_Reset(Filter_MovingAvg_t *f);


/*================================================================
 * 3. 中值滤波器（Median Filter）
 *
 *    对窗口内样本排序，取中间值。
 *    对脉冲噪声（尖峰毛刺）抑制效果极好。
 *    适合在低通/均值之前做预处理。
 *================================================================*/

typedef struct {
    float   *buffer;        /* 样本缓冲区（调用方提供）         */
    uint16_t size;          /* 窗口大小 N（建议奇数：3/5/7）    */
    uint16_t index;         /* 当前写入位置                    */
    uint16_t count;         /* 已累计样本数                    */
} Filter_Median_t;

/**
 * @brief 初始化中值滤波器
 * @param f      滤波器句柄
 * @param buffer 样本缓冲区（外部提供，大小 = size × sizeof(float)）
 * @param size   窗口大小 N，建议奇数
 * @note 窗口大小不要超过 16，否则内部排序缓冲区会越界。
 */
void Filter_Median_Init(Filter_Median_t *f, float *buffer, uint16_t size);

/**
 * @brief 输入新采样值，返回窗口内中值
 * @param f     滤波器句柄
 * @param input 当前原始采样值
 * @return      窗口内中值
 */
float Filter_Median_Update(Filter_Median_t *f, float input);

/**
 * @brief 重置滤波器状态
 * @param f 滤波器句柄
 */
void Filter_Median_Reset(Filter_Median_t *f);

#endif /* FILTER_H */
