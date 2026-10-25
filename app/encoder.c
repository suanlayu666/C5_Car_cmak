#include "encoder.h"
#include "mx_tim1.h"
#include "mx_tim2.h"

static uint32_t last_left;
static uint16_t last_right;

void Encoder_Init(void)
{
    hal_tim_handle_t *left = mx_tim2_gethandle();
    hal_tim_handle_t *right = mx_tim1_gethandle();
    HAL_TIM_SetCounter(left, 0U);
    HAL_TIM_SetCounter(right, 0U);
    last_left = last_right = 0U;
    HAL_TIM_IC_StartChannel(left, HAL_TIM_CHANNEL_1);
    HAL_TIM_IC_StartChannel(left, HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(left);
    HAL_TIM_IC_StartChannel(right, HAL_TIM_CHANNEL_1);
    HAL_TIM_IC_StartChannel(right, HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(right);
}

int32_t Read_Speed(hal_tim_handle_t *timer)
{
    uint32_t current = HAL_TIM_GetCounter(timer);
    int32_t count;
    /* 连续计数取差，不在读取中途清零。TIM2 是 32 位，TIM1 是 16 位。 */
    if (timer == mx_tim2_gethandle())
    {
        count = (int32_t)(current - last_left);
        last_left = current;
    }
    else
    {
        count = (int16_t)(uint16_t)(current - last_right);
        last_right = (uint16_t)current;
    }
    return -count; /* 两轮前进时原始计数均为负，统一取反。 */
}
