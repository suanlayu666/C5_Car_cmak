#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "stm32c5xx_hal_uart.h"

/* 首次启动或接收故障后调用：清空旧数据，重新开启 USART2 中断接收。 */
hal_status_t debug_uart_init(void);

/* 主循环取一个字符：HAL_OK 有数据，HAL_BUSY 暂无数据，其余状态需重启接收。 */
hal_status_t debug_uart_read_char(char *character);

/* 接收事件由 main.c 的统一 HAL 回调分发。 */
void debug_uart_rx_complete(hal_uart_handle_t *uart, uint32_t size);
void debug_uart_rx_error(hal_uart_handle_t *uart);

/* 主循环发送调试文本；发送失败不影响接收和运动控制。text 必须有效。 */
hal_status_t debug_uart_write(const char *text);

#endif /* DEBUG_UART_H */
