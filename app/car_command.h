#ifndef CAR_COMMAND_H
#define CAR_COMMAND_H

#include <stdint.h>

/* 单位都是“计数/10 ms”，不是 PWM 百分比。 */
#define CAR_SPEED_INITIAL  15
#define CAR_SPEED_MAX      20

typedef struct
{
    int16_t left;
    int16_t right;
} car_target_t;

/* 基础速度恢复为 15，方向恢复为前进直行。仅由主循环使用。 */
void car_command_init(void);

/* + 加速，- 减速，l 左转，r 右转，f 前进直行，b 后退直行，s 停止。
 * +/- 调整速度大小，保留前后方向和转向；l/r 持续转向，停一轮转另一轮。
 * 后退时交换停哪一轮，让 l/r 始终对应车头向左/向右转动。
 * s 将速度置零、清除转向，保留前后方向；f/b 切换方向并恢复直行。
 * 前后换向由控制回调先停止两轮，连续 50 ms 无编码器计数后执行最新目标。
 * 返回 1 表示命令有效，target 保存新的左右目标；未知字符、换行返回 0。
 */
uint8_t car_command_handle(char command, car_target_t *target);

#endif /* CAR_COMMAND_H */
