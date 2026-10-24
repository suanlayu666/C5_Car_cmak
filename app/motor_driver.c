#include "motor_driver.h"
#include "mx_gpio_default.h"
#include "mx_tim12.h"

/* 电机接线、左右安装方向决定实际转向，这两个值与编码器取反相互独立。
 * 左右轮均已实测原方向相反，因此都设为 -1，统一正占空比对应前进。
 * 若某轮输入正占空比时实际后退，只将对应值的正负号取反。
 */
#define MOTOR_LEFT_DIRECTION  -1
#define MOTOR_RIGHT_DIRECTION -1

/* 保存最终作用到方向引脚上的占空比，用于判断方向变化和共用 STBY 状态。
 * static 变量只在本文件内可见，上电初值为 0。
 */
static int16_t left_duty;
static int16_t right_duty;

void motor_init(void)
{
    /* 先待机、清零，再启动 PWM；必须在 mx_system_init() 成功后调用。 */
    motor_stop();

    hal_tim_handle_t *tim = mx_tim12_gethandle();
    /* HAL2 分别开启输出通道和共用计数器，PWM1 模式由 CubeMX2 配置。 */
    HAL_TIM_OC_StartChannel(tim, HAL_TIM_CHANNEL_1);
    HAL_TIM_OC_StartChannel(tim, HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(tim);
}

void motor_set_left(int16_t duty)
{
    /* 1. 限制占空比，再按左轮实际安装方向修正正负号。 */
    if (duty > 100) duty = 100;
    if (duty < -100) duty = -100;
    duty = duty * MOTOR_LEFT_DIRECTION;

    hal_tim_handle_t *tim = mx_tim12_gethandle();

    /* 2. 启动、换向或停止时，先清掉旧 PWM，再设置方向。
     * CCR 开启了预装载，因此生成更新事件让零值立即生效。
     * 更新事件会重置共用 PWM 周期，不改变另一轮的占空比。
     * 同向调速时跳过这里，直接在第 4 步设置新的 CCR。
     */
    if ((duty > 0 && left_duty <= 0)
        || (duty < 0 && left_duty >= 0)
        || (duty == 0 && left_duty != 0))
    {
        HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_1, 0U);
        HAL_TIM_GenerateEvent(tim, HAL_TIM_SW_EVENT_UPD);
    }

    /* 3. 左轮方向引脚：正数 IN1=1/IN2=0，负数 IN1=0/IN2=1。 */
    if (duty > 0)
    {
        HAL_GPIO_WritePin(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, HAL_GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, HAL_GPIO_PIN_SET);
    }
    else if (duty < 0)
    {
        HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN, HAL_GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN, HAL_GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN | MOTOR_L_IN2_PIN,
                          HAL_GPIO_PIN_RESET);
    }
    left_duty = duty; /* 记录带正负号的指令，用于下次判断换向和共用 STBY。 */

    /* 4. 取绝对值，直接设置左轮 CCR1。
     * CCR = (ARR+1) * 占空比 / 100；当前 30% 对应 7200*30/100=2160。
     */
    if (duty < 0) duty = -duty;
    HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_1,
                                   (HAL_TIM_GetPeriod(tim) + 1U) * (uint32_t)duty / 100U);

    /* 5. 两轮都为零才待机，不能因左轮停止而关闭仍在运行的右轮。 */
    if (left_duty == 0 && right_duty == 0)
        HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, HAL_GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, HAL_GPIO_PIN_SET);
}

void motor_set_right(int16_t duty)
{
    /* 1. 限幅和方向修正，与左轮一致。 */
    if (duty > 100) duty = 100;
    if (duty < -100) duty = -100;
    duty = duty * MOTOR_RIGHT_DIRECTION;

    hal_tim_handle_t *tim = mx_tim12_gethandle();

    /* 2. 启动、换向或停止前，先让右轮原有 PWM 归零。 */
    if ((duty > 0 && right_duty <= 0)
        || (duty < 0 && right_duty >= 0)
        || (duty == 0 && right_duty != 0))
    {
        HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_2, 0U);
        HAL_TIM_GenerateEvent(tim, HAL_TIM_SW_EVENT_UPD);
    }

    /* 3. 直接设置右轮方向引脚。 */
    if (duty > 0)
    {
        HAL_GPIO_WritePin(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, HAL_GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, HAL_GPIO_PIN_SET);
    }
    else if (duty < 0)
    {
        HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN, HAL_GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN, HAL_GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN | MOTOR_R_IN2_PIN,
                          HAL_GPIO_PIN_RESET);
    }
    right_duty = duty;

    /* 4. 直接设置右轮 CCR2，使用占空比的绝对值。 */
    if (duty < 0) duty = -duty;
    HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_2,
                                   (HAL_TIM_GetPeriod(tim) + 1U) * (uint32_t)duty / 100U);

    /* 5. 更新两轮共用的待机引脚。 */
    if (left_duty == 0 && right_duty == 0)
        HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, HAL_GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, HAL_GPIO_PIN_SET);
}

void motor_stop(void)
{
    /* 先拉低总使能，再清除 PWM、方向和软件记录，不等待定时器自然更新。 */
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, HAL_GPIO_PIN_RESET);
    hal_tim_handle_t *tim = mx_tim12_gethandle();
    HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_1, 0U);
    HAL_TIM_OC_SetCompareUnitPulse(tim, HAL_TIM_OC_COMPARE_UNIT_2, 0U);
    HAL_TIM_GenerateEvent(tim, HAL_TIM_SW_EVENT_UPD);
    HAL_GPIO_WritePin(MOTOR_L_IN1_PORT, MOTOR_L_IN1_PIN | MOTOR_L_IN2_PIN,
                      HAL_GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_R_IN1_PORT, MOTOR_R_IN1_PIN | MOTOR_R_IN2_PIN,
                      HAL_GPIO_PIN_RESET);
    left_duty = 0;
    right_duty = 0;
}
