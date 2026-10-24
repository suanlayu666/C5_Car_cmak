#include "debug_uart.h"
#include "mx_usart2.h"
#include <string.h>

#define DEBUG_UART_RX_SIZE    32U

/* 中断写入、主循环取出。环形缓冲保留一个空位，可保存 31 个待处理字符。 */
static uint8_t rx_byte;
static volatile uint8_t rx_buffer[DEBUG_UART_RX_SIZE];
static volatile uint32_t rx_write;
static volatile uint32_t rx_read;
static volatile uint8_t rx_fault;

hal_status_t debug_uart_init(void)
{
  /* 先停止旧接收，再清缓冲，避免旧的加速命令在恢复后继续执行。
   * HAL2 AbortReceive 会清错误标志并丢弃 UART 中的残留字节。
   */
  HAL_CORTEX_NVIC_DisableIRQ(USART2_IRQn);
  hal_status_t status = HAL_UART_AbortReceive(mx_usart2_uart_gethandle());
  rx_write = 0U;
  rx_read = 0U;
  rx_fault = 1U;
  if (status != HAL_OK) return status;

  status = HAL_UART_Receive_IT(mx_usart2_uart_gethandle(), &rx_byte, 1U);
  if (status != HAL_OK) return status;
  rx_fault = 0U;
  /* TIM6 优先级为 3，速度控制优先于串口；中断入口由本文件提供。 */
  HAL_CORTEX_NVIC_SetPriority(USART2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_5,
                              HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_ClearPendingIRQ(USART2_IRQn);
  HAL_CORTEX_NVIC_EnableIRQ(USART2_IRQn);
  return HAL_OK;
}

hal_status_t debug_uart_read_char(char *character)
{
  if (character == NULL || rx_fault != 0U) return HAL_ERROR;
  if (rx_read == rx_write) return HAL_BUSY;

  *character = (char)rx_buffer[rx_read];
  rx_read = (rx_read + 1U) % DEBUG_UART_RX_SIZE;
  return HAL_OK;
}

void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(mx_usart2_uart_gethandle());
}

/* HAL2 回调多了接收字节数和事件参数；每收到一个字节便重新开启接收。 */
void HAL_UART_RxCpltCallback(hal_uart_handle_t *huart, uint32_t size_byte,
                            hal_uart_rx_event_types_t rx_event)
{
  (void)rx_event;
  if (huart != mx_usart2_uart_gethandle() || rx_fault != 0U) return;

  if (size_byte == 1U && rx_byte != '\r' && rx_byte != '\n')
  {
    uint32_t next = (rx_write + 1U) % DEBUG_UART_RX_SIZE;
    if (next == rx_read)
    {
      rx_fault = 1U; /* 不悄悄丢掉可能是停止命令的字符，交给主循环停机。 */
    }
    else
    {
      rx_buffer[rx_write] = rx_byte;
      rx_write = next;
    }
  }

  if (rx_fault == 0U && HAL_UART_Receive_IT(huart, &rx_byte, 1U) != HAL_OK)
    rx_fault = 1U;
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *huart)
{
  if (huart == mx_usart2_uart_gethandle()) rx_fault = 1U;
}

hal_status_t debug_uart_write(const char *text)
{
  return HAL_UART_Transmit(mx_usart2_uart_gethandle(), text, (uint32_t)strlen(text), 20U);
}
