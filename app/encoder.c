#include "encoder.h"
#include "mx_tim1.h"
#include "mx_tim2.h"
#include "mx_tim6.h"

/* 中断更新、主循环读取；volatile 保证每次访问实际内存。 */
static volatile encoder_sample_t latest_sample;

/* 保存上次计数，定时器连续运行；本次减上次就是这 10 ms 的净计数。 */
static uint32_t left_previous;
static uint16_t right_previous;

void encoder_init(void)
{
    /* CubeMX2 已经配置了编码器模式、引脚、滤波等参数。
     * 此处只负责启动，调用前必须已完成 mx_system_init()。
     */
    hal_tim_handle_t *left = mx_tim2_gethandle();
    hal_tim_handle_t *right = mx_tim1_gethandle();

    /* 左轮 TIM2：CH1 接 A 相，CH2 接 B 相，再启动计数器。 */
    HAL_TIM_IC_StartChannel(left, HAL_TIM_CHANNEL_1);
    HAL_TIM_IC_StartChannel(left, HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(left);

    /* 右轮 TIM1：同样开启两个输入通道和计数器。 */
    HAL_TIM_IC_StartChannel(right, HAL_TIM_CHANNEL_1);
    HAL_TIM_IC_StartChannel(right, HAL_TIM_CHANNEL_2);
    HAL_TIM_Start(right);

    /* TIM6 时钟为 144 MHz，CubeMX2 已配置 PSC=143、ARR=9999：
     * 周期 = (143+1) * (9999+1) / 144000000 = 0.01 s = 10 ms。
     * 启动采样前统一清零，让第一段计数从这里开始累计。
     */
    HAL_TIM_SetCounter(left, 0U);
    HAL_TIM_SetCounter(right, 0U);
    HAL_TIM_SetCounter(mx_tim6_gethandle(), 0U);
    left_previous = HAL_TIM_GetCounter(left);
    right_previous = (uint16_t)HAL_TIM_GetCounter(right);

    /* 配置定时器时可能留下更新标志；先清除，避免一启动就进入一次中断。
     * 这里使用库提供的 LL 接口清标志，随后用 HAL2 接口启动更新中断。
     */
    LL_TIM_ClearFlag_UPDATE(TIM6);
    HAL_TIM_Start_IT(mx_tim6_gethandle());
}

/* 连续计数取差值，不在采样时清零；前进时原始计数为负，返回时取反。 */
static int32_t encoder_read_left(void)
{
    hal_tim_handle_t *tim = mx_tim2_gethandle();

    /* TIM2 为 32 位：无符号相减自然处理 0/0xFFFFFFFF 之间的回绕。
     * 再转成有符号差值；例如 current=0xFFFFFFFF、previous=0，差值为 -1。
     * 此转换适用于本工程的 ARM GCC；10 ms 内净计数绝对值须小于 2^31。
     */
    uint32_t current = HAL_TIM_GetCounter(tim);
    int32_t count = (int32_t)(current - left_previous);
    left_previous = current;

    /* 实测左轮前进时原始计数为负，取反后统一为“前进正、后退负”。 */
    return -count;
}

static int32_t encoder_read_right(void)
{
    hal_tim_handle_t *tim = mx_tim1_gethandle();

    /* TIM1 为 16 位，先把差值截成 16 位，再按有符号数解释。
     * 例如 previous=2、current=65535，实际是减了 3，结果应为 -3。
     * 10 ms 内净计数须处于 -32768~32767，才能区分回绕与真实大位移。
     */
    uint16_t current = (uint16_t)HAL_TIM_GetCounter(tim);
    int32_t count = (int16_t)(uint16_t)(current - right_previous);
    right_previous = current;

    /* 实测右轮前进时原始计数也为负，因此同样取反。 */
    return -count;
}

/* 编码器模块只负责采样。由 main.c 的 TIM6 回调按固定周期调用。 */
void encoder_update(void)
{
    latest_sample.left = encoder_read_left();
    latest_sample.right = encoder_read_right();
}

encoder_sample_t encoder_get_sample(void)
{
    /* volatile 不能保证一整组字段在同一时刻读取完成。
     * 复制这几个字段时短暂屏蔽中断，防止左轮是旧数据、右轮却是新数据。
     * 保存并恢复原来的屏蔽状态，避免意外打开调用者已经关闭的中断。
     */
    uint32_t interrupt_state = __get_PRIMASK();
    __disable_irq();
    encoder_sample_t sample = latest_sample;
    __set_PRIMASK(interrupt_state);

    return sample;
}
