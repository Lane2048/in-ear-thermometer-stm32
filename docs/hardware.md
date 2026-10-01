# Hardware connections

The supplied `.ioc`, startup file, and linker script target **NUCLEO-L053R8 / STM32L053R8Tx**. The linker defines **64 KB Flash** and **8 KB RAM**. A different STM32 device or custom PCB needs matching device, pin, startup, and memory settings.

## Pin mapping

| STM32 pin | Signal | Connection / behavior |
| --- | --- | --- |
| PA0 | `WAKE` / EXTI0 | Driven output of the optical-trigger logic; a high-to-low transition starts a measurement |
| PA4 | `POWERSWITCH` | External peripheral power-switch control; high = on, low = off in the application |
| PB8 | I2C1 SCL | MLX90614 SCL |
| PB9 | I2C1 SDA | MLX90614 SDA |
| PA9 | USART1 TX | BLE module RX |
| PA10 | USART1 RX | BLE module TX; reception is not used by the active main loop |
| PA2 | USART2 TX | Debug output through the Nucleo ST-LINK virtual COM connection |
| PA3 | USART2 RX | Debug UART receive pin; no application command parser is enabled |
| PA5 | `LD2` | Nucleo green LED; initialized low, not used as an application status indicator |
| PA13 | SWDIO | ST-LINK debugging/programming |
| PA14 | SWCLK | ST-LINK debugging/programming |

The mapping comes from `Core/Inc/main.h`, the peripheral initialization files, and `System_test_ob.ioc`.

## External hardware

### MLX90614

- Connect the sensor to I2C1 and the appropriate peripheral supply and common ground.
- Match the supply and logic levels to the exact MLX90614 variant or breakout used.
- Provide suitable external I2C pull-ups. PB8/PB9 are configured as open-drain with **no internal pull-ups**.
- The driver uses 7-bit address **`0x5A`**, reads ambient register **`0x06`** and object register **`0x07`**, and converts the returned words to °C.

### Trigger and power switch

PA0 has **no internal pull-up or pull-down**. The trigger circuit must drive a defined level. To reproduce a trigger, begin high, transition low, and remain low through the 10 ms check. A signal already held low at startup does not provide the required falling edge.

PA4 controls an **external switching circuit** for the downstream sensor/BLE supply; it is a control signal rather than a supply output. Match the circuit to the application's active-high behavior. The project uses a beam-break/optical trigger, but the source archive does not include its schematic or the power-switch schematic.

### BLE module

Use a UART-connected BLE module configured for **115200 baud, 8N1, no hardware flow control**, with a shared ground and compatible logic levels. Connect TX to RX in each direction.

The firmware sends text through USART1. It does not configure the module with AT commands or implement a Bluetooth stack, service, advertising payload, or phone application. Wireless behavior depends on the external module's settings and receiver. Read the [USART1 wake limitation](firmware.md#known-limitations) before testing BLE transmission.

### Debug terminal

The Nucleo's ST-LINK virtual COM connection can expose USART2 on PA2/PA3 when the board's relevant connections/solder bridges are configured as described by ST. Use **115200 baud, 8N1, no flow control**. Only successful trigger events produce measurement logs.

## Reference documents

- [ST NUCLEO-L053R8 board information](https://www.st.com/en/evaluation-tools/nucleo-l053r8.html)
- [ST Nucleo-64 board user manual, UM1724](https://www.st.com/resource/en/user_manual/DM00105823-.pdf)
- [ST UART tutorial for NUCLEO-L053R8](https://dev.st.com/stm32cube-docs/stm32cubemx/6.18.0/en/docs/markup/CubeMX_UserManual/chapters/14_14_tutorial_4_example_of_uart_communications_with_an_stm32l053xx_nucleo_board.html)
