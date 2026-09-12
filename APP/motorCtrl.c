#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "OLED.h"

void setShutdown(GPIO_PinState State)
{
    HAL_GPIO_WritePin(CTRL_SD_GPIO_Port, CTRL_SD_Pin, State);
}

void rotateMotor(uint8_t step)
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_RESET);

    switch (step) {
        case 1: // U+ V-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
            break;
        case 2: // U+ W-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
            break;
        case 3: // V+ W-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
            break;
        case 4: // V+ U-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
            break;
        case 5: // W+ U-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
            HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
            break;
        case 6: // W+ V-
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
            HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
            break;
        default:
            break;
    }
}

void StartMotorCtrlTask(void *argument)
{
    static uint8_t step = 0;
    OLED_Init();
    OLED_ShowString(1, 1, "Step:");
    setShutdown(GPIO_PIN_SET);
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, osWaitForever);
        step ++;
        if (step > 6) {step = 1;}
        rotateMotor(step);
        OLED_ShowNum(1, 6, step, 1);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == KEY0_Pin)
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;

        vTaskNotifyGiveFromISR(
            MotorCtrlTaskHandle,
            &higherPriorityTaskWoken
        );

        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}