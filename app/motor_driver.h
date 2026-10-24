#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>

/* 在 mx_system_init() 成功后调用一次：启动两路 PWM，并保持 STBY=Low。
 * 本模块复用 CubeMX2 的配置，不重复初始化 GPIO、时钟和定时器参数。
 */
void motor_init(void);

/* 设置单轮占空比：-100 ~ 100，超出范围自动限制。
 * 正数/负数表示两个方向，绝对值是百分比，不是 RPM。
 * 例如 30 为一个方向的 30% PWM，-30 为相反方向的 30% PWM。
 * 正数是否对应实际前进需要单轮测试，方向修正在 motor_driver.c 顶部。
 * 0 停止该轮；另一轮仍运行时不会拉低共用的 STBY。
 * 运行中反转前应先停止，等车轮停下后再给反向指令；这里没有减速规划。
 * 初始化后调用。当前两轮都由 TIM6 中断中的 PI 按固定周期调节。
 * 主循环不能与中断同时改电机状态；停止前须关闭 PI，更新过程需避免被打断。
 */
void motor_set_left(int16_t duty);
void motor_set_right(int16_t duty);

/* 停止两轮并拉低 STBY，使两路输出进入高阻待机。
 * 这是释放驱动，不是主动刹车，车轮可能因惯性继续转动。
 * 不关闭 TIM12，之后仍可通过 motor_set_left/right() 恢复驱动。
 */
void motor_stop(void);

#endif /* MOTOR_DRIVER_H */
