#include "bluetooth.h"
#include "control.h"
#include "mx_uart4.h"

static uint8_t rx_byte;

void Bluetooth_Init(void)
{
    HAL_UART_Receive_IT(mx_uart4_uart_gethandle(), &rx_byte, 1U);
}

void HAL_UART_RxCpltCallback(hal_uart_handle_t *uart, uint32_t size, hal_uart_rx_event_types_t event)
{
    (void)event;
    if (uart != mx_uart4_uart_gethandle() || size != 1U) return;
    /* 收到一个字符就执行；回车、换行等无效字符由控制模块忽略。 */
    Control_Command(rx_byte);
    HAL_UART_Receive_IT(uart, &rx_byte, 1U);
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *uart)
{
    if (uart != mx_uart4_uart_gethandle()) return;
    HAL_UART_AbortReceive(uart);
    HAL_UART_Receive_IT(uart, &rx_byte, 1U);
}
