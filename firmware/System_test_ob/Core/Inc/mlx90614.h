#ifndef MLX90614_H
#define MLX90614_H

#include "stm32l0xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef MLX90614_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MLX90614_ReadTaTo(float *Ta, float *To);

#endif
