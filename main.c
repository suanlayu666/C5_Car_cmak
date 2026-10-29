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
#include "bluetooth.h"
#include "control.h"
#include "encoder.h"
#include "motor.h"
#include "mx_tim6.h"
#include "mx_uart4.h"
#include "mx_usart2.h"
#include <stdio.h>
#include <string.h>
#include "ccd.h"

int main(void)
{
    if (mx_system_init() != SYSTEM_OK) return -1;
    Motor_Init();
    Encoder_Init();
    Bluetooth_Init();
    CCD_Init();

    LL_TIM_ClearFlag_UPDATE(TIM6);
    HAL_TIM_Start_IT(mx_tim6_gethandle()); /* 10 ms 速度闭环。 */

    uint32_t last_print = HAL_GetTick();

    while (1)
    {
        CCD_Task();
        if ((uint32_t)(HAL_GetTick() - last_print) >= 1000U)
        {
            last_print = HAL_GetTick();
            /* 复制一组一致的数据；不要在主循环再次读取编码器。 */
            uint32_t irq = __get_PRIMASK();
            __disable_irq();
            int16_t tl = Target_Left, tr = Target_Right;
            int32_t left = Encoder_Left, right = Encoder_Right;
            int16_t lp = PWM_Left, rp = PWM_Right;
            __set_PRIMASK(irq);

            char text[96];
            snprintf(text, sizeof(text),
                     "PI10ms TL=%ld L=%ld LP=%ld%% TR=%ld R=%ld RP=%ld%%\r\n",
                     (long)tl, (long)left, (long)lp, (long)tr, (long)right, (long)rp);
            /* USART2 → ST-LINK → 电脑，115200。这里只显示，不接收遥控。 */
            //HAL_UART_Transmit(mx_usart2_uart_gethandle(), text, (uint32_t)strlen(text), 20U);
            /* 同一条数据通过 UART4 → JDY-31 回传，网页每秒显示左右占空比。 */
            HAL_UART_Transmit(mx_uart4_uart_gethandle(), text, (uint32_t)strlen(text), 100U);
        }
    }
}
