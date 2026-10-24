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
#include "encoder.h"
#include "motor_driver.h"
#include "speed_pid.h"
#include "mx_tim6.h"
#include <stdio.h>

/* 目标单位：计数/10 ms；两轮分别保存 PI 状态。 */
#define LEFT_TARGET_COUNT  15
#define LEFT_PI_KP         1.0f
#define LEFT_PI_KI         5.0f
#define RIGHT_TARGET_COUNT 15
#define RIGHT_PI_KP        1.0f
#define RIGHT_PI_KI        5.0f
static speed_pid_t left_pid;
static speed_pid_t right_pid;
/* 主循环与中断共享的运行标志和 PWM 输出。 */
static volatile uint8_t pi_enabled = 0U;
static volatile int16_t left_pwm = 0;
static volatile int16_t right_pwm = 0;

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

  /* 上电等待两秒再持续运行，PWM 由中断统一更新。 */
  HAL_Delay(2000);
  pi_enabled = 1U;
  uint32_t uart_tick = HAL_GetTick();

  while (1)
  {
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - uart_tick) >= 1000U)
    {
      uart_tick = now;

      /* 一次复制同一轮控制的计数和 PWM；串口发送放在保护区外。 */
      uint32_t interrupt_state = __get_PRIMASK();
      __disable_irq();
      const encoder_sample_t sample = encoder_get_sample();
      const int16_t left_output = left_pwm;
      const int16_t right_output = right_pwm;
      __set_PRIMASK(interrupt_state);

      /* 每秒显示最新的 10 ms 反馈，不累计这一秒的计数。 */
      char text[96];
      snprintf(text, sizeof(text),
               "PI10ms TL=%ld L=%ld LP=%ld%% TR=%ld R=%ld RP=%ld%%\r\n",
               (long)LEFT_TARGET_COUNT, (long)sample.left, (long)left_output,
               (long)RIGHT_TARGET_COUNT, (long)sample.right, (long)right_output);
      if (debug_uart_write(text) != HAL_OK)
      {
        /* 先禁止 PI，再停止电机，防止下个中断重新启动驱动。 */
        interrupt_state = __get_PRIMASK();
        __disable_irq();
        pi_enabled = 0U;
        left_pwm = 0;
        right_pwm = 0;
        motor_stop();
        __set_PRIMASK(interrupt_state);
        return (-1);
      }
    }
  }
}

/* 每 10 ms 按“采样 -> PI -> PWM”的顺序执行，禁止在中断中发送串口或延时。 */
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  if (htim == mx_tim6_gethandle())
  {
    encoder_update();

    if (pi_enabled != 0U)
    {
      const encoder_sample_t sample = encoder_get_sample();
      /* 两轮共用算法，分别使用自己的目标、反馈和积分状态。 */
      float left_output = speed_pid_update(&left_pid, (float)LEFT_TARGET_COUNT,
                                           (float)sample.left);
      float right_output = speed_pid_update(&right_pid, (float)RIGHT_TARGET_COUNT,
                                            (float)sample.right);
      left_pwm = (int16_t)left_output; /* 输出已限幅到 0~30，转成整数百分点。 */
      right_pwm = (int16_t)right_output;
      motor_set_left(left_pwm);
      motor_set_right(right_pwm);
    }
  }
}

