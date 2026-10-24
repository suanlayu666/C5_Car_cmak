#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* 保存最近一个 10 ms 内的左右轮净计数。 */
typedef struct
{
    int32_t left;
    int32_t right;
} encoder_sample_t;

/* 系统初始化成功后调用一次，启动左右编码器和 TIM6 的 10 ms 采样中断。 */
void encoder_init(void);

/* 在 TIM6 的 10 ms 回调中调用一次，读取连续计数的前后差值并保存结果。
 * 调度位置在 main.c，主循环只调用下面的 get_sample()，不重复采样。
 */
void encoder_update(void);

/* 获取最近一次完整采样，不清零硬件计数器，也不消耗已保存的结果。
 * 前进为正，后退为负；返回的是每 10 ms 的计数，不是 RPM。
 * 主循环显示时使用本接口，计数差值统一由 TIM6 中断更新。
 */
encoder_sample_t encoder_get_sample(void);

#endif /* ENCODER_H */
