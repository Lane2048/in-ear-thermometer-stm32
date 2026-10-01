/**
 * @file ble_uart.c
 * @brief Optional blocking-transmit and interrupt-receive UART helpers.
 * The current main loop uses HAL_UART_Transmit() directly instead.
 */
#include "ble_uart.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *s_ble = NULL;
static UART_HandleTypeDef *s_dbg = NULL;

// One-byte receive buffer, reused when reception is rearmed in the callback.
static uint8_t s_rx_byte;

static void dbg_write(const char *s)
{
    if (s_dbg == NULL) return;
    HAL_UART_Transmit(s_dbg, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

void BLE_UART_Init(UART_HandleTypeDef *ble_huart, UART_HandleTypeDef *dbg_huart)
{
    s_ble = ble_huart;
    s_dbg = dbg_huart;

    // Reception starts only when the application calls BLE_UART_StartRxIT().
    dbg_write("[BLE_UART] init done\r\n");
}

void BLE_UART_StartRxIT(void)
{
    if (s_ble == NULL) return;
    HAL_UART_Receive_IT(s_ble, &s_rx_byte, 1);
    dbg_write("[BLE_UART] RX IT started\r\n");
}

HAL_StatusTypeDef BLE_UART_Send(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (s_ble == NULL) return HAL_ERROR;
    return HAL_UART_Transmit(s_ble, (uint8_t*)data, (uint16_t)len, timeout_ms);
}

HAL_StatusTypeDef BLE_UART_SendString(const char *s, uint32_t timeout_ms)
{
    return BLE_UART_Send((const uint8_t*)s, strlen(s), timeout_ms);
}

HAL_StatusTypeDef BLE_UART_SendTemp_x100(float temp_c, uint32_t timeout_ms)
{
    // Scale by 100 and round, e.g. 38.27 C becomes the four-byte string "3827".
    int t100 = (int)(temp_c * 100.0f + 0.5f);

    char payload[5]; // Four payload characters plus the string terminator.
    snprintf(payload, sizeof(payload), "%04d", t100);

    return BLE_UART_Send((const uint8_t*)payload, 4, timeout_ms);
}

void BLE_UART_OnRxCplt(UART_HandleTypeDef *huart)
{
    if (s_ble == NULL) return;
    if (huart != s_ble) return;

    // Echo the received BLE byte to the optional debug UART.
    if (s_dbg)
    {
        const char tag[] = "[BLE_RX] ";
        HAL_UART_Transmit(s_dbg, (uint8_t*)tag, sizeof(tag)-1, 10);
        HAL_UART_Transmit(s_dbg, &s_rx_byte, 1, 10);
        HAL_UART_Transmit(s_dbg, (uint8_t*)"\r\n", 2, 10);
    }

    // Rearm the one-byte interrupt receive operation for the next byte.
    HAL_UART_Receive_IT(s_ble, &s_rx_byte, 1);
}
