# MD_05_6-Step_Speed_OpenLoop

STM32F407 BLDC motor-control firmware based on STM32 HAL, CMSIS-RTOS V2, and FreeRTOS. This project implements **Hall sensor-based automatic commutation with bidirectional speed control**. ADC measurements are collected by DMA and real-time motor diagnostics are processed by the monitor task.

The project is generated with STM32CubeMX and built with CMake/Ninja. The target uses an STM32F407 device running at 168 MHz.

## Current Features

- Hall sensor-based automatic six-step BLDC commutation
- Bidirectional motor control (forward and reverse rotation)
- TIM1 three-channel PWM for the high-side phases
- GPIO-controlled low-side phase outputs
- ADC1 and ADC3 multi-channel continuous conversion with circular DMA
- Motor monitor task for back-EMF, phase current, bus voltage, temperature, and Hall sensor inputs
- Real-time motor state tracking and commutation based on Hall signal transitions
- SSD1306-compatible OLED support through the existing BSP driver

This project provides Hall-based automatic commutation with open-loop PWM speed control. Closed-loop speed regulation, current limiting, and advanced fault protection are not yet implemented.

## Runtime Flow

1. `main()` initializes GPIO, DMA, ADC1, ADC3, TIM1, TIM5, USART1, and the FreeRTOS scheduler.
2. `MX_FREERTOS_Init()` creates `MonitorTask` and `BtnStateTask`.
3. `MonitorTask` starts the ADC DMA conversions, OLED, and USART1 receive-to-idle interrupt, then continuously publishes motor telemetry.
4. Pressing `KEY1` or `KEY2` notifies `BtnStateTask`. `KEY1` starts/stops forward rotation; `KEY2` starts/stops reverse rotation.
5. Hall sensor transitions trigger `HAL_GPIO_EXTI_Callback()`. The callback measures the elapsed Hall interval with TIM5, updates the filtered speed, reads the new Hall pattern, and commutates the motor.
6. The appropriate high-side PWM channel and low-side GPIO output are activated based on the Hall pattern and rotation direction.
7. If TIM5 overflows before another Hall transition, the measured speed is reset to zero.

## Motor Control

TIM1 generates PWM on the high-side phase outputs. The motor driver uses six-step commutation patterns based on Hall sensor inputs:

| Phase | High-side PWM | Low-side GPIO |
| --- | --- | --- |
| U | PA8 / TIM1_CH1 | PB13 / `PWM_UL` |
| V | PA9 / TIM1_CH2 | PB14 / `PWM_VL` |
| W | PA10 / TIM1_CH3 | PB15 / `PWM_WL` |

### Forward Rotation Commutation Table (Hall Signal → Active Phase)

| Hall Signal | High-side PWM | Low-side output |
| --- | --- | --- |
| 5 (101) | U+ | V- |
| 1 (001) | U+ | W- |
| 3 (011) | V+ | W- |
| 2 (010) | V+ | U- |
| 6 (110) | W+ | U- |
| 4 (100) | W+ | V- |

### Reverse Rotation Commutation Table

The reverse direction reverses the commutation sequence:

| Hall Signal | High-side PWM | Low-side output |
| --- | --- | --- |
| 5 (101) | V+ | U- |
| 1 (001) | W+ | U- |
| 3 (011) | W+ | V- |
| 2 (010) | U+ | V- |
| 6 (110) | U+ | W- |
| 4 (100) | V+ | W- |

Before each commutation transition, all PWM channels and low-side GPIO outputs are disabled to prevent shoot-through. The PWM period is configured as:

```text
TIM1 clock = 168 MHz
Prescaler  = 0
Period     = 8400 - 1
PWM        = 168 MHz / 8400 = 20 kHz
```

The initial compare value is `10`, corresponding to approximately 0.12% duty cycle with the current 8400-count period. The firmware initializes the runtime duty command to `1000` (approximately 11.9%). Speed is controlled by changing the TIM1 compare value over USART1. The low-side outputs are ordinary GPIOs rather than TIM1 complementary outputs, so dead-time and shoot-through protection must be handled by the power stage or added explicitly before higher-power testing.

