#include "mlx90614.h"

#define MLX90614_ADDR_7BIT  0x5A
#define MLX90614_ADDR       (MLX90614_ADDR_7BIT << 1)

#define MLX90614_RAM_TA     0x06
#define MLX90614_RAM_TO1    0x07

static I2C_HandleTypeDef *mlx_i2c = NULL;

static float word_to_temp(uint16_t w)
{
    return (float)w * 0.02f - 273.15f;
}

HAL_StatusTypeDef MLX90614_Init(I2C_HandleTypeDef *hi2c)
{
    mlx_i2c = hi2c;

    return HAL_I2C_IsDeviceReady(mlx_i2c, MLX90614_ADDR, 3, 100);
}

HAL_StatusTypeDef MLX90614_ReadTaTo(float *Ta, float *To)
{
    if (mlx_i2c == NULL || Ta == NULL || To == NULL)
        return HAL_ERROR;

    uint8_t rx[3];
    uint16_t raw;
    HAL_StatusTypeDef st;

    st = HAL_I2C_Mem_Read(mlx_i2c, MLX90614_ADDR, MLX90614_RAM_TA,
                          I2C_MEMADD_SIZE_8BIT, rx, 3, 100);
    if (st != HAL_OK) return st;

    raw = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    *Ta = word_to_temp(raw);

    st = HAL_I2C_Mem_Read(mlx_i2c, MLX90614_ADDR, MLX90614_RAM_TO1,
                          I2C_MEMADD_SIZE_8BIT, rx, 3, 100);
    if (st != HAL_OK) return st;

    raw = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
    *To = word_to_temp(raw);

    return HAL_OK;
}
