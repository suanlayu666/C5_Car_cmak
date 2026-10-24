#ifndef SPEED_PID_H
#define SPEED_PID_H

/* PI 控制，每 10 ms 调用一次，输入单位为“计数/10 ms”，
 * 输出单位为 PWM 百分比。当前只控制前进，输出范围为 0~30%。
 */
#define SPEED_PID_SAMPLE_TIME_S  0.01f
#define SPEED_PID_OUTPUT_MAX     30.0f

typedef struct
{
    float kp;        /* 比例系数：误差每增加 1 个计数，P 输出增加多少百分点。 */
    float ki;        /* 积分系数：按秒计算，在 update 中乘以 0.01 s。 */
    float integral;  /* 已累计的 I 输出，每个电机需要各自保存一份。 */
} speed_pid_t;

/* 设置系数并清零积分，须在启动闭环之前调用。 */
void speed_pid_init(speed_pid_t *pid, float kp, float ki);

/* 每 10 ms 调用一次：目标、实际计数必须来自相同的采样周期。
 * target <= 0 时输出 0 并清零积分；当前不处理负目标和倒车闭环。
 * 返回 0~30 的 PWM 百分比。pid 必须是已初始化的有效对象。
 */
float speed_pid_update(speed_pid_t *pid, float target, float feedback);

#endif /* SPEED_PID_H */
