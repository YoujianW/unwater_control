/***************************************************************
 * bsp_software_i2c.h
 * 软件 I2C 主机通用接口（平台无关）
 ***************************************************************/
#ifndef BSP_SOFTWARE_I2C_H
#define BSP_SOFTWARE_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_SOFTWARE_I2C_HALF_PERIOD_US
#define BSP_SOFTWARE_I2C_HALF_PERIOD_US  5U
#endif

#ifndef BSP_SOFTWARE_I2C_TIMEOUT_US
#define BSP_SOFTWARE_I2C_TIMEOUT_US      1000U
#endif

typedef enum {
    BSP_SOFTWARE_I2C_OK = 0,
    BSP_SOFTWARE_I2C_NACK,
    BSP_SOFTWARE_I2C_TIMEOUT,
    BSP_SOFTWARE_I2C_BUS_BUSY,
    BSP_SOFTWARE_I2C_INVALID_PARAM
} BSP_SOFTWARE_I2C_Status_t;

typedef struct {
    void *scl_port;             /* GPIO 端口，由 APP 传入 */
    void *sda_port;
    uint16_t scl_pin;          /* 单个 GPIO 引脚位掩码，不是引脚序号 */
    uint16_t sda_pin;
    uint32_t half_period_us;   /* 半周期最小延时，默认 5 us，实际频率受操作耗时影响 */
    uint32_t timeout_us;       /* 每次等待 SCL 释放的超时时间 */
} BSP_SOFTWARE_I2C_Handle_t;

/**
 * @brief 绑定引脚、加载默认时序，并配置 GPIO 为开漏输出，释放总线
 * @param handle    待初始化的软件 I2C 句柄指针
 * @param scl_port  SCL 所在 GPIO 端口（如 GPIOB）
 * @param scl_pin   SCL 引脚位掩码（如 GPIO_PINS_10）
 * @param sda_port  SDA 所在 GPIO 端口，可与 SCL 使用不同端口
 * @param sda_pin   SDA 引脚位掩码（如 GPIO_PINS_11），不能与 SCL 为同一引脚
 * @return BSP_SOFTWARE_I2C_OK：初始化成功；BSP_SOFTWARE_I2C_INVALID_PARAM：参数无效
 * @note SCL/SDA 均需外部上拉。AT32 适配自动开启对应 GPIO 时钟。
 *       调用前须初始化时间组件及其硬件定时器；移植时修改 platform.h。
 *       同一总线仅支持单主机，多任务使用时由调用方锁住整个事务。
 *       初始化后可修改 half_period_us / timeout_us，两者必须大于 0。
 * @example
 *       BSP_SOFTWARE_I2C_Handle_t bus;
 *       BSP_SOFTWARE_I2C_Init(&bus, GPIOB, GPIO_PINS_10, GPIOB, GPIO_PINS_11);
 *       BSP_SOFTWARE_I2C_ReadReg(&bus, 0x68, 0x75, &id);
 *       GPIOB 等平台符号由 APP 自行包含芯片头文件获得。
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Init(BSP_SOFTWARE_I2C_Handle_t *handle,
    void *scl_port, uint16_t scl_pin, void *sda_port, uint16_t sda_pin);

/* 基础时序接口：0 表示成功，其他值见状态枚举。
 * Start 同时支持重复 START。基础接口失败后由调用方 Stop / 释放总线。
 * SendByte 只发送 8 位，随后需调用 WaitAck；ReadByte 包含第 9 位 ACK/NACK。
 */

