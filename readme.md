# MD_05_BLDC_Speed_OpenLoop

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

1. `main()` initializes GPIO, DMA, ADC1, ADC3, TIM1, and interrupt handlers.
2. `MX_FREERTOS_Init()` creates the motor control and monitor tasks.
3. `MonitorTask` continuously samples ADC1/ADC3 and reads the three Hall sensor inputs.
4. Hall sensor state changes trigger `EXTI_Callback()`, which notifies the motor control task.
5. `MotorCtrlTask` reads the current Hall sensor state and calls `MotorCtrl_DriveMotor()` to update the commutation state.
6. The appropriate high-side PWM channel and low-side GPIO outputs are activated based on the Hall pattern and rotation direction.
7. Motor data is continuously processed for diagnostics and monitoring.

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
Prescaler  = 84 - 1
Period     = 100 - 1
PWM        = 168 MHz / 84 / 100 = 20 kHz
```

The initial compare value is `10`, corresponding to approximately 10% duty cycle. Speed is controlled by adjusting the PWM duty cycle. The low-side outputs are ordinary GPIOs rather than TIM1 complementary outputs, so dead-time and shoot-through protection must be handled by the power stage or added explicitly before higher-power testing.

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

## USART1 Monitor and Duty Input

USART1 is configured for **1152000 baud, 8 data bits, no parity, 1 stop bit**. It streams real-time diagnostic data as comma-separated values:

```text
BEMFu,BEMFv,BEMFw,Iu,Iv,Iw,Vbus,temp,hall,speed
```

The receive idle interrupt accepts an ASCII decimal duty value, for example:

```text
50
```

The received string is converted to `uint8_t` and stored as `uartDuty`. Input values should be in the range `0` to `100`.

For VOFA+ FireWater monitoring, select USART1, set the baud rate to `1152000`, and use comma-separated text display.

## Project Layout

- `APP/motorCtrl.c`: Hall-based automatic commutation, direction control, and driver control
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

1. Confirm Hall sensor inputs (PH10, PH11, PH12) are reading correctly in the monitor task.
2. Confirm `EXTI_Callback()` is triggered when Hall sensors transition.
3. Confirm `MotorCtrlTask` is notified and calls `MotorCtrl_DriveMotor()` with the correct Hall signal.
4. Confirm `CTRL_SD` is at the driver enable level.
5. Measure PA8/PA9/PA10 for 20 kHz PWM and PB13/PB14/PB15 for the selected low-side output.
6. Verify the commutation pattern matches the forward or reverse table based on motor direction.
7. Test with a current-limited supply until dead-time and fault handling are verified.
8. If commutation is reversed, swap the motor direction setting or check Hall sensor wiring polarity.

## Regenerating CubeMX Files

Update peripheral and pin settings in `config.ioc`, then regenerate the project with STM32CubeMX. Keep application logic in the user source files and preserve the generated source list in `CMakeLists.txt` when adding new application files.
