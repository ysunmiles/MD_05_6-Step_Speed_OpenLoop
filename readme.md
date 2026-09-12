# MD_04_BLDC_VOFA_BTN

STM32F407 BLDC motor-control firmware based on STM32 HAL, CMSIS-RTOS V2, VOFA+ and FreeRTOS. The current control path is a manual six-step commutation prototype: each press of `KEY0` advances one commutation step. ADC measurements are collected by DMA and streamed to VOFA+ using the FireWater protocol.

The project is generated with STM32CubeMX and built with CMake/Ninja. The target uses an STM32F407 device running at 168 MHz.

## Current Features

- Manual six-step BLDC commutation driven by an EXTI button interrupt
- TIM1 three-channel PWM for the high-side phases
- GPIO-controlled low-side phase outputs
- ADC1 and ADC3 multi-channel continuous conversion with circular DMA
- Motor monitor task for back-EMF, phase current, bus voltage, temperature, and Hall inputs
- CMSIS-RTOS message queue between monitor and UART output tasks
- USART1 FireWater output for VOFA+
- SSD1306-compatible OLED support through the existing BSP driver

This is an open-loop/manual commutation prototype. Automatic Hall commutation, closed-loop speed control, current control, and fault protection are not yet implemented in this repository.

## Runtime Flow

1. `main()` initializes GPIO, DMA, USART1, ADC1, ADC3, and TIM1.
2. `MX_FREERTOS_Init()` creates the monitor, display, and motor-control tasks.
3. `MonitorTask` continuously samples ADC1/ADC3 and reads the three Hall inputs.
4. The monitor data is placed in `MotorDatasQueue`.
5. `DisplayTask` sends one FireWater frame for every queued data set.
6. A falling edge on `KEY0` wakes `MotorCtrlTask`, increments the commutation step, and calls `rotateMotor()`.

## Motor Control

TIM1 generates PWM on the high-side phase outputs:

| Phase | High-side PWM | Low-side GPIO |
| --- | --- | --- |
| U | PA8 / TIM1_CH1 | PB13 / `PWM_UL` |
| V | PA9 / TIM1_CH2 | PB14 / `PWM_VL` |
| W | PA10 / TIM1_CH3 | PB15 / `PWM_WL` |

The current six-step table is:

| Step | High-side PWM | Low-side output |
| --- | --- | --- |
| 1 | U+ | V- |
| 2 | U+ | W- |
| 3 | V+ | W- |
| 4 | V+ | U- |
| 5 | W+ | U- |
| 6 | W+ | V- |

Before each step, all three PWM channels and low-side GPIO outputs are disabled. The PWM period is configured as:

```text
TIM1 clock = 168 MHz
Prescaler  = 84 - 1
Period     = 100 - 1
PWM        = 168 MHz / 84 / 100 = 20 kHz
```

The initial compare value is `10`, corresponding to approximately 10% duty cycle. The low-side outputs are ordinary GPIOs rather than TIM1 complementary outputs, so dead-time and shoot-through protection must be handled by the power stage or added explicitly before higher-power testing.

## Pinout

| Function | MCU pin | Configuration |
| --- | --- | --- |
| Commutation button `KEY0` | PE2 | Falling-edge EXTI, pull-up |
| U/V/W back-EMF inputs | PF9 / PF8 / PF7 | ADC3 channels 7 / 6 / 5 |
| Temperature input `VTEMP` | PA0 | ADC3 channel 0 |
| Phase current U/V/W | PB0 / PA6 / PA3 | ADC1 channels 8 / 6 / 3 |
| DC bus voltage `VBUS` | PB1 | ADC1 channel 9 |
| Hall U/V/W | PH10 / PH11 / PH12 | Digital inputs |
| Driver shutdown/enable | PF10 | `CTRL_SD` GPIO output |
| OLED SCL/SDA | PD14 / PD0 | Software GPIO interface |
| OLED power/ground control | PD4 / PG0 | GPIO-controlled |

Verify the driver IC enable polarity before powering the motor. The firmware currently calls `setShutdown(GPIO_PIN_RESET)` when the motor-control task starts.

## VOFA+ FireWater Output

USART1 is configured for **115200 baud, 8 data bits, no parity, 1 stop bit**. Each line contains the following 11 comma-separated values:

```text
BEMFu,BEMFv,BEMFw,Iu,Iv,Iw,Vbus,temp,Hallu,Hallv,Hallw\n
```

Example:

```text
12.345,12.346,12.347,1.200,1.201,1.202,24.000,35.600,1,0,1
```

In VOFA+, select the serial port, set the baud rate to 115200, and choose the FireWater protocol. The first eight fields are formatted as floating-point values with three decimal places; the last three fields are Hall logic levels.

## Project Layout

- `APP/motorCtrl.c`: manual commutation, button notification, and driver control
- `APP/monitor.c`: ADC DMA acquisition and motor data calculation
- `APP/display.c`: message-queue consumer and FireWater UART output
- `APP/monitor.h`: `MotorDatasType` definition
- `Core/`: STM32CubeMX-generated initialization, interrupts, and RTOS setup
- `BSP/`: OLED driver
- `Drivers/`: STM32F4 CMSIS and HAL drivers
- `Middlewares/Third_Party/FreeRTOS/`: FreeRTOS kernel and CMSIS-RTOS adapter
- `config.ioc`: STM32CubeMX configuration source
- `CMakeLists.txt` and `CMakePresets.json`: CMake/Ninja build configuration

## Build

### Prerequisites

- CMake 3.22 or later
- Ninja
- Arm GNU Toolchain with `arm-none-eabi-gcc`
- ST-LINK or another compatible programmer/debugger

From the project root:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

For an optimized build:

```powershell
cmake --preset Release
cmake --build --preset Release
```

Build outputs are placed in `build/Debug` or `build/Release`. The executable target is named `config`.

## Debug Checklist

When manual commutation does not work, check these signals in order:

1. Confirm `EXTI2_IRQHandler()` is reached when PE2 is pulled low.
2. Confirm `HAL_GPIO_EXTI_Callback()` calls `vTaskNotifyGiveFromISR()`.
3. Confirm `MotorCtrlTask` increments `step` after each button press.
4. Confirm `CTRL_SD` is at the driver enable level.
5. Measure PA8/PA9/PA10 for 20 kHz PWM and PB13/PB14/PB15 for the selected low-side output.
6. Test with a current-limited supply until dead-time and fault handling are verified.

## Regenerating CubeMX Files

Update peripheral and pin settings in `config.ioc`, then regenerate the project with STM32CubeMX. Keep application logic in the user source files and preserve the generated source list in `CMakeLists.txt` when adding new application files.