/**
 * @brief 产生起始信号 START 或重复起始信号（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @retval BSP_SOFTWARE_I2C_OK             起始信号已产生
 * @retval BSP_SOFTWARE_I2C_TIMEOUT        SCL 释放超时
 * @retval BSP_SOFTWARE_I2C_BUS_BUSY       SDA 被拉低
 * @retval BSP_SOFTWARE_I2C_INVALID_PARAM  句柄或时序参数无效
 * @note 在 SCL 高电平期间将 SDA 从高拉低，结束时 SCL 为低电平
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Start(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 产生停止信号 STOP 并释放总线（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @retval BSP_SOFTWARE_I2C_OK             停止信号已产生且总线空闲
 * @retval BSP_SOFTWARE_I2C_TIMEOUT        SCL 释放超时
 * @retval BSP_SOFTWARE_I2C_BUS_BUSY       释放后 SDA 仍被拉低
 * @retval BSP_SOFTWARE_I2C_INVALID_PARAM  句柄或时序参数无效
 * @note 在 SCL 高电平期间将 SDA 从低释放为高；超时时也会释放主机对两线的驱动
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Stop(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 产生第 9 个时钟并读取从机应答（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @retval BSP_SOFTWARE_I2C_OK             收到从机 ACK
 * @retval BSP_SOFTWARE_I2C_NACK           从机未应答
 * @retval BSP_SOFTWARE_I2C_TIMEOUT        SCL 释放超时
 * @retval BSP_SOFTWARE_I2C_INVALID_PARAM  句柄或时序参数无效
 * @note 在 SendByte 后调用；读取完成后拉低 SCL，不会自动产生 STOP
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_WaitAck(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 主机发送应答 ACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @return BSP_SOFTWARE_I2C_OK：成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 第 9 个时钟期间拉低 SDA，通知从机继续发送；结束时拉低 SCL 并释放 SDA
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_Ack(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 主机发送非应答 NACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @return BSP_SOFTWARE_I2C_OK：成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 第 9 个时钟期间释放 SDA，通知从机结束发送；结束时 SCL 为低电平
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_NAck(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 发送一个字节，高位先行（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @param data    待发送的字节
 * @return BSP_SOFTWARE_I2C_OK：8 位发送完成；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 不包含从机应答时钟，发送完成后需调用 WaitAck 判断从机是否接收
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_SendByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t data);

/**
 * @brief 读取一个字节，高位先行，并发送 ACK 或 NACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @param ack     非 0：读取后发送 ACK；0：读取后发送 NACK
 * @param data    存放读取结果的单字节变量指针
 * @return BSP_SOFTWARE_I2C_OK：读取及应答成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 仅成功时写入 *data；读取最后一个字节时应传入 ack = 0
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_ReadByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t ack, uint8_t *data);

/* 阻塞寄存器接口：address 为未左移的 7 位地址，reg 为 8 位寄存器地址。
 * size 必须大于 0；连续访问方式由从机协议决定。
 * MemRead 使用重复 START，最后一个字节发送 NACK，然后 STOP。
 * 事务开始后失败也会尝试 STOP；读失败时缓冲区可能已包含部分数据。
 */

/**
 * @brief 向指定寄存器连续写入数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器起始地址
 * @param data     待发送的数据缓冲区
 * @param size     写入字节数，必须大于 0
 * @return BSP_SOFTWARE_I2C_OK：写入并停止成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 寄存器地址只发送一次，每个发送字节均检查从机 ACK；事务开始后会尝试 STOP
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_MemWrite(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, const uint8_t *data, uint16_t size);

/**
 * @brief 从指定寄存器连续读取数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器起始地址
 * @param data     接收数据的缓冲区，容量至少为 size 字节
 * @param size     读取字节数，必须大于 0
 * @return BSP_SOFTWARE_I2C_OK：读取并停止成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 发送寄存器地址后使用重复 START 切换到读取，最后一个字节发送 NACK。
 *       事务开始后会尝试 STOP；失败时缓冲区可能已包含部分数据。
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_MemRead(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *data, uint16_t size);

/**
 * @brief 向指定寄存器写入单字节数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器地址
 * @param value    要写入的单字节值
 * @return BSP_SOFTWARE_I2C_OK：写入并停止成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 等价于 size = 1 的 MemWrite，包含起始、从机应答检查及停止操作
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_WriteReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t value);

/**
 * @brief 从指定寄存器读取单字节数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器地址
 * @param value    存放读取结果的单字节变量指针
 * @return BSP_SOFTWARE_I2C_OK：读取并停止成功；其他值见 BSP_SOFTWARE_I2C_Status_t
 * @note 等价于 size = 1 的 MemRead，读取完成后发送 NACK 并尝试 STOP
 */
BSP_SOFTWARE_I2C_Status_t BSP_SOFTWARE_I2C_ReadReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