Hall speed is calculated from the interval between Hall transitions using TIM5, which counts at 1 MHz:

```text
speed sample = 60,000,000 / (TIM5 count × 6 × 2)
reported speed = average of the latest 6 samples
```

The calculation assumes six Hall transitions per electrical revolution and two electrical pole pairs. Adjust the formula if the motor has a different pole-pair count.

## Pinout

| Function | MCU pin | Configuration |
| --- | --- | --- |
| Forward/stop button `KEY1` | PE3 | Falling-edge EXTI, pull-up |
| Reverse/stop button `KEY2` | PE4 | Falling-edge EXTI, pull-up |
| U/V/W back-EMF inputs | PF9 / PF8 / PF7 | ADC3 channels 7 / 6 / 5 |
| Temperature input `VTEMP` | PA0 | ADC3 channel 0 |
| Phase current U/V/W | PB0 / PA6 / PA3 | ADC1 channels 8 / 6 / 3 |
| DC bus voltage `VBUS` | PB1 | ADC1 channel 9 |
| Hall U/V/W | PH10 / PH11 / PH12 | Digital inputs |
| Driver shutdown/enable | PF10 | `CTRL_SD` GPIO output |
| OLED SCL/SDA | PD14 / PD0 | Software GPIO interface |
| OLED power/ground control | PD4 / PG0 | GPIO-controlled |

Verify the driver IC enable polarity before powering the motor. The firmware initializes `CTRL_SD` high, sets it high when a direction is started, and resets it low when that direction is stopped.

## USART1 Monitor and Duty Input

USART1 is configured for **1152000 baud, 8 data bits, no parity, 1 stop bit**. It streams real-time diagnostic data as comma-separated values:

```text
BEMFu,BEMFv,BEMFw,Iu,Iv,Iw,Vbus,temp,hall,speed,duty
```

The receive-to-idle interrupt accepts an ASCII decimal duty command, for example:

```text
50
```

The received string is converted to `uint16_t` and written directly to the TIM1 compare registers. With the current period, valid compare values are `0` to `8399`; values above the period are not clamped by the firmware. For percentage-based control, convert the desired percentage to a compare value using approximately `duty = percentage × 84`.

The telemetry frame is transmitted with USART1 TX DMA and is formatted for VOFA+ FireWater. Select USART1, set the baud rate to `1152000`, and use comma-separated text display. The monitor task currently sends frames continuously without an `osDelay()`, so the achievable rate depends on UART and DMA throughput.

## Project Layout

- `APP/motorCtrl.c`: Hall-based automatic commutation, direction control, speed calculation, and driver control
- `APP/motorCtrl.h`: Motor control interface and configuration
- `APP/monitor.c`: ADC DMA acquisition and motor data calculation
- `APP/monitor.h`: Motor data type definitions
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

When Hall-based commutation does not work, check these signals in order:

1. Confirm Hall sensor inputs (PH10, PH11, PH12) are reading correctly.
2. Confirm `HAL_GPIO_EXTI_Callback()` is triggered on Hall transitions.
3. Confirm `KEY1` or `KEY2` starts the expected direction and enables the driver.
4. Confirm `CTRL_SD` is at the driver enable level.
5. Measure PA8/PA9/PA10 for 20 kHz PWM and PB13/PB14/PB15 for the selected low-side output.
6. Verify the commutation pattern matches the forward or reverse table based on motor direction.
7. Confirm TIM5 is running and that the reported speed responds to Hall transitions.
8. Test with a current-limited supply until dead-time and fault handling are verified.
9. If commutation is reversed, swap the motor direction setting or check Hall sensor wiring polarity.

## Regenerating CubeMX Files

Update peripheral and pin settings in `config.ioc`, then regenerate the project with STM32CubeMX. Keep application logic in the user source files and preserve the generated source list in `CMakeLists.txt` when adding new application files.
