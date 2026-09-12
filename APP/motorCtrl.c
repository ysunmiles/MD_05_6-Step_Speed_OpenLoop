#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "OLED.h"
#include "motorCtrl.h"

void setShutdown(GPIO_PinState State)
{
    HAL_GPIO_WritePin(CTRL_SD_GPIO_Port, CTRL_SD_Pin, State);
}

void driveMotor(uint8_t hallSignal, uint8_t rotateDirection)
{
    static uint8_t lastHallSignal = 0xFF;
    if (hallSignal == lastHallSignal)
    {
        return;
    }
    lastHallSignal = hallSignal;

    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_RESET);

    if (rotateDirection == 1)
    {
        switch (hallSignal) {
            case 5: // U+ V-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
                break;
            case 1: // U+ W-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
                break;
            case 3: // V+ W-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
                HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
                break;
            case 2: // V+ U-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
                HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
                break;
            case 6: // W+ U-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
                HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
                break;
            case 4: // W+ V-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
                HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
                break;
            default:
                break;
        }
    }
    
}

void StartMotorCtrlTask(void *argument)
{
    for(;;)
    {
        static uint8_t motorState = 0;
        ulTaskNotifyTake(pdTRUE, osWaitForever);
        if (motorState == 0)
        {
            setShutdown(GPIO_PIN_SET);
            HAL_TIM_Base_Start_IT(&htim1);
            motorState = 1;
        }
        else if (motorState == 1)
        {
            setShutdown(GPIO_PIN_RESET);
            HAL_TIM_Base_Stop(&htim1);
            motorState = 0;
        }
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
    }

    if(GPIO_Pin & (HALLU_Pin|HALLV_Pin|HALLW_Pin))
    {
        static uint32_t tickus = 0;
        HAL_TIM_Base_Stop(&htim5);
        tickus = __HAL_TIM_GET_COUNTER(&htim5);
        if (tickus == 0) {   
            HAL_TIM_Base_Start(&htim5);
            return;}
        float speed = (float)60/(tickus*6*2*1e-6);
        
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(MonitorTaskHandle, (uint32_t)speed, eSetValueWithOverwrite, &higherPriorityTaskWoken);
        __HAL_TIM_SET_COUNTER(&htim5, 0);
        HAL_TIM_Base_Start(&htim5);
    }
}

uint8_t getHall(void)
{
    uint8_t hallu, hallv, hallw;
    hallu = HAL_GPIO_ReadPin(HALLU_GPIO_Port, HALLU_Pin);
    hallv = HAL_GPIO_ReadPin(HALLV_GPIO_Port, HALLV_Pin);
    hallw = HAL_GPIO_ReadPin(HALLW_GPIO_Port, HALLW_Pin);

    uint8_t hallSignal = (hallu<<2)|(hallv<<1)|(hallw);
    return hallSignal;
}

void motorCtrl_PWMCallback(void)
{
    // 获取hall信号
    uint8_t hallSignal = getHall();
    // 将hall信号转换为step信息
    driveMotor(hallSignal, ROTATE_DIR);
}