#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 一个 10 ms 采样周期内的左右轮净计数。 */
typedef struct
{
    int32_t left;
    int32_t right;
} encoder_sample_t;

/* 系统初始化成功后调用一次，启动左右编码器和 TIM6 的 10 ms 采样中断。 */
void encoder_init(void);

/* 只在 TIM6 的 10 ms 回调中调用一次，返回连续计数的前后差值。
 * 前进正、后退负，单位为计数/10 ms，不是 RPM。
 * 本次读取会更新上次计数，因此主循环不要额外调用，否则会拆分采样周期。
 */
encoder_sample_t encoder_read(void);

#endif /* ENCODER_H */
