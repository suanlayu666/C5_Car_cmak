#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>

void Motor_Init(void);
/* PWM 百分比：正数前进，负数后退，0 停止。 */
void Motor_Set(int16_t left, int16_t right);
#endif
