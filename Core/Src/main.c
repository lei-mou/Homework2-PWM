/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "stm32f1xx_hal_gpio.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "oled.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t led_status = 3;
uint8_t key_status = 0;
uint8_t hold_count = 0;
uint8_t shake_count = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim3) {
    uint8_t lvl = HAL_GPIO_ReadPin(key_GPIO_Port, key_Pin);
    switch (key_status) {
      case 0: // 按下
        if (lvl == GPIO_PIN_RESET) {
          key_status  = 1;
          shake_count = hold_count = 0;
        }
        break;

      case 1: // 按下消抖
        if (lvl == GPIO_PIN_RESET) {
          if (++shake_count >= 2) key_status = 2;
        } else {
          key_status = 0;
        }
        break;

      case 2: // 按住
        if (lvl == GPIO_PIN_SET) {
          key_status  = 3;
          shake_count = 0;
        } else if (hold_count <= 101) {
          hold_count++;
        }
        break;

      case 3: // 松开消抖
        if (lvl == GPIO_PIN_SET) {
          if (++shake_count >= 2) {
            if (hold_count >= 101) led_status = (led_status == 5) ? 0 : 5;
            else if (led_status != 5) led_status = (led_status + 1) % 5;
            key_status = 0;
          }
        } else {
          key_status  = 2;
          shake_count = 0;
        }
        break;
    }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(50);
  OLED_Init();

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim3);

  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  
  int value = 0;
  float sevro_duty = 0;
  float Voltage = 0;
  int int_part = 0;
  int dec_part = 0;
  int light_duty = 0;
  uint32_t breath_time = 0;
  uint32_t screem_time = 0;
  uint8_t led_direction = 1;
  char message1[20] = "";
  char message2[20] = "";
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    //获取ADC读数和计算
    value = HAL_ADC_GetValue(&hadc1);
    sevro_duty = (((value / 4095.0)*10 + 2.5) / 100.0) * 2000;
    Voltage = 3.3 * (value / 4095.0);
    int_part = (int)Voltage;
    dec_part = (int)((Voltage - int_part) * 10);
    //操控舵机
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, sevro_duty);

    //呼吸灯部分
    if ( led_status == 0 ){
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    }else if (led_status == 1) {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 250);
    }else if (led_status == 2) {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 500);
    }else if (led_status == 3) {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 750);
    }else if (led_status == 4) {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 999);
    }else if (led_status == 5) {
      if ( HAL_GetTick() - breath_time >= 1)
        {
          breath_time = HAL_GetTick();
          if (led_direction == 1){
            if (light_duty >= 999){
              led_direction = !led_direction;
            }else {
              light_duty ++;
            }
          }else {
            if (light_duty <= 0){
              led_direction = !led_direction;
            }else {
              light_duty --;
            }
          }
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, light_duty);
        }
    }
    
    //OLED操控
    if ( HAL_GetTick() - screem_time >= 60)
    {
      screem_time = HAL_GetTick();
      sprintf(message1, "value: %d", value);
      sprintf(message2, "U: %d.%01dV", int_part, dec_part);
    
      OLED_NewFrame();
      OLED_PrintString(10, 0, message1, &font16x16, OLED_COLOR_NORMAL);
      OLED_PrintString(10, 17, message2, &font16x16, OLED_COLOR_NORMAL);
      OLED_DrawRectangle(10, 33, 101, 12, OLED_COLOR_NORMAL);
      OLED_DrawFilledRectangle(10, 34, (int)(value / 4095.0 * 100), 12, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
