#include "speed_pid.h"

void speed_pid_init(speed_pid_t *pid, float kp, float ki)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->integral = 0.0f;
}

float speed_pid_update(speed_pid_t *pid, float target, float feedback)
{
    /* 零目标直接停止并清除历史积分，防止旧积分继续要求电机运行。 */
    if (target == 0.0f)
    {
        pid->integral = 0.0f;
        return 0.0f;
    }

    /* 1. 误差 = 目标 - 实际。实际偏慢时为正，偏快时为负。 */
    float error = target - feedback;

    /* 2. P 项立即响应误差，I 项逐次累计，补偿长期存在的速度偏差。
     * 例如 ki=5、error=10：每次 I 增加 5*10*0.01=0.5 个百分点。
     */
    float proportional = pid->kp * error;
    float next_integral = pid->integral + pid->ki * error * SPEED_PID_SAMPLE_TIME_S;

    /* 积分自身也限幅，防止误差长时间存在时无限累计。 */
    if (next_integral > SPEED_PID_OUTPUT_MAX) next_integral = SPEED_PID_OUTPUT_MAX;
    if (next_integral < SPEED_PID_OUTPUT_MIN) next_integral = SPEED_PID_OUTPUT_MIN;

    /* 3. 如果输出已越界，而且误差还推动它继续越界，本次不累加积分。
     * 例如已经超过 30%，还要求加速时冻结积分，避免解除限制后难以恢复。
     */
    float output = proportional + next_integral;
    if ((output > SPEED_PID_OUTPUT_MAX && error > 0.0f)
        || (output < SPEED_PID_OUTPUT_MIN && error < 0.0f))
    {
        output = proportional + pid->integral;
    }
    else
    {
        pid->integral = next_integral;
    }

    /* 4. 正输出驱动前进，负输出驱动后退，两侧限幅均为 30%。 */
    if (output > SPEED_PID_OUTPUT_MAX) output = SPEED_PID_OUTPUT_MAX;
    if (output < SPEED_PID_OUTPUT_MIN) output = SPEED_PID_OUTPUT_MIN;
    return output;
}
