#ifndef CONTROL_H
#define CONTROL_H
#include <stdint.h>

extern volatile int16_t Target_Left, Target_Right;
extern volatile int32_t Encoder_Left, Encoder_Right;
extern volatile int16_t PWM_Left, PWM_Right;

void Control_Command(uint8_t command);
void Control(void); /* TIM6 每 10 ms 调用一次。 */
#endif
