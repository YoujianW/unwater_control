/***************************************************************
 * alg_filter.c
 * 数字滤波算法库实现（Algorithm Layer）
 *
 * 滤波器选型指南：
 *   - 一阶低通：缓慢变化信号首选（温度、电池电压、环境光）
 *   - 滑动均值：周期性干扰首选（PWM 纹波、50Hz 工频、ADC 量化噪声）
 *   - 中值滤波：偶发脉冲干扰首选（ESD、换向尖峰、接触噪声）
 *   - 级联策略：中值→均值→低通（去尖峰+平周期+柔跟踪，电机电流标配）
 ***************************************************************/
#include "Filter.h"
#include <string.h>
#include <stdlib.h>

/*================================================================
 * 内部工具：插入排序
 *
 * 为什么用插入排序而不是 qsort？
 *   - 窗口 N≤7 时，插入排序 O(N²) 常数因子极小，实测比 qsort 快 2-3×
 *   - 数据几乎有序（环形缓冲区相邻值接近），插入排序接近 O(N)
 *================================================================*/
static void insert_sort(float *arr, uint16_t n)
{
    for (uint16_t i = 1; i < n; i++) {
        float key = arr[i];
        int16_t j = (int16_t)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/*================================================================
 * 1. 一阶低通滤波器（EMA / IIR）
 *
 *    差分方程：y[n] = α·x[n] + (1-α)·y[n-1]
 *
 *                         dt
 *    α 计算公式：α = ─────────
 *                      τ + dt
 *
 *    dt = 调用周期（你的任务是 20ms）
 *    τ  = 时间常数（输出达到终值 63.2% 所需时间）
 *
 *    阶跃响应时间表：
 *       1τ → 63.2%     3τ → 95.0%     5τ → 99.3%
 *
 *    工程调参表（dt=20ms）：
 *       α=0.167  → τ=0.1s  → 适合快速保护（过流）
 *       α=0.063  → τ=0.3s  → 适合电机电流平滑显示
 *       α=0.038  → τ=0.5s  → 适合 NTC 温度
 *       α=0.013  → τ=1.5s  → 适合环境温度、电池电压
 *================================================================*/

void Filter_LowPass1_Init(Filter_LowPass1_t *f, float alpha)
{
    if (f == NULL) return;

    /* 限幅：α 必须在 (0, 1] */
    if (alpha <= 0.0f)  alpha = 0.001f;
    if (alpha > 1.0f)   alpha = 1.0f;

    f->alpha       = alpha;
    f->output      = 0.0f;
    f->initialized = 0;
}

float Filter_LowPass1_Update(Filter_LowPass1_t *f, float input)
{
    if (f == NULL) return input;

    /*
     * 首次调用特殊处理：
     * 不经过递推公式，直接用 input 初始化 output。
     * 否则会从 0 开始爬升，产生虚假的"缓慢上升"过渡过程。
     * 
     * 例：真实温度 60°C，若从 0 开始递推，需 3τ 才到 57°C，
     *     期间上位机会看到持续 1.5s 的假升温。
     */
    if (f->initialized == 0) {
        f->output      = input;
        f->initialized = 1;
        return input;
    }

    /*
     * IIR 递推：
     *   新输出 = α × 新输入 + (1-α) × 旧输出
     *
     * 直观理解：
     *   每次更新，输出向输入方向移动 α 比例的距离。
     *   α=1    → 输出=输入（无滤波）
     *   α=0.5  → 每次折半移动（强滤波但慢）
     *   α=0.01 → 几乎不动（极强滤波）
     */
    f->output = f->alpha * input + (1.0f - f->alpha) * f->output;
    return f->output;
}

void Filter_LowPass1_Reset(Filter_LowPass1_t *f, float value)
{
    if (f == NULL) return;
    f->output      = value;
    f->initialized = 1;
}

/*================================================================
 * 2. 滑动均值滤波器（SMA，O(1) 增量算法）
 *
 *    数学模型：y[n] = (x[n] + x[n-1] + ... + x[n-N+1]) / N
 *
 *    频率响应：
 *      - 对 f = k·(fs/N) 有零点（完全抑制），k=1,2,3...
 *      - 在 fs=50Hz, N=10 时，零点在 5Hz, 10Hz, 15Hz...
 *      - 对 PWM 20kHz 纹波本身不能滤（远高于奈奎斯特），
 *        但能抑制 PWM 在 ADC 采样中的混叠低频分量
 *
 *    群延迟 = N/2 个采样周期（不可消除的物理延迟）
 *      N=10, dt=20ms → 延迟 100ms
 *
 *    算法复杂度：O(1)，不随窗口大小增长
 *================================================================*/

void Filter_MovingAvg_Init(Filter_MovingAvg_t *f, float *buffer, uint16_t size)
{
    if (f == NULL || buffer == NULL || size == 0) return;

    f->buffer = buffer;
    f->size   = size;
    f->index  = 0;
    f->count  = 0;
    f->sum    = 0.0f;

    memset(buffer, 0, size * sizeof(float));
}

float Filter_MovingAvg_Update(Filter_MovingAvg_t *f, float input)
{
    if (f == NULL || f->buffer == NULL || f->size == 0) return input;

    /*
     * 阶段 1：窗口填充期（count < size）
     *
     * 此时样本数不足，用 count 而非 size 做除数。
     * 如果直接用 size 做除数，初始的零值会把输出严重拉低。
     *
     * 例：size=8, 已写入 3 个值 [9.2, 9.5, 9.0]
     *     正确：(9.2+9.5+9.0)/3 = 9.23 ✓
     *     错误：(9.2+9.5+9.0+0+0+0+0+0)/8 = 3.46 ✗
     */
    if (f->count < f->size) {
        f->buffer[f->index] = input;
        f->sum   += input;
        f->count++;
        f->index = (f->index + 1) % f->size;
        return f->sum / (float)f->count;
    }

    /*
     * 阶段 2：窗口满载期（count == size）
     *
     * O(1) 增量更新：
     *   sum_new = sum_old - oldest_value + new_value
     *
     * f->index 始终指向"即将被覆盖的最旧数据"。
     *
     * 环形缓冲区演进示例（size=3）：
     *   初始：[x0, x1, x2], index=0, sum=x0+x1+x2
     *   输入 x3：
     *     sum = sum - buffer[0] + x3 = x1+x2+x3
     *     buffer[0] = x3
     *     index = 1（现在 buffer[1]=x1 是最旧的）
     *   输入 x4：
     *     sum = sum - buffer[1] + x4 = x2+x3+x4
     *     buffer[1] = x4
     *     index = 2
     *   ...
     */
    f->sum -= f->buffer[f->index];
    f->buffer[f->index] = input;
    f->sum += input;
    f->index = (f->index + 1) % f->size;

    return f->sum / (float)f->size;
}

void Filter_MovingAvg_Reset(Filter_MovingAvg_t *f)
{
    if (f == NULL) return;
    f->index = 0;
    f->count = 0;
    f->sum   = 0.0f;
    if (f->buffer != NULL) {
        memset(f->buffer, 0, f->size * sizeof(float));
    }
}

/*================================================================
 * 3. 中值滤波器（Median Filter）
 *
 *    对窗口内样本排序，取中间值。
 *
 *    核心优势：
 *      - 对孤立脉冲 100% 免疫（只要脉冲宽度 < 窗口的一半）
 *      - 保持阶跃边沿不变形（这点优于所有线性滤波器）
 *
 *    核心劣势：
 *      - 对高斯白噪声的抑制能力弱于等窗口的均值滤波器
 *      - 需要排序，但 N≤7 时影响可忽略
 *
 *    为什么放在第一级？
 *      换向尖峰是单点脉冲 → 中值直接丢弃
 *      PWM 纹波是连续周期 → 中值无法滤除 → 交给后级的均值+低通
 *
 *    memcpy 正确性说明：
 *      环形缓冲区在 fill 阶段后，所有有效值在 [0, size) 区间内。
 *      中值滤波只关心"值集合"，不关心时间顺序。
 *      因此 memcpy(buffer, count) 拷贝的 count 个元素就是完整的集合。
 *================================================================*/

void Filter_Median_Init(Filter_Median_t *f, float *buffer, uint16_t size)
{
    if (f == NULL || buffer == NULL || size == 0) return;

    f->buffer = buffer;
    f->size   = size;
    f->index  = 0;
    f->count  = 0;

    memset(buffer, 0, size * sizeof(float));
}

float Filter_Median_Update(Filter_Median_t *f, float input)
{
    if (f == NULL || f->buffer == NULL || f->size == 0) return input;

    /* ---- 环形入队 ---- */
    f->buffer[f->index] = input;
    f->index = (f->index + 1) % f->size;
    if (f->count < f->size) f->count++;

    /* ---- 拷贝到栈上排序（不破坏环形 buffer 结构）---- */
    /*
     * sorted[] 放在栈上：16×4B = 64B，对嵌入式栈压力可忽略。
     * 如果你的编译器栈紧张（<1KB），可改为 static _Thread_local，
     * 但当前设计保持简单。
     */
    float sorted[16];
    uint16_t n = f->count;          /* 实际有效样本数（≤ size） */

    memcpy(sorted, f->buffer, n * sizeof(float));
    insert_sort(sorted, n);

    /* ---- 取中值 ---- */
    /*
     * 奇数窗口：sorted[n/2] 即正中间元素
     *   例 n=5: indices [0,1,2,3,4] → sorted[2] ✓
     * 偶数窗口：两个中间值的平均
     *   例 n=4: indices [0,1,2,3] → (sorted[1]+sorted[2])/2 ✓
     *
     * 工程建议：始终用奇数（3/5/7），避免偶数平均引入的 0.5LSB 抖动
     */
    if (n % 2 == 1) {
        return sorted[n / 2];
    } else {
        return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0f;
    }
}

void Filter_Median_Reset(Filter_Median_t *f)
{
    if (f == NULL) return;
    f->index = 0;
    f->count = 0;
    if (f->buffer != NULL) {
        memset(f->buffer, 0, f->size * sizeof(float));
    }
}
