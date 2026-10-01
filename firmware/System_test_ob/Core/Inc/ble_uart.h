/*
 * ble_uart.h
 *
 * Optional UART helpers; these are not called by the current main loop.
 *
 *  Created on: 8 Jan 2026
 *      Author: Lane
 */

#ifndef INC_BLE_UART_H_
#define INC_BLE_UART_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l0xx_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Bind already-initialized UART handles to the BLE helpers.
 * @param ble_huart BLE module UART, typically USART1 (&huart1).
 * @param dbg_huart Optional debug UART, typically USART2 (&huart2); NULL disables it.
 * @note This stores the handles; it does not configure the UARTs or BLE module.
 */
void BLE_UART_Init(UART_HandleTypeDef *ble_huart, UART_HandleTypeDef *dbg_huart);

/**
 * @brief Start receiving one byte using a UART interrupt.
 * @note Call after BLE_UART_Init(). Enable the UART interrupt and forward the
 * HAL receive-complete callback to BLE_UART_OnRxCplt() to keep reception running.
 */
void BLE_UART_StartRxIT(void);

/**
 * @brief Send raw bytes using a blocking HAL UART transfer.
 * @param data Buffer containing the bytes to send.
 * @param len Number of bytes, passed to HAL as a uint16_t.
 * @param timeout_ms HAL transmit timeout in milliseconds.
 * @return HAL transmit status, or HAL_ERROR if no BLE UART handle is bound.
 */
HAL_StatusTypeDef BLE_UART_Send(const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief Send a null-terminated string using a blocking transfer.
 * @note The terminating null byte is not transmitted.
 */
HAL_StatusTypeDef BLE_UART_SendString(const char *s, uint32_t timeout_ms);

/**
 * @brief Scale a temperature by 100 and send four decimal characters.
 * For example, 38.27 C is transmitted as "3827" without a decimal point.
 * @note Intended for non-negative values that round into 0000..9999.
 * No range validation is performed; the current main loop uses another format.
 */
HAL_StatusTypeDef BLE_UART_SendTemp_x100(float temp_c, uint32_t timeout_ms);

/**
 * @brief Handle a completed one-byte receive operation and rearm reception.
 * @param huart UART handle supplied by HAL_UART_RxCpltCallback().
 * @note The application must forward that callback here. Bytes may be echoed
 * to the optional debug UART; this helper does not parse commands.
 */
void BLE_UART_OnRxCplt(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif // INC_BLE_UART_H_
