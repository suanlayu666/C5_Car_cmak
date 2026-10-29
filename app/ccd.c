#include "ccd.h"
#include "mx_adc1.h"
#include "mx_gpio_default.h"
#include "mx_tim7.h"
#include "stm32c5xx_ll_tim.h"
#include "mx_usart2.h"
#include <stdio.h>

volatile uint16_t CCD_Value[128];
static char ccd_text[768];

#define CCD_CLK(v) HAL_GPIO_WritePin(CCD_CLK_PORT, CCD_CLK_PIN, (v) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
#define CCD_SI(v) HAL_GPIO_WritePin(CCD_SI_PORT, CCD_SI_PIN, (v) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);

/*tim7每1us触发一次*/
static void Delay_us(uint16_t us)
{
    uint16_t start = (uint16_t)LL_TIM_GetCounter(TIM7);
    while((uint16_t)(LL_TIM_GetCounter(TIM7) - start) < us)
    {

    }
}

/*读取一次ao电压*/ 
static uint16_t Get_ADC_Value(void)
{
    hal_adc_handle_t *adc = mx_adc1_gethandle();
    HAL_ADC_REG_StartConv(adc);
    HAL_ADC_REG_PollForConv(adc, 2U);
    return (uint16_t)HAL_ADC_REG_ReadConversionData(adc);
}

void CCD_Init(void)
{
    CCD_CLK(0);
    CCD_SI(0);

    /*启动微妙计时*/
    HAL_TIM_Start(mx_tim7_gethandle());

    /*启动ADC*/
    HAL_ADC_Start(mx_adc1_gethandle());

    HAL_ADC_Calibrate(mx_adc1_gethandle());
}

/*每20ms调用一次，读取每行128个像素值*/
void Get_CCD_Value()
{
    CCD_CLK(0);
    CCD_SI(1);
    Delay_us(20);

    CCD_CLK(1);
    CCD_SI(0);

    for(int i = 0; i < 128; i++)
    {
        CCD_CLK(0);
        Delay_us(1);

        /*12位结果向右移动四位，转换为0-255*/
        CCD_Value[i] = Get_ADC_Value() >> 4;

        CCD_CLK(1); 
        Delay_us(1);
    }
    /*产生129个上升沿，结束读取*/
    CCD_CLK(0);
}

void CCD_Task(void)
{
    static uint32_t last_send = 0;

    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_send) >= 1000U)
    {
        last_send = now;

        int len = sprintf(ccd_text, "[");
        for (int i = 0; i < 128; i++)
        {
            len += sprintf(ccd_text + len, "%u%s",
                           (unsigned int)CCD_Value[i],
                           (i == 127) ? " ]\r\n" : ", ");
        }
        HAL_UART_Transmit(mx_usart2_uart_gethandle(),
                          ccd_text,
                          (uint32_t)len,
                          100U);
    }
}

