/**
 * @file Mlx90614.c
 * @brief Read MLX90614 ambient/object registers through the STM32 HAL I2C API.
 */
#include "mlx90614.h"

// Default 7-bit sensor address; HAL expects it shifted left by one bit.
#define MLX90614_ADDR_7BIT  0x5A
#define MLX90614_ADDR       (MLX90614_ADDR_7BIT << 1)

// RAM registers for ambient temperature (Ta) and object channel 1 (To).
#define MLX90614_RAM_TA     0x06
#define MLX90614_RAM_TO1    0x07

static I2C_HandleTypeDef *mlx_i2c = NULL;

// Convert the sensor word from 0.02 K units to degrees Celsius.
static float word_to_temp(uint16_t w)
{
    return (float)w * 0.02f - 273.15f;
}

// Bind the I2C bus and probe readiness (up to three attempts, 100 ms timeout).
HAL_StatusTypeDef MLX90614_Init(I2C_HandleTypeDef *hi2c)
{
    mlx_i2c = hi2c;

    return HAL_I2C_IsDeviceReady(mlx_i2c, MLX90614_ADDR, 3, 100);
}

HAL_StatusTypeDef MLX90614_ReadTaTo(float *Ta, float *To)
{
    if (mlx_i2c == NULL || Ta == NULL || To == NULL)
        return HAL_ERROR;

    // Read two data bytes followed by PEC. This driver does not validate PEC
    // or the error flag in the temperature word.
    uint8_t rx[3];
    uint16_t raw;
    HAL_StatusTypeDef st;

    // Read ambient temperature first; bytes are returned least-significant first.
    st = HAL_I2C_Mem_Read(mlx_i2c, MLX90614_ADDR, MLX90614_RAM_TA,
                          I2C_MEMADD_SIZE_8BIT, rx, 3, 100);
    if (st != HAL_OK) return st;

    raw = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    *Ta = word_to_temp(raw);

    // Read object channel 1 using the same word-to-temperature conversion.
    st = HAL_I2C_Mem_Read(mlx_i2c, MLX90614_ADDR, MLX90614_RAM_TO1,
                          I2C_MEMADD_SIZE_8BIT, rx, 3, 100);
    if (st != HAL_OK) return st;

    raw = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    *To = word_to_temp(raw);

    return HAL_OK;
}
