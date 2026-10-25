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
#include "main.h"
#include "debug_uart.h"
#include "bluetooth.h"
#include "car_command.h"
#include "encoder.h"
#include "motor_driver.h"
#include "speed_pid.h"
#include "mx_tim6.h"
#include <stdio.h>

#define SPEED_KP 1.0f
#define SPEED_KI 5.0f
#define DIRECTION_STOP_TICKS 5U /* 连续 50 ms 无计数后再反转。 */

static speed_pid_t left_pid;
static speed_pid_t right_pid;
/* 主循环修改目标，TIM6 每 10 ms 执行一次控制。 */
static volatile int16_t left_target = CAR_SPEED_INITIAL;
static volatile int16_t right_target = CAR_SPEED_INITIAL;
/* 仅用于每秒显示，由控制中断更新；主循环不额外读取编码器。 */
static volatile encoder_sample_t latest_sample;
static volatile int16_t left_pwm;
static volatile int16_t right_pwm;

int main(void)
{
  if (mx_system_init() != SYSTEM_OK) return (-1);

  motor_init();
  speed_pid_init(&left_pid, SPEED_KP, SPEED_KI);
  speed_pid_init(&right_pid, SPEED_KP, SPEED_KI);
  car_command_init(); /* 上电速度为 0，用 + 起步。 */
  encoder_init();
  /* 初始化失败会被各自的 read_char 报告，由下面的流程重新接收。 */
  (void)debug_uart_init();
  (void)bluetooth_init();
  uint32_t print_tick = HAL_GetTick();

  while (1)
  {
    uint8_t receive_error = 0U;
    /* 两路各取一个字符，都交给同一个命令处理函数。 */
    for (uint32_t source = 0U; source < 2U; source++)
    {
      char command;
      hal_status_t status = source == 0U ? debug_uart_read_char(&command)
                                         : bluetooth_read_char(&command);
      if (status == HAL_BUSY) continue;
      if (status != HAL_OK) { receive_error = 1U; break; }
      car_target_t target;
      if (car_command_handle(command, &target))
      {
        uint32_t interrupt_state = __get_PRIMASK();
        __disable_irq();
        left_target = target.left;
        right_target = target.right;
        __set_PRIMASK(interrupt_state);
      }
    }

    if (receive_error)
    {
      /* 接收错误：停车、清掉目标和积分，重新等待新命令，不退出主循环。 */
      car_target_t stopped;
      car_command_handle('s', &stopped);
      uint32_t interrupt_state = __get_PRIMASK();
      __disable_irq();
      left_target = 0;
      right_target = 0;
      left_pid.integral = 0.0f;
      right_pid.integral = 0.0f;
      left_pwm = 0;
      right_pwm = 0;
      motor_stop();
      __set_PRIMASK(interrupt_state);
      HAL_Delay(10U); /* 只在接收故障时等待；10 ms 控制中断照常运行。 */
      (void)debug_uart_init();
      (void)bluetooth_init();
      continue;
    }
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - print_tick) >= 1000U)
    {
      print_tick = now;
      /* 一次复制目标、最新 10 ms 反馈和 PWM，复制完立即恢复中断。 */
      uint32_t interrupt_state = __get_PRIMASK();
      __disable_irq();
      const int16_t tl = left_target, tr = right_target;
      const int16_t lp = left_pwm, rp = right_pwm;
      const encoder_sample_t sample = latest_sample;
      __set_PRIMASK(interrupt_state);

      char text[96];
      snprintf(text, sizeof(text),
               "PI10ms TL=%ld L=%ld LP=%ld%% TR=%ld R=%ld RP=%ld%%\r\n",
               (long)tl, (long)sample.left, (long)lp,
               (long)tr, (long)sample.right, (long)rp);
      /* 不把打印结果写入接收状态；打印失败只跳过本次，下一秒再试。 */
      (void)debug_uart_write(text);
    }
  }
}

/* HAL 回调只能定义一份；各模块先判断句柄，只处理自己的串口。 */
void HAL_UART_RxCpltCallback(hal_uart_handle_t *uart, uint32_t size,
                            hal_uart_rx_event_types_t event)
{
  (void)event;
  debug_uart_rx_complete(uart, size);
  bluetooth_rx_complete(uart, size);
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *uart)
{
  debug_uart_rx_error(uart);
  bluetooth_rx_error(uart);
}

void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  static int8_t last_direction;
  static uint8_t waiting;
  static uint8_t stopped_ticks;

  if (htim != mx_tim6_gethandle()) return;
  const encoder_sample_t sample = encoder_read();
  latest_sample = sample;

  /* 当前指令让两轮同向或停一轮，用目标符号判断是否需要前后换向。 */
  int8_t direction = 0;
  if (left_target > 0 || right_target > 0) direction = 1;
  else if (left_target < 0 || right_target < 0) direction = -1;

  if (!waiting && last_direction != 0 && direction != 0 && direction != last_direction)
  {
    waiting = 1U;
    stopped_ticks = 0U; /* 先停驱动，从下一个采样周期开始判断停稳。 */
  }
  else if (waiting)
  {
    if (sample.left == 0 && sample.right == 0) stopped_ticks++;
    else stopped_ticks = 0U;
    if (stopped_ticks >= DIRECTION_STOP_TICKS)
    {
      waiting = 0U;
      last_direction = 0;
    }
  }

  if (waiting)
  {
    left_pid.integral = 0.0f;
    right_pid.integral = 0.0f;
    left_pwm = 0;
    right_pwm = 0;
    motor_stop();
    return;
  }

  /* 停车后保留上次方向，防止刚收到 s 又反转时跳过停稳判断。 */
  if (direction != 0) last_direction = direction;

  left_pwm = (int16_t)speed_pid_update(&left_pid, left_target, sample.left);
  right_pwm = (int16_t)speed_pid_update(&right_pid, right_target, sample.right);
  motor_set_left(left_pwm);
  motor_set_right(right_pwm);
}
