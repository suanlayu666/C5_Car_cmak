/* 头文件只公开接口；具体发送过程放在 debug_uart.c 中。 */
#ifndef DEBUG_UART_H
#define DEBUG_UART_H

/* 提供 hal_status_t、HAL_OK、HAL_ERROR 等 HAL2 类型和状态码。 */
#include "stm32c5xx_hal_def.h"

/* 系统初始化成功后调用一次，开启 USART2 单字节接收中断和接收缓冲。 */
hal_status_t debug_uart_init(void);

/* 主循环取出一个字符，不等待数据。
 * HAL_OK：收到字符；HAL_BUSY：缓冲为空；HAL_ERROR：接收错误或缓冲溢出。
 * character 必须是有效地址。中断只收字节，命令解析放在主循环。
 */
hal_status_t debug_uart_read_char(char *character);

/**
 * @brief  通过 USART2 发送一段文本，不自动添加换行。
 * @param  text  以 '\0' 结尾的有效 C 字符串；const 表示函数不修改文本。
 * @return HAL_OK 表示发送成功或文本为空；其他 HAL 状态表示发送失败。
 *         NULL 指针由本模块返回 HAL_ERROR。
 * @note   必须在 mx_system_init() 成功后调用，复用已初始化的 USART2。
 *         本接口阻塞等待发送完成，只在主循环使用，不在控制中断中调用。
 *         文本遇到第一个 '\0' 就结束，不适合发送包含零字节的二进制数据。
 */
hal_status_t debug_uart_write(const char *text);

#endif /* DEBUG_UART_H */
