/**
 * @file mlx90614.h
 * @brief MLX90614 ambient/object temperature readout interface.
 */
#ifndef MLX90614_H
#define MLX90614_H

#include "stm32l0xx_hal.h"
#include <stdint.h>

/**
 * @brief Bind an initialized I2C handle and check sensor readiness.
 * @param hi2c I2C bus connected to the sensor; the application uses &hi2c1.
 * @return Status returned by HAL_I2C_IsDeviceReady().
 */
HAL_StatusTypeDef MLX90614_Init(I2C_HandleTypeDef *hi2c);
/**
 * @brief Read ambient and object channel 1 temperatures in degrees Celsius.
 * @param Ta Output pointer for the ambient temperature.
 * @param To Output pointer for the object temperature.
 * @return HAL_OK when both I2C reads succeed, otherwise a HAL error status.
 * @note Call MLX90614_Init() first. Consume both outputs only on HAL_OK.
 */
HAL_StatusTypeDef MLX90614_ReadTaTo(float *Ta, float *To);

#endif
