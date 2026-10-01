/*
 * ble_uart.h
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
 * @brief 初始化 BLE UART 模块
 * @param ble_huart   BLE 所在 UART（建议 USART1 -> &huart1）
 * @param dbg_huart   调试输出 UART（Putty，建议 USART2 -> &huart2），不需要可传 NULL
 */
void BLE_UART_Init(UART_HandleTypeDef *ble_huart, UART_HandleTypeDef *dbg_huart);

/**
 * @brief 开始 1-byte 中断接收（用于调试或命令）
 */
void BLE_UART_StartRxIT(void);

/**
 * @brief 发送原始字节（blocking）
 */
HAL_StatusTypeDef BLE_UART_Send(const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief 发送字符串（blocking）
 */
HAL_StatusTypeDef BLE_UART_SendString(const char *s, uint32_t timeout_ms);

/**
 * @brief 可选：把温度打包成 4 个十进制字符（例如 38.27 -> "3827"）并发送
 */
HAL_StatusTypeDef BLE_UART_SendTemp_x100(float temp_c, uint32_t timeout_ms);

/**
 * @brief 必须从全局 HAL_UART_RxCpltCallback() 里转发进来
 */
void BLE_UART_OnRxCplt(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif // BLE_UART_Hs
