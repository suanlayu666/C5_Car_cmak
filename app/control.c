#include "control.h"
#include "encoder.h"
#include "motor.h"
#include "pid.h"
#include "mx_tim1.h"
#include "mx_tim2.h"
#include "mx_tim6.h"
#include "ccd.h"

volatile int16_t Target_Left, Target_Right;
volatile int32_t Encoder_Left, Encoder_Right;
volatile int16_t PWM_Left, PWM_Right;

static PID_TypeDef left_pid = {1.0f, 5.0f, 0.0f};
static PID_TypeDef right_pid = {1.0f, 5.0f, 0.0f};

void Control_Command(uint8_t command)
{
    static int16_t speed;       /* 0～20，单位为计数/10 ms。 */
    static int8_t travel = 1;   /* 1 前进，-1 后退。 */
    static char turn = 'f';     /* f 直行、l 左转、r 右转。 */
    switch (command)
    {
        case '+': if (speed < 50) speed++; break;
        case '-': if (speed > 0) speed--; break;
        case 'f': case 'F': travel = 1; turn = 'f'; break;
        case 'b': case 'B': travel = -1; turn = 'f'; break;
        case 'l': case 'L': turn = 'l'; break;
        case 'r': case 'R': turn = 'r'; break;
        case 's': case 'S': speed = 0; turn = 'f'; break;
        default: return;
    }
    int16_t left = speed * travel;
    int16_t right = speed * travel;
    /* 转向停内侧轮；后退时交换停哪一轮，让 l/r 仍对应车头转向。 */
    if ((turn == 'l' && travel > 0) || (turn == 'r' && travel < 0)) left = 0;
    if ((turn == 'r' && travel > 0) || (turn == 'l' && travel < 0)) right = 0;
    /* 两个目标一起更新，避免 TIM6 读到只更新了一半的目标。 */
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    Target_Left = left;
    Target_Right = right;
    __set_PRIMASK(irq);
}

void Control(void)
{
    static int8_t last_direction;
    static uint8_t stopped_ticks;
    Encoder_Left = Read_Speed(mx_tim2_gethandle());
    Encoder_Right = Read_Speed(mx_tim1_gethandle());
    int8_t direction = 0;
    if (Target_Left > 0 || Target_Right > 0) direction = 1;
    else if (Target_Left < 0 || Target_Right < 0) direction = -1;

    /* 前后换向：先关闭驱动，连续 5 次采样停稳后，下一周期再给反向 PWM。 */
    if (direction != 0 && last_direction != 0 && direction != last_direction)
    {
        left_pid.integral = right_pid.integral = 0.0f;
        PWM_Left = PWM_Right = 0;
        Motor_Set(0, 0);
        if (Encoder_Left == 0 && Encoder_Right == 0) stopped_ticks++;
        else stopped_ticks = 0U;
        if (stopped_ticks >= 5U) last_direction = direction;
        return;
    }
    stopped_ticks = 0U;
    if (direction != 0) last_direction = direction;
    PWM_Left = Speed_PI(&left_pid, Target_Left, Encoder_Left);
    PWM_Right = Speed_PI(&right_pid, Target_Right, Encoder_Right);
    Motor_Set(PWM_Left, PWM_Right);
}

void HAL_TIM_UpdateCallback(hal_tim_handle_t *timer)
{
static uint8_t count = 0;

    if (timer == mx_tim6_gethandle()) 
    {
        /*小车速度控制，10ms一次*/
        Control();

        /*CCD采集，20ms一次*/
        count++;
        if (count >= 2)
        {
            count = 0;
            Get_CCD_Value();
        }
    }
}
