#include "debug_uart.h"
/* CubeMX2 生成的 USART2 接口，提供 mx_usart2_uart_gethandle()。 */
#include "mx_usart2.h"

/* strlen() 用于计算字符串的字节数，不包含末尾的 '\0'。 */
#include <string.h>

/* 传给 HAL 的发送等待超时参数，单位 ms；不是每次发送固定延时 20 ms。
 * 20U 中的 U 表示无符号整数，与 HAL 的时间参数类型相匹配。
 */
#define DEBUG_UART_TIMEOUT_MS 20U

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
