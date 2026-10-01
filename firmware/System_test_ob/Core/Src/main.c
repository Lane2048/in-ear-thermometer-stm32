/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Event-triggered infrared temperature measurement.
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
  * Falling-edge wake starts a temperature-measurement window.
  * PA4 controls the external peripheral supply; USART2 carries debug logs
  * and USART1 carries temperature text for an external BLE module.
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
// State labels retained from the disabled legacy state machine below.
typedef enum {
    MODE_ARMED = 0,     // Legacy state: wait for a rising-edge trigger.
    MODE_ACTIVE,        // Legacy active state; the current measurement loop uses 10 s.
    MODE_WAIT_FALL      // Legacy state: wait for a falling edge before rearming.
} app_mode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

volatile app_mode_t g_mode = MODE_ARMED;
volatile uint8_t g_exti_event = 0;   // Set by the EXTI callback when an interrupt occurs.
volatile uint16_t g_exti_pin = 0;    // Last triggering GPIO pin mask, retained for diagnostics.

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
 * @brief Redirect printf() output to the USART2 debug terminal.
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
  // Select falling-edge wake; g_mode is retained for the legacy state machine.
  g_mode = MODE_ARMED;
  EXTI_SetEdge_Falling(WAKE_Pin);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

// Disabled legacy control flow. The active loop below uses falling-edge wake.
//	if (g_mode == MODE_ARMED)
//	{
//	  // Enter STOP and wait for a rising-edge interrupt.
//	  EnterStop();
//
//	  // After an EXTI wake event, enter the legacy active state.
//	  if (g_exti_event) {
//		  g_exti_event = 0;
//		  g_mode = MODE_ACTIVE;
//	  }
//	}
//
//	else if (g_mode == MODE_ACTIVE)
//	{
//	  // Run the measurement helper; its current timeout is 10 s.
//	  ActiveWindow_30s();
//
//	  // After measurement, wait for the falling-edge release before rearming.
//	  g_mode = MODE_WAIT_FALL;
//	  EXTI_SetEdge_Falling(WAKE_Pin);
//	}
//
//	else if (g_mode == MODE_WAIT_FALL)
//	{
//	  // Enter STOP and wait for a falling-edge interrupt.
//	  EnterStop();
//
//	  if (g_exti_event) {
//		  g_exti_event = 0;
//
//		  // A falling edge indicates that the external signal returned low.
//		  // Rearm the rising edge and return to the legacy armed state.
//		  EXTI_SetEdge_Rising(WAKE_Pin);
//		  g_mode = MODE_ARMED;
//	  }
//	}

	  EnterStop();   // Enter STOP and wait for a falling-edge wake event.

	  if (g_exti_event) {
		  g_exti_event = 0;

		  // Accept the wake event only if PA0 is still low.
		  if (HAL_GPIO_ReadPin(WAKE_GPIO_Port, WAKE_Pin) == GPIO_PIN_RESET)
		  {
			  // Debounce: wait 10 ms, then confirm that PA0 remains low.
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
  * @brief Select the 16 MHz HSI clock for the CPU and configured peripherals.
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
/**
 * @brief Record an EXTI event for processing in the main loop.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Keep this callback short; sensor reads and UART output run in the main loop.
    g_exti_event = 1;
    g_exti_pin = GPIO_Pin;
}

/**
 * @brief Select rising-edge EXTI for one GPIO pin (legacy helper).
 */
static void EXTI_SetEdge_Rising(uint16_t pin)
{
    uint32_t line = 0;
    // Convert the single GPIO_PIN_x bit mask to its EXTI line index.
    for (int i = 0; i < 16; i++) {
        if (pin == (1U << i)) { line = i; break; }
    }

    // Disable falling-edge detection and enable rising-edge detection.
    EXTI->FTSR &= ~(1U << line);
    EXTI->RTSR |=  (1U << line);

    // Clear a pending interrupt by writing one to its bit.
    EXTI->PR = (1U << line);
}

/**
 * @brief Select falling-edge EXTI for the active wake input.
 */
static void EXTI_SetEdge_Falling(uint16_t pin)
{
    uint32_t line = 0;
    for (int i = 0; i < 16; i++) {
        if (pin == (1U << i)) { line = i; break; }
    }

    // Disable rising-edge detection and enable falling-edge detection.
    EXTI->RTSR &= ~(1U << line);
    EXTI->FTSR |=  (1U << line);

    // Clear a pending interrupt by writing one to its bit.
    EXTI->PR = (1U << line);
}

/**
 * @brief Switch off the peripheral supply and wait in low-power STOP mode.
 * Execution continues after an interrupt; the clock is then restored.
 */
static void EnterStop(void)
{
	PowerSwitch_ForceLow();

    // Release the UART pins to reduce back-powering of the unpowered BLE module.
    BLE_UartPins_HiZ();

    // Suspend periodic tick interrupts around STOP entry and wake-up.
    HAL_SuspendTick();

    // Clear the software event flag before arming the next sleep.
    g_exti_event = 0;

    /* Clear a stale EXTI pending bit that could cause an immediate wake-up. */
    __HAL_GPIO_EXTI_CLEAR_IT(WAKE_Pin);

    /* Clear the power wake-up flag before entering STOP. */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Complete register writes and synchronize execution before sleeping. */
    __DSB();
    __ISB();

    // Enter STOP with the low-power regulator and wait for an interrupt.
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    // Execution resumes here after wake-up.
    HAL_ResumeTick();

    // Restore the configured system clock after STOP.
    SystemClock_Config();

    // Reinitialize USART1. GPIO restoration still needs review: HAL_UART_Init()
    // only reruns MSP pin setup when the UART handle is in the RESET state.
    MX_USART1_UART_Init();
}

/**
 * @brief Power the peripherals, read temperatures, and submit UART telemetry.
 * @note The legacy name says 30 s; the loop currently uses a 10000 ms timeout.
 * The settling delay and sensor readiness check occur before that timeout.
 */
static void ActiveWindow_30s(void)
{
    // Enable the external sensor/BLE supply and allow 200 ms to settle.
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_SET);
    HAL_Delay(200);

    printf("MLX90614 bring-up\r\n");
    if (MLX90614_Init(&hi2c1) != HAL_OK) {
        printf("MLX90614 not ready\r\n");
        // Continue with timed read attempts even if the readiness probe fails.
    } else {
        printf("MLX90614 ready\r\n");
    }

    // Start the measurement timer after peripheral power-up and initialization.
    uint32_t t0 = HAL_GetTick();

    while ((HAL_GetTick() - t0) < 10000U) {
        float Ta, To;

        if (MLX90614_ReadTaTo(&Ta, &To) == HAL_OK) {
            printf("Ta=%.2fC  To=%.2fC\r\n", Ta, To);

            // Format both temperatures as an ASCII line for the BLE module UART.
            char buf[64];
            snprintf(buf, sizeof(buf), "Ta=%.2f, To=%.2f\r\n", Ta, To);
            HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);
        } else {
            printf("Read ERR\r\n");
        }

        // Space read attempts by 500 ms; I2C and UART operations add overhead.
        HAL_Delay(500);
    }

    // Disable the downstream supply at the end of the measurement window.
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Hold the external peripheral power-switch control low before STOP.
 */
static void PowerSwitch_ForceLow(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Set the output latch low before configuring the GPIO to avoid a high pulse.
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);

    // Keep PA4 actively driven low as a push-pull output.
    GPIO_InitStruct.Pin = POWERSWITCH_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Reassert the low level after GPIO initialization.
    HAL_GPIO_WritePin(GPIOA, POWERSWITCH_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Disable USART1 and release PA9/PA10 while the BLE supply is off.
 */
static void BLE_UartPins_HiZ(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // This project maps USART1 TX/RX to PA9/PA10.
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Disable USART1 before changing the pins away from their alternate function.
    __HAL_UART_DISABLE(&huart1);

    // Use analog mode with no pulls to release the pins and reduce leakage.
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
