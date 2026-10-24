#include "debug_uart.h"
/* CubeMX2 生成的 USART2 接口，提供 mx_usart2_uart_gethandle()。 */
#include "mx_usart2.h"

/* strlen() 用于计算字符串的字节数，不包含末尾的 '\0'。 */
#include <string.h>

/* 传给 HAL 的发送等待超时参数，单位 ms；不是每次发送固定延时 20 ms。
 * 20U 中的 U 表示无符号整数，与 HAL 的时间参数类型相匹配。
 */
#define DEBUG_UART_TIMEOUT_MS 20U
#define DEBUG_UART_RX_SIZE    32U

/* 中断写入、主循环取出。环形缓冲保留一个空位，可保存 31 个待处理字符。 */
static uint8_t rx_byte;
static volatile uint8_t rx_buffer[DEBUG_UART_RX_SIZE];
static volatile uint32_t rx_write;
static volatile uint32_t rx_read;
static volatile uint8_t rx_fault;

hal_status_t debug_uart_init(void)
{
  rx_write = 0U;
  rx_read = 0U;
  rx_fault = 0U;

  hal_status_t status = HAL_UART_Receive_IT(mx_usart2_uart_gethandle(), &rx_byte, 1U);
  if (status != HAL_OK) return status;

  /* TIM6 为优先级 3，串口为 5，速度控制可优先于串口接收执行。
   * 当前 CubeMX2 未生成 USART2 中断，因此由本模块提供 IRQHandler。
   */
  HAL_CORTEX_NVIC_SetPriority(USART2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_5,
                              HAL_CORTEX_NVIC_SUB_PRIORITY_0);
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
  if (huart != mx_usart2_uart_gethandle()) return;

  if (size_byte == 1U)
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
  /* NULL 表示没有有效字符串地址，不能交给 strlen() 或 HAL 读取。 */
  if (text == NULL)
  {
    return HAL_ERROR;
  }

  const size_t length = strlen(text);
  /* 空字符串 "" 是有效输入，但没有数据需要发送，直接报告成功。 */
  if (length == 0U)
  {
    return HAL_OK;
  }

  /* HAL2 四个参数依次为：
   * 1. USART2 句柄：描述已初始化的串口实例，直接复用生成代码中的对象。
   * 2. 待发送文本的地址。
   * 3. 字节数：将 strlen() 的 size_t 转为 HAL 接口要求的 uint32_t。
   * 4. 发送等待超时参数，单位 ms。
   * 此调用使用轮询方式等待发送完成，期间主循环会短暂等待；不使用 DMA。
   * 原样返回 HAL 状态，让调用者判断成功、忙、超时或其他错误。
   */
  return HAL_UART_Transmit(mx_usart2_uart_gethandle(), text,
                           (uint32_t)length, DEBUG_UART_TIMEOUT_MS);
}
