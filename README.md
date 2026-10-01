# In-ear infrared thermometer — STM32 firmware

An event-triggered infrared thermometer prototype developed for an Electronic & Electrical Engineering final-year project at the University of Edinburgh. The firmware combines an **STM32L053**, an **MLX90614 infrared sensor**, a **UART-connected BLE module**, and an **external optical trigger**.

The application is designed to spend its idle time in **STOP mode**, switch on the peripheral supply when triggered, read ambient and object temperatures, and transmit the readings during a short measurement window.

**Current version:** the original `System_test_ob` bench prototype for **NUCLEO-L053R8**. Application logic is preserved; application comments now explain the active flow in English. The UART1 pin-restoration issue described in [firmware notes](docs/firmware.md#known-limitations) needs attention before relying on BLE output after STOP.

## At a glance

| Item | Configuration in this repository |
| --- | --- |
| Board / MCU | NUCLEO-L053R8 / STM32L053R8Tx |
| Language / framework | C, STM32 HAL; bare-metal main loop |
| Sensor | MLX90614, I2C1, 7-bit address `0x5A` |
| Trigger | PA0, falling-edge EXTI, followed by a 10 ms low-level check |
| Measurement window | Approximately 10 s, after peripheral power-up and sensor initialization |
| Sampling | A 500 ms delay between read attempts; communication adds overhead |
| BLE interface | USART1, 115200 baud, 8N1; temperature text sent to an external BLE module |
| Debug interface | USART2, 115200 baud, 8N1; `printf()` output |
| Idle power handling | Peripheral supply off; USART1 pins high impedance; MCU in STOP |

`Ta` is the sensor's ambient-temperature reading. `To` is its object-temperature reading. The firmware outputs these sensor readings in °C; body-temperature compensation is not implemented in this snapshot.

## Getting started

1. Clone or download the repository:

   ```sh
   git clone https://github.com/Lane2048/in-ear-thermometer-stm32.git
   ```

2. Follow the [hardware connections](docs/hardware.md). You need the Nucleo board, MLX90614, a driven trigger signal, an external power-switch circuit, and a suitably configured UART BLE module for wireless output.
3. In **STM32CubeIDE**, choose **File → Import → General → Existing Projects into Workspace**, then select `firmware/System_test_ob`. Import the existing project named `System_test_ob`.
4. Select the **Debug** build configuration and build the project. The required HAL/CMSIS source files are included. Keep floating-point `printf` support enabled (`-u _printf_float`); the supplied Debug configuration already enables it.
5. Connect the board through ST-LINK and create a local STM32 debug/run configuration to program it. Open the board's virtual COM port at **115200, 8 data bits, no parity, 1 stop bit, no flow control**.
6. Start with PA0 high, then drive it low and keep it low through the debounce check. The first debug message is produced on a valid trigger, rather than at startup. Return PA0 high before creating another falling-edge trigger.

See [firmware notes](docs/firmware.md) for the execution path, expected output, and current limitations. These are reproduction instructions; a fresh IDE build and hardware run have not been verified for this repository import.

## What the firmware does

1. **Sleep:** force PA4 low, disable USART1, place PA9/PA10 in analog mode, suspend SysTick, and enter STOP.
2. **Wake:** PA0 EXTI records an event. The main loop restores the clock and checks that the trigger remains low after 10 ms.
3. **Measure and send:** drive PA4 high, wait 200 ms, initialize the sensor, then read `Ta` and `To` for approximately 10 s. Successful readings are printed on USART2 and submitted to USART1.
4. **Return to sleep:** switch the peripheral supply off and repeat the STOP sequence.

The routine is still named `ActiveWindow_30s()`, but its executable timeout is **`10000U` (10 s)**. BLE UART transmission happens during measurement; this version has no separate 5 s transmit phase. Historical commented code in `main.c` describes an earlier state machine and is not the active control flow.

## Where to look

| Path | Purpose |
| --- | --- |
| [main.c](firmware/System_test_ob/Core/Src/main.c) | Wake handling, STOP entry, power switching, measurement window, and telemetry |
| [Mlx90614.c](firmware/System_test_ob/Core/Src/Mlx90614.c) | Sensor register reads and raw-word conversion to °C |
| [gpio.c](firmware/System_test_ob/Core/Src/gpio.c), [i2c.c](firmware/System_test_ob/Core/Src/i2c.c), [usart.c](firmware/System_test_ob/Core/Src/usart.c) | GPIO and peripheral configuration |
| [ble_uart.c](firmware/System_test_ob/Core/Src/ble_uart.c) | Additional BLE UART helpers; not called by the active application |
| [System_test_ob.ioc](firmware/System_test_ob/System_test_ob.ioc) | STM32CubeMX pin, clock, and project configuration |
| `firmware/System_test_ob/Drivers/` | Included ST HAL and Arm CMSIS dependencies |
| [Hardware guide](docs/hardware.md) | Pin mapping, interface requirements, and external hardware |
| [Firmware guide](docs/firmware.md) | Runtime details, serial output, build notes, and limitations |

## Project scope

This repository contains the firmware prototype and its development configuration. PCB design files, the BLE module's configuration, calibration models, and experimental datasets are separate project materials. Battery-life and accuracy results from the wider project are not reproduced by this source tree alone.

The included third-party components retain their original copyright notices and license files under `Drivers/`.
