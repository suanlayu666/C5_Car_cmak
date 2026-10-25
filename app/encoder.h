#ifndef ENCODER_H
#define ENCODER_H
#include "stm32c5xx_hal_tim.h"

void Encoder_Init(void);
/* 每 10 ms 对每轮读一次；返回净计数，前进为正，后退为负。 */
int32_t Read_Speed(hal_tim_handle_t *timer);
#endif
