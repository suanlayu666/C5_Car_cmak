/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
  ******************************************************************************
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* 生成的 PE2 引脚别名，避免在应用里重复硬编码端口和引脚号。 */
#include "mx_gpio_default.h"
/* 自己编写的串口模块，主循环只调用接口，不直接处理 USART2 配置。 */
#include "debug_uart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

/**
  * brief:  The application entry point.
  * retval: none but we specify int to comply with C99 standard
  */
int main(void)
{
  /** System Init: this code placed in targets folder initializes your system.
    * It calls the initialization (and sets the initial configuration) of the peripherals.
    * You can use STM32CubeMX to generate and call this code or not in this project.
    * It also contains the HAL initialization and the initial clock configuration.
    */
  /* 时钟、GPIO、USART2 等由生成代码统一初始化；失败时不进入应用循环。
   * PB7/STBY 在 GPIO 初始化时设为 Low，本测试不会使能电机驱动。
   */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /* HAL_GetTick() 返回毫秒计数。两个变量分别记录各任务上次执行的时刻。
     * 它们在 while 外定义，才能在每轮循环之间保留记录。
     */
    uint32_t led_tick = HAL_GetTick();
    uint32_t uart_tick = led_tick;

    while (1)
    {
      /* 每轮读取当前时间，让两个任务根据各自的时间间隔决定是否执行。
       * 不用 HAL_Delay(500)，主循环可以继续轮询其他任务。
       * 但 debug_uart_write() 自身仍是短暂阻塞发送，并非异步接口。
       */
      const uint32_t now = HAL_GetTick();

      /* 无符号时间差表示经过的毫秒数；持续轮询时也能跨越 tick 计数回绕。
       * 每 500 ms 翻转一次，因此完整的亮灭周期约为 1 s。
       */
      if ((uint32_t)(now - led_tick) >= 500U)
      {
        /* 记录实际执行时刻；这是主循环调度，不是硬件精确定时。 */
        led_tick = now;
        HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
      }

      /* 串口任务每 1000 ms 执行一次，与 LED 任务分别计时。 */
      if ((uint32_t)(now - uart_tick) >= 1000U)
      {
        uart_tick = now;
        /* \r\n 是回车和换行，方便电脑串口工具将每次输出显示为新的一行。 */
        if (debug_uart_write("C5_Car USART2 OK\r\n") != HAL_OK)
        {
          /* 当前验证程序遇到发送错误就结束 main，不继续重复发送。 */
          return (-1);
        }
      }
    }
  }
} /* end main */

