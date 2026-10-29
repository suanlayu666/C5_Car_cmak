/**
  ******************************************************************************
  * @file           : mx_gpio_default.h
  * @brief          : Header for mx_gpio_default.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_GPIO_DEFAULT_H
#define MX_GPIO_DEFAULT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"
#include "mx_def.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/* Exported defines for gpio_default in HAL layer                             */
/******************************************************************************/

/* Primary aliases for GPIO PA5 pin */
#define CCD_CLK_PORT                                    HAL_GPIOA
#define CCD_CLK_PIN                                     HAL_GPIO_PIN_5
#define CCD_CLK_INIT_STATE                              HAL_GPIO_PIN_RESET
#define CCD_CLK_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define CCD_CLK_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA6 pin */
#define CCD_SI_PORT                                     HAL_GPIOA
#define CCD_SI_PIN                                      HAL_GPIO_PIN_6
#define CCD_SI_INIT_STATE                               HAL_GPIO_PIN_RESET
#define CCD_SI_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define CCD_SI_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB0 pin */
#define MOTOR_L_IN1_PORT                                HAL_GPIOB
#define MOTOR_L_IN1_PIN                                 HAL_GPIO_PIN_0
#define MOTOR_L_IN1_INIT_STATE                          HAL_GPIO_PIN_RESET
#define MOTOR_L_IN1_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define MOTOR_L_IN1_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB1 pin */
#define MOTOR_L_IN2_PORT                                HAL_GPIOB
#define MOTOR_L_IN2_PIN                                 HAL_GPIO_PIN_1
#define MOTOR_L_IN2_INIT_STATE                          HAL_GPIO_PIN_RESET
#define MOTOR_L_IN2_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define MOTOR_L_IN2_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB5 pin */
#define MOTOR_R_IN1_PORT                                HAL_GPIOB
#define MOTOR_R_IN1_PIN                                 HAL_GPIO_PIN_5
#define MOTOR_R_IN1_INIT_STATE                          HAL_GPIO_PIN_RESET
#define MOTOR_R_IN1_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define MOTOR_R_IN1_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB6 pin */
#define MOTOR_R_IN2_PORT                                HAL_GPIOB
#define MOTOR_R_IN2_PIN                                 HAL_GPIO_PIN_6
#define MOTOR_R_IN2_INIT_STATE                          HAL_GPIO_PIN_RESET
#define MOTOR_R_IN2_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define MOTOR_R_IN2_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB7 pin */
#define MOTOR_STBY_PORT                                 HAL_GPIOB
#define MOTOR_STBY_PIN                                  HAL_GPIO_PIN_7
#define MOTOR_STBY_INIT_STATE                           HAL_GPIO_PIN_RESET
#define MOTOR_STBY_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define MOTOR_STBY_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE2 pin */
#define STATUS_LED_PORT                                 HAL_GPIOE
#define STATUS_LED_PIN                                  HAL_GPIO_PIN_2
#define STATUS_LED_INIT_STATE                           HAL_GPIO_PIN_RESET
#define STATUS_LED_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define STATUS_LED_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for gpio_default in HAL layer                           */
/******************************************************************************/
/**
  * @brief mx_gpio_default init function
  * This function configures the hardware resources used in this example
  * @retval 0  GPIO group correctly initialized
  * @retval -1 Issue detected during GPIO group initialization
  */
system_status_t mx_gpio_default_init(void);

/**
  * @brief  De-initialize gpio_default instance.
  */
system_status_t mx_gpio_default_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
