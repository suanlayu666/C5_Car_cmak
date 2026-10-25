#include "motor.h"
#include "mx_gpio_default.h"
#include "mx_tim12.h"

void Motor_Init(void)
{
    Motor_Set(0, 0);
    HAL_TIM_OC_StartChannel(mx_tim12_gethandle(), HAL_TIM_CHANNEL_1);
    HAL_TIM_OC_StartChannel(mx_tim12_gethandle(), HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(mx_tim12_gethandle());
}

void Motor_Set(int16_t left, int16_t right)
{
    if (left > 100) left = 100;
    if (left < -100) left = -100;
    if (right > 100) right = 100;
    if (right < -100) right = -100;
    /* 两轮已实测需要反向修正，让正数统一表示小车前进。 */
    left = -left;
    right = -right;
    hal_tim_handle_t *timer = mx_tim12_gethandle();
    /* 改方向前先清掉旧 PWM；更新事件让预装载的零值立即生效。 */
    HAL_TIM_OC_SetCompareUnitPulse(timer, HAL_TIM_OC_COMPARE_UNIT_1, 0U);
    HAL_TIM_OC_SetCompareUnitPulse(timer, HAL_TIM_OC_COMPARE_UNIT_2, 0U);
    HAL_TIM_GenerateEvent(timer, HAL_TIM_SW_EVENT_UPD);
    HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, left > 0 ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, left < 0 ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, right > 0 ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, right < 0 ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);

    /* CCR = (ARR + 1) × 占空比 / 100，当前 ARR + 1 = 7200。 */
    uint32_t period = HAL_TIM_GetPeriod(timer) + 1U;
    HAL_TIM_OC_SetCompareUnitPulse(timer, HAL_TIM_OC_COMPARE_UNIT_1,
                                  period * (uint32_t)(left < 0 ? -left : left) / 100U);
    HAL_TIM_OC_SetCompareUnitPulse(timer, HAL_TIM_OC_COMPARE_UNIT_2,
                                  period * (uint32_t)(right < 0 ? -right : right) / 100U);
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN,
                     left == 0 && right == 0 ? HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET);
}
