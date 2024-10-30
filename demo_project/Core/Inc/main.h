/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"

#include "app_conf.h"
#include "app_entry.h"
#include "app_common.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SEN_INT2_Pin GPIO_PIN_13
#define SEN_INT2_GPIO_Port GPIOC
#define SEN_INT2_EXTI_IRQn EXTI15_10_IRQn
#define SPI_SEL2_Pin GPIO_PIN_0
#define SPI_SEL2_GPIO_Port GPIOC
#define SPI_SEL1_Pin GPIO_PIN_1
#define SPI_SEL1_GPIO_Port GPIOC
#define SEN_EN1_Pin GPIO_PIN_0
#define SEN_EN1_GPIO_Port GPIOA
#define SPI_SEL0_Pin GPIO_PIN_1
#define SPI_SEL0_GPIO_Port GPIOA
#define A121_SPI_SS_Pin GPIO_PIN_4
#define A121_SPI_SS_GPIO_Port GPIOA
#define SEN_EN2_Pin GPIO_PIN_10
#define SEN_EN2_GPIO_Port GPIOC
#define SEN_INT1_Pin GPIO_PIN_12
#define SEN_INT1_GPIO_Port GPIOC
#define SEN_INT1_EXTI_IRQn EXTI15_10_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
