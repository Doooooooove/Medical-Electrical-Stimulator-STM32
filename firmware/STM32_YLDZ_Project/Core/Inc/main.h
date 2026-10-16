/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern unsigned int codetest;

typedef enum {
    MODE_NMES_SWALLOW = 0, // 基(1): 医疗吞咽刺激波 (NMES 梯形渐增渐降包络脉冲串)
    MODE_CONSTANT_I   = 1, // 基(2): 恒流输出 (0~10mA 连续可调, 步进1mA, 2kΩ负载, 精度±5%)
    MODE_EXPONENTIAL  = 2, // 基(3): 指数衰减波 (时间常数tau可调, 频率可调)
    MODE_BIPHASIC_DC  = 3  // 发(1): 带直偏的对称双向方波 (频率可调, 脉宽可调)
} Stim_Mode_TypeDef;

typedef struct {
    Stim_Mode_TypeDef mode;      // 当前模式
    float current_mA;            // 设定峰值/目标电流 (0.0 ~ 10.0 mA, 步进 1.0 mA)
    uint16_t freq_Hz;            // 频率 (Hz), 1 ~ 200 Hz
    uint16_t pulse_width_us;     // 脉宽 (us), 100 ~ 1000 us (0.1ms ~ 1.0ms)
    float tau_ms;                // 指数时间常数 (ms)
    float dc_offset_mA;          // 直流偏置电流 (mA)
    float measured_impedance;    // 自动计算的阻抗 (Ω)
    uint8_t is_running;          // 启停控制: 1=输出中, 0=停止
    uint32_t countdown_s;        // 治疗倒计时 (秒)
} Stim_Config_TypeDef;

extern Stim_Config_TypeDef g_stim;
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
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
