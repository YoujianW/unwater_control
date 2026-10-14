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
void BSP_SOFTWARE_I2C_Init(BSP_SOFTWARE_I2C_Handle_t *handle,
    void *scl_port, uint16_t scl_pin, void *sda_port, uint16_t sda_pin);

/* 基础时序接口由调用方组织 START/STOP；参数无效或超时时提前结束。
 * Start 同时支持重复 START；寄存器接口在内部组织完整事务。
 * SendByte 只发送 8 位，随后需调用 WaitAck；ReadByte 包含第 9 位 ACK/NACK。
 */

/**
 * @brief 产生起始信号 START 或重复起始信号（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @note 在 SCL 高电平期间将 SDA 从高拉低，结束时 SCL 为低电平
 */
void BSP_SOFTWARE_I2C_Start(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 产生停止信号 STOP 并释放总线（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @note 在 SCL 高电平期间将 SDA 从低释放为高；超时时也会释放主机对两线的驱动
 */
void BSP_SOFTWARE_I2C_Stop(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 产生第 9 个时钟并读取从机应答（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @return 0：收到从机 ACK；1：未应答、超时或参数无效
 * @note 在 SendByte 后调用；读取完成后拉低 SCL，不会自动产生 STOP
 */
uint8_t BSP_SOFTWARE_I2C_WaitAck(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 主机发送应答 ACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @note 第 9 个时钟期间拉低 SDA，通知从机继续发送；结束时拉低 SCL 并释放 SDA
 */
void BSP_SOFTWARE_I2C_Ack(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 主机发送非应答 NACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @note 第 9 个时钟期间释放 SDA，通知从机结束发送；结束时 SCL 为低电平
 */
void BSP_SOFTWARE_I2C_NAck(BSP_SOFTWARE_I2C_Handle_t *handle);

/**
 * @brief 发送一个字节，高位先行（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @param data    待发送的字节
 * @note 不包含从机应答时钟，发送完成后需调用 WaitAck 判断从机是否接收
 */
void BSP_SOFTWARE_I2C_SendByte(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t data);

/**
 * @brief 读取一个字节，高位先行，并发送 ACK 或 NACK（阻塞）
 * @param handle  已初始化的软件 I2C 句柄指针
 * @param ack     非 0：读取后发送 ACK；0：读取后发送 NACK
 * @return 读取到的字节；超时或参数无效时返回 0
 * @note 读取最后一个字节时应传入 ack = 0；返回 0 也可能是有效数据
 */
uint8_t BSP_SOFTWARE_I2C_ReadByte(BSP_SOFTWARE_I2C_Handle_t *handle, uint8_t ack);

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
 * @note 寄存器地址只发送一次，每个发送字节均检查从机 ACK；事务开始后会尝试 STOP
 */
void BSP_SOFTWARE_I2C_MemWrite(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, const uint8_t *data, uint16_t size);

/**
 * @brief 从指定寄存器连续读取数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器起始地址
 * @param data     接收数据的缓冲区，容量至少为 size 字节
 * @param size     读取字节数，必须大于 0
 * @note 发送寄存器地址后使用重复 START 切换到读取，最后一个字节发送 NACK。
 *       事务开始后会尝试 STOP；失败时缓冲区可能已包含部分数据。
 */
void BSP_SOFTWARE_I2C_MemRead(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *data, uint16_t size);

/**
 * @brief 向指定寄存器写入单字节数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器地址
 * @param value    要写入的单字节值
 * @note 等价于 size = 1 的 MemWrite，包含起始、从机应答检查及停止操作
 */
void BSP_SOFTWARE_I2C_WriteReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t value);

/**
 * @brief 从指定寄存器读取单字节数据（阻塞）
 * @param handle   已初始化的软件 I2C 句柄指针
 * @param address  未左移的 7 位设备地址（0x00 ~ 0x7F）
 * @param reg      8 位寄存器地址
 * @param value    存放读取结果的单字节变量指针
 * @note 等价于 size = 1 的 MemRead，读取完成后发送 NACK 并尝试 STOP
 */
void BSP_SOFTWARE_I2C_ReadReg(BSP_SOFTWARE_I2C_Handle_t *handle,
    uint8_t address, uint8_t reg, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
