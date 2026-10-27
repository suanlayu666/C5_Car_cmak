#include "pid.h"

int16_t Speed_PI(PID_TypeDef *pid, int16_t target, int32_t measured)
{
    if (target == 0)
    {
        pid->integral = 0.0f;
        return 0;
    }
    float error = (float)target - (float)measured;
    float next_integral = pid->integral + pid->ki * error * 0.01f;
    if (next_integral > 90.0f) next_integral = 90.0f;
    if (next_integral < -90.0f) next_integral = -90.0f;

    /* 输出饱和且误差继续推高饱和时，暂停累加积分。 */
    float output = pid->kp * error + next_integral;
    if ((output > 90.0f && error > 0.0f) || (output < -90.0f && error < 0.0f))
        output = pid->kp * error + pid->integral;
    else
        pid->integral = next_integral;
    if (output > 90.0f) output = 90.0f;
    if (output < -90.0f) output = -90.0f;
    return (int16_t)output;
}
