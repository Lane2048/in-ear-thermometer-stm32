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
  * Temp measument + ble advertising powered by transistor circuit at run mode
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mlx90614.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    MODE_ARMED = 0,     // 等待上升沿唤醒
    MODE_ACTIVE,        // 工作窗口 30s
    MODE_WAIT_FALL      // 等待下降沿“释放”，再重新武装
} app_mode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

volatile app_mode_t g_mode = MODE_ARMED;
volatile uint8_t g_exti_event = 0;   // 1 表示收到 EXTI 事件
volatile uint16_t g_exti_pin = 0;    // 记录触发的 pin（可选）

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
static void EXTI_SetEdge_Rising(uint16_t pin);
static void EXTI_SetEdge_Falling(uint16_t pin);
static void EnterStop(void);
static void ActiveWindow_30s(void);
static void PowerSwitch_ForceLow(void);
static void BLE_UartPins_HiZ(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Redirect printf() output to USART2 (Putty).
 *
 * This allows using printf(...) for debug logs without additional wrapper
 * functions. The BLE string is sent separately using HAL_UART_Transmit on huart1.
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // 初始：等待上升沿唤醒
  g_mode = MODE_ARMED;
  EXTI_SetEdge_Falling(WAKE_Pin);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

//	if (g_mode == MODE_ARMED)
//	{
//	  // 进入 STOP，等待 RISING 唤醒
//	  EnterStop();
//
//	  // 唤醒后，如果是 EXTI 事件，则进入 ACTIVE
//	  if (g_exti_event) {
//		  g_exti_event = 0;
//		  g_mode = MODE_ACTIVE;
//	  }
//	}
//
//	else if (g_mode == MODE_ACTIVE)
//	{
//	  // 工作窗口 30s
//	  ActiveWindow_30s();
//
//	  // 30s 后不立刻武装 RISING，而是先等 FALLING（释放）
//	  g_mode = MODE_WAIT_FALL;
//	  EXTI_SetEdge_Falling(WAKE_Pin);
//	}
//
//	else if (g_mode == MODE_WAIT_FALL)
//	{
//	  // 进入 STOP，等待 FALLING
//	  EnterStop();
//
//	  if (g_exti_event) {
//		  g_exti_event = 0;
//
//		  // 收到 FALLING：说明外部信号回到低电平了
//		  // 重新武装 RISING，回到 ARMED
//		  EXTI_SetEdge_Rising(WAKE_Pin);
//		  g_mode = MODE_ARMED;
//	  }
//	}

	  EnterStop();   // 等 falling 唤醒

	  if (g_exti_event) {
		  g_exti_event = 0;

		  // 再确认一次当前电平确实为低，防止毛刺误触发
		  if (HAL_GPIO_ReadPin(WAKE_GPIO_Port, WAKE_Pin) == GPIO_PIN_RESET)
		  {
			  // 可选：简单消抖
			  HAL_Delay(10);
			  if (HAL_GPIO_ReadPin(WAKE_GPIO_Port, WAKE_Pin) == GPIO_PIN_RESET)
			  {
				  ActiveWindow_30s();
			  }
		  }
	  }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 只记录事件，避免在中断里printf / I2C / UART
    g_exti_event = 1;
    g_exti_pin = GPIO_Pin;
}

static void EXTI_SetEdge_Rising(uint16_t pin)
{
    uint32_t line = 0;
    // pin 是 GPIO_PIN_0..GPIO_PIN_15 这种位掩码
    for (int i = 0; i < 16; i++) {
        if (pin == (1U << i)) { line = i; break; }
    }

    // 关掉 falling，打开 rising
    EXTI->FTSR &= ~(1U << line);
    EXTI->RTSR |=  (1U << line);

    // 清 pending
    EXTI->PR = (1U << line);
}

static void EXTI_SetEdge_Falling(uint16_t pin)
{
    uint32_t line = 0;
    for (int i = 0; i < 16; i++) {
        if (pin == (1U << i)) { line = i; break; }
    }

    EXTI->RTSR &= ~(1U << line);
    EXTI->FTSR |=  (1U << line);

    EXTI->PR = (1U << line);
}

static void EnterStop(void)
{
	PowerSwitch_ForceLow();

    // 关键：避免通过USART1脚给BLE反向供电
    BLE_UartPins_HiZ();

    // 可选：避免 SysTick 在 STOP 前后乱触发
    HAL_SuspendTick();

    // 清 EXTI 标志
    g_exti_event = 0;

    /* 4) 清 EXTI pending：关键！防止旧的pending导致“秒醒” */
    __HAL_GPIO_EXTI_CLEAR_IT(WAKE_Pin);

    /* 5) 清 PWR Wakeup flag（有些情况下也会导致立刻返回） */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* 6) Data Synchronization Barrier：确保上面的寄存器写入生效后再睡 */
    __DSB();
    __ISB();

    // 进入 STOP，WFI 等中断唤醒
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    // 唤醒后继续执行到这里
    HAL_ResumeTick();

    // STOP 唤醒后，必须恢复系统时钟
    SystemClock_Config();

    // 唤醒后如果你还要用USART1，需要重新Init或至少重新使能并恢复AF模式
    // 最稳妥做法：MX_USART1_UART_Init(); 以及把PA9/PA10恢复AF
    MX_USART1_UART_Init();
}

static void ActiveWindow_30s(void)
{
    // 上电下游
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_SET);
    HAL_Delay(200);

    printf("MLX90614 bring-up\r\n");
    if (MLX90614_Init(&hi2c1) != HAL_OK) {
        printf("MLX90614 not ready\r\n");
        // 这里你可以选择直接退出窗口，或者继续尝试
    } else {
        printf("MLX90614 ready\r\n");
    }

    uint32_t t0 = HAL_GetTick();

    while ((HAL_GetTick() - t0) < 10000U) {
        float Ta, To;

        if (MLX90614_ReadTaTo(&Ta, &To) == HAL_OK) {
            printf("Ta=%.2fC  To=%.2fC\r\n", Ta, To);

            char buf[64];
            snprintf(buf, sizeof(buf), "Ta=%.2f, To=%.2f\r\n", Ta, To);
            HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);
        } else {
            printf("Read ERR\r\n");
        }

        HAL_Delay(500);
    }

    // 关下游
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);
}

static void PowerSwitch_ForceLow(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 先写0，避免glitch
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);

    // 再强制配置成推挽输出
    GPIO_InitStruct.Pin = POWERSWITCH_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 再写一次0，确保生效
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);
}

static void BLE_UartPins_HiZ(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 假设 USART1 TX=PA9, RX=PA10（你要按实际改）
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 先关闭USART1，避免外设继续驱动引脚
    __HAL_UART_DISABLE(&huart1);

    // 把TX/RX设为模拟输入(最低漏电，且高阻)
    GPIO_InitStruct.Pin  = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


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
