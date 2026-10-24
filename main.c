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
#include "car_command.h"
#include "encoder.h"
#include "motor_driver.h"
#include "speed_pid.h"
#include "mx_tim6.h"
#include <stdio.h>

/* 目标单位：计数/10 ms；两轮分别保存 PI 状态。 */
#define LEFT_PI_KP         1.0f
#define LEFT_PI_KI         5.0f
#define RIGHT_PI_KP        1.0f
#define RIGHT_PI_KI        5.0f
#define DIRECTION_STOP_TICKS 5U /* 连续 5 个 10 ms 周期无计数，再执行反向目标。 */
static speed_pid_t left_pid;
static speed_pid_t right_pid;
/* 主循环与中断共享的运行标志和 PWM 输出。 */
static volatile uint8_t pi_enabled = 0U;
static volatile uint8_t direction_wait = 0U;
static volatile int16_t left_pwm = 0;
static volatile int16_t right_pwm = 0;
static volatile int16_t left_target = CAR_SPEED_INITIAL;
static volatile int16_t right_target = CAR_SPEED_INITIAL;

int main(void)
{
  /* 时钟、GPIO、定时器和串口由 CubeMX2 生成的代码初始化。 */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }

  motor_init(); /* 启动 PWM，保持 STBY=Low。 */
  speed_pid_init(&left_pid, LEFT_PI_KP, LEFT_PI_KI);
  speed_pid_init(&right_pid, RIGHT_PI_KP, RIGHT_PI_KI);
  encoder_init(); /* 启动编码器和 TIM6 的 10 ms 中断。 */
  car_command_init();
  if (debug_uart_init() != HAL_OK) return (-1); /* 此时仍保持电机待机。 */

  /* 保留两秒启动等待，但等待期间也能接收 s 等命令。 */
  const uint32_t start_tick = HAL_GetTick();
  uint32_t uart_tick = start_tick;

  while (1)
  {
    const uint32_t now = HAL_GetTick();
    if (pi_enabled == 0U && (uint32_t)(now - start_tick) >= 2000U)
      pi_enabled = 1U;

    char command;
    hal_status_t uart_status = debug_uart_read_char(&command);
    if (uart_status == HAL_OK)
    {
      car_target_t target;
      if (car_command_handle(command, &target) != 0U)
      {
        /* 两个目标一起更新，防止中断读到一新一旧的转向指令。 */
        uint32_t interrupt_state = __get_PRIMASK();
        __disable_irq();
        left_target = target.left;
        right_target = target.right;
        __set_PRIMASK(interrupt_state);
        uart_tick = now - 1000U; /* 有效命令立即回显目标和当前反馈。 */
      }
    }
    else if (uart_status == HAL_BUSY)
    {
      uart_status = HAL_OK; /* 没有新字符是正常状态。 */
    }

    if (uart_status == HAL_OK && (uint32_t)(now - uart_tick) >= 1000U)
    {
      uart_tick = now;

      /* 一次复制同一轮控制的计数和 PWM；串口发送放在保护区外。 */
      uint32_t interrupt_state = __get_PRIMASK();
      __disable_irq();
      const encoder_sample_t sample = encoder_get_sample();
      const int16_t left_output = left_pwm;
      const int16_t right_output = right_pwm;
      const int16_t left_goal = left_target;
      const int16_t right_goal = right_target;
      const uint8_t waiting = direction_wait;
      __set_PRIMASK(interrupt_state);

      /* 每秒显示最新的 10 ms 反馈，不累计这一秒的计数。 */
      char text[96];
      snprintf(text, sizeof(text),
               "PI10ms TL=%ld L=%ld LP=%ld%% TR=%ld R=%ld RP=%ld%% WAIT=%lu\r\n",
               (long)left_goal, (long)sample.left, (long)left_output,
               (long)right_goal, (long)sample.right, (long)right_output,
               (unsigned long)waiting);
      uart_status = debug_uart_write(text);
    }

    if (uart_status != HAL_OK)
    {
      /* 收发错误或接收缓冲溢出时，先禁止 PI，再停止两轮。 */
      uint32_t interrupt_state = __get_PRIMASK();
      __disable_irq();
      pi_enabled = 0U;
      direction_wait = 0U;
      left_pwm = 0;
      right_pwm = 0;
      motor_stop();
      __set_PRIMASK(interrupt_state);
      return (-1);
    }
  }
}

/* 每 10 ms 按“采样 -> PI -> PWM”的顺序执行，禁止在中断中发送串口或延时。 */
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  /* 保留上次非零方向，即使先收到 s，再收到反向命令，也要检查是否停稳。 */
  static int8_t last_direction = 0;
  static uint8_t stopped_ticks = 0U;

  if (htim == mx_tim6_gethandle())
  {
    encoder_update();

    if (pi_enabled != 0U)
    {
      const encoder_sample_t sample = encoder_get_sample();

      /* 当前命令两轮同向，转弯时一轮为 0；因此用一个符号表示前进/后退。 */
      int8_t requested_direction = 0;
      if (left_target > 0 || right_target > 0) requested_direction = 1;
      else if (left_target < 0 || right_target < 0) requested_direction = -1;

      if (direction_wait == 0U && last_direction != 0
          && requested_direction != 0 && requested_direction != last_direction)
      {
        direction_wait = 1U;
        stopped_ticks = 0U; /* 先停止驱动，从下一个采样周期开始判断。 */
      }
      else if (direction_wait != 0U)
      {
        if (sample.left == 0 && sample.right == 0) stopped_ticks++;
        else stopped_ticks = 0U;

        if (stopped_ticks >= DIRECTION_STOP_TICKS)
        {
          direction_wait = 0U;
          last_direction = 0;
        }
      }

      if (direction_wait != 0U)
      {
        /* 等待期间停止两轮并清积分；不延时，串口可以继续接收最新命令。 */
        left_pid.integral = 0.0f;
        right_pid.integral = 0.0f;
        left_pwm = 0;
        right_pwm = 0;
        motor_stop();
        return;
      }

      if (requested_direction != 0) last_direction = requested_direction;

      /* 两轮共用算法，分别使用自己的目标、反馈和积分状态。 */
      float left_output = speed_pid_update(&left_pid, (float)left_target,
                                           (float)sample.left);
      float right_output = speed_pid_update(&right_pid, (float)right_target,
                                            (float)sample.right);
      left_pwm = (int16_t)left_output; /* 输出已限幅到 -30~30，保留方向正负号。 */
      right_pwm = (int16_t)right_output;
      motor_set_left(left_pwm);
      motor_set_right(right_pwm);
    }
  }
}

