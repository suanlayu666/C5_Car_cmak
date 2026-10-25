#include "bluetooth.h"
#include "mx_uart4.h"

#define RX_SIZE 32U
static uint8_t rx_byte;
static volatile uint8_t buffer[RX_SIZE];
static volatile uint32_t read_pos, write_pos;
static volatile uint8_t rx_fault;
static uint8_t line_length, line_command;

hal_status_t bluetooth_init(void)
{
    HAL_CORTEX_NVIC_DisableIRQ(UART4_IRQn);
    hal_status_t status = HAL_UART_AbortReceive(mx_uart4_uart_gethandle());
    read_pos = write_pos = line_length = 0U;
    rx_fault = 1U;
    if (status != HAL_OK) return status;
    status = HAL_UART_Receive_IT(mx_uart4_uart_gethandle(), &rx_byte, 1U);
    if (status != HAL_OK) return status;
    rx_fault = 0U;
    /* 优先级由 CubeMX2 设置为 5，中断入口也已生成。 */
    HAL_CORTEX_NVIC_ClearPendingIRQ(UART4_IRQn);
    HAL_CORTEX_NVIC_EnableIRQ(UART4_IRQn);
    return HAL_OK;
}

hal_status_t bluetooth_read_char(char *command)
{
    if (rx_fault) return HAL_ERROR;
    if (read_pos == write_pos) return HAL_BUSY;
    *command = (char)buffer[read_pos];
    read_pos = (read_pos + 1U) % RX_SIZE;
    return HAL_OK;
}

void bluetooth_rx_complete(hal_uart_handle_t *uart, uint32_t size)
{
    if (uart != mx_uart4_uart_gethandle() || rx_fault) return;
    if (size == 1U)
    {
        if (rx_byte == '\r' || rx_byte == '\n')
        {
            /* 只接收单字符行，JDY-31 的多字符状态文本整行忽略。 */
            if (line_length == 1U)
            {
                uint32_t next = (write_pos + 1U) % RX_SIZE;
                if (next == read_pos) rx_fault = 1U;
                else { buffer[write_pos] = line_command; write_pos = next; }
            }
            line_length = 0U;
        }
        else
        {
            if (line_length == 0U) line_command = rx_byte;
            if (line_length < 2U) line_length++; /* 2 表示多字符行，不再累加。 */
        }
    }
    if (!rx_fault && HAL_UART_Receive_IT(uart, &rx_byte, 1U) != HAL_OK) rx_fault = 1U;
}

void bluetooth_rx_error(hal_uart_handle_t *uart)
{
    if (uart == mx_uart4_uart_gethandle()) rx_fault = 1U;
}
