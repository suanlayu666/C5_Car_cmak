#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "stm32c5xx_hal_uart.h"

/* UART4：PB8 RX、PB9 TX、9600；配置由 CubeMX2 生成。 */
hal_status_t bluetooth_init(void);
/* 每条指令是一个字符加换行。HAL_OK 有指令，HAL_BUSY 暂无，HAL_ERROR 接收故障。 */
hal_status_t bluetooth_read_char(char *command);
/* main.c 的统一 HAL 回调负责分发，应用其他位置不调用。 */
void bluetooth_rx_complete(hal_uart_handle_t *uart, uint32_t size);
void bluetooth_rx_error(hal_uart_handle_t *uart);

#endif
