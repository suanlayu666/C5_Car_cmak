#ifndef PID_H
#define PID_H
#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float integral; /* 左右轮各保存一份积分。 */
} PID_TypeDef;

/* 每 10 ms 调用；输入为计数/10 ms，输出为 -30～30 的 PWM 百分比。 */
int16_t Speed_PI(PID_TypeDef *pid, int16_t target, int32_t measured);
#endif
