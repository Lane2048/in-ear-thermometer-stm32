# Firmware guide

## Execution path

The active application is a bare-metal main loop in `Core/Src/main.c`. It uses HAL calls and an EXTI event flag; it does not run an RTOS.

### 1. Initialize

`HAL_Init()` and `SystemClock_Config()` initialize the MCU with the **16 MHz HSI clock**, without a PLL. GPIO, USART2, I2C1, and USART1 are then initialized. Although the generated GPIO configuration enables both EXTI edges, `main()` explicitly selects **falling-edge** wake behavior.

### 2. Enter STOP

`EnterStop()`:

- Forces PA4 low through `PowerSwitch_ForceLow()`.
- Disables USART1 and places PA9/PA10 in analog mode without pulls through `BLE_UartPins_HiZ()`, intended to reduce leakage/back-powering through the UART connection.
- Suspends SysTick, clears EXTI and power wake flags, and executes STOP entry with the low-power regulator and `WFI`.
- Resumes SysTick after wake, restores the system clock, and calls `MX_USART1_UART_Init()`.

The USART1 GPIO-restoration caveat below applies to this final step. Debug hardware, the external circuits, and the I2C power/pull-up arrangement all affect the measured idle current.

### 3. Validate the trigger

`HAL_GPIO_EXTI_Callback()` sets `g_exti_event` and records the triggering pin. Sensor and UART work is performed in the main loop, outside the interrupt callback.

After wake, the loop checks PA0 low, delays **10 ms**, and checks it again. A valid event calls `ActiveWindow_30s()`. Another measurement needs a new falling edge, so the external signal must first return high.

### 4. Measure and transmit

The active routine sets PA4 high, waits **200 ms**, and calls `MLX90614_Init(&hi2c1)`. The approximately **10 s** loop begins after this initialization attempt.

For each successful `MLX90614_ReadTaTo()` call:

- Ambient (`Ta`) and object (`To`) readings are obtained from registers `0x06` and `0x07`.
- The first two received bytes form a little-endian word, converted using `raw * 0.02f - 273.15f`.
- A readable line is printed on USART2.
- An ASCII line is submitted to USART1 with `HAL_UART_Transmit()`.

Each iteration ends with a **500 ms delay**. Sensor transactions and UART output add time, so this is a nominal cadence rather than a precise 2 Hz scheduler. The downstream supply is switched off at the end.

## Serial output

Both UARTs use **115200 baud, 8N1, no flow control**. Example values below illustrate the format; they are not captured test data.

Debug output, USART2:

```text
MLX90614 bring-up
MLX90614 ready
Ta=24.50C  To=36.80C
```

Telemetry submitted to USART1:

```text
Ta=24.50, To=36.80
```

Lines terminate with `\r\n`. Initialization failures print `MLX90614 not ready`; unsuccessful reads print `Read ERR`. After an initialization failure, the code still enters the timed read loop and attempts reads.

`ble_uart.c` provides separate send/receive helpers, including a four-character temperature format, but the active main loop does **not** initialize or call them. The current payload is the two-temperature ASCII line above. BLE receive helpers and a command interface are not wired into the application.

## Import and build notes

- Import the existing Eclipse/STM32CubeIDE project from `firmware/System_test_ob`. Keep its internal project name **`System_test_ob`**, which is referenced by the build configuration.
- The recorded generator versions are **STM32CubeMX 6.16.0** and **STM32Cube firmware package L0 V1.12.3**. These describe the supplied configuration; the original IDE executable version is not recorded here.
- HAL/CMSIS dependencies and their licenses are included. Generated `Debug/` and `Release/` output is excluded; STM32CubeIDE regenerates build files when building the project.
- Use the **Debug** configuration for the initial build. It includes **`-u _printf_float`** for floating-point formatting. The supplied Release configuration does not carry the equivalent option, so enable floating-point `printf` support if using Release.
- The sensor header is named **`mlx90614.h`** to match the existing includes on case-sensitive filesystems. Its contents are unchanged from the uploaded `Mlx90614.h`.
- Create a local ST-LINK launch configuration. The original machine-specific `.launch` file is excluded.
- Import and build the supplied project before regenerating from `.ioc`. Code generation can change peripheral and build settings; compare any regenerated files before replacing this snapshot.

For the IDE import/build workflow, see [ST's STM32CubeIDE user guide, UM2609](https://www.st.com/resource/en/user_manual/dm00629856-description-of-the-integrated-development-environment-for-stm32-products-stmicroelectronics.pdf).

## Known limitations

These observations come from inspection of the supplied source. Hardware behavior and fixes have not been tested as part of this import.

| Area | Current behavior / follow-up |
| --- | --- |
| USART1 after STOP | `BLE_UartPins_HiZ()` sets PA9/PA10 to analog mode. The wake path calls `MX_USART1_UART_Init()`, but the included HAL only calls `HAL_UART_MspInit()` when the UART handle is in `HAL_UART_STATE_RESET`. Disabling the UART does not reset that handle, so this path does not explicitly restore the pins to `GPIO_AF4_USART1`. GPIO restoration needs correcting and a STOP-to-wake hardware test before relying on BLE output. |
| Window name and old control code | `ActiveWindow_30s()` actually tests `10000U`. The commented ARMED/ACTIVE/WAIT_FALL state machine and its enum are historical; the active loop waits for falling edges. |
| Sensor data checks | Three bytes are read per register, but the third PEC byte and the sensor word's error flag are not validated. HAL I2C errors are reported; no additional data-integrity layer is implemented. |
| UART error handling | The main loop does not check the return status of the telemetry transmit call. |
| Power-up state | Generated GPIO initialization sets PA4 high; the first STOP entry subsequently forces it low. Verify this initial power pulse and the external power circuit on the bench. |
| Trigger handling | Debounce is a two-sample low-level check. Events during the active window are not queued, and STOP entry clears the event/pending flags. |
| BLE configuration | Module settings and a wireless receiver are external. UART transmission alone does not establish or validate a BLE link. |
| Temperature interpretation | The output is raw converted ambient/object sensor temperature. This source does not apply a body-temperature calibration or compensation model. |

## Bench checks

When hardware is available, check the following with the actual circuit:

1. Build and program the Debug configuration, then confirm the USART2 terminal settings.
2. Provide a defined PA0 high-to-low transition and confirm the sensor bring-up/read logs.
3. Check PA4 switches the intended downstream rail and confirm supply/bus levels while powered off and on.
4. After correcting USART1 GPIO restoration, observe PA9 traffic after STOP wake and verify reception on the configured BLE module and wireless receiver.
5. Return PA0 high, trigger again, and verify repeat operation and return to STOP. Measure MCU/peripheral current with the debug/probe configuration recorded.

No fresh IDE compilation, on-board execution, current measurement, or wireless reception test is claimed for this repository import.
