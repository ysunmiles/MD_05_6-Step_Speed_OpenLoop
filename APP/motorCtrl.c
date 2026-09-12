#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "OLED.h"
#include "motorCtrl.h"

static volatile MotorDirection motorDirection = MOTOR_DIR_FORWARD;

void motorCtrl_SetDirection(MotorDirection direction)
{
    if (direction == MOTOR_DIR_FORWARD || direction == MOTOR_DIR_REVERSE)
    {
        motorDirection = direction;
    }
}

MotorDirection motorCtrl_GetDirection(void)
{
    return motorDirection;
}

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
    else if (rotateDirection == 2)
    {
        switch (hallSignal) {
            case 5: // V+ U-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
                HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
                break;
            case 1: // W+ U-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
                HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_SET);
                break;
            case 3: // W+ V-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
                HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
                break;
            case 2: // U+ V-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_SET);
                break;
            case 6: // U+ W-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
                break;
            case 4: // V+ W-
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
                HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_SET);
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
        uint8_t keyValue = ulTaskNotifyTake(pdTRUE, osWaitForever);
        if (keyValue == 1)
        {
            if (motorState==0 || motorState==2)
            {
                setShutdown(GPIO_PIN_SET);
                HAL_TIM_Base_Start_IT(&htim1);
                motorCtrl_SetDirection(MOTOR_DIR_FORWARD);
                motorState = 1;
            }
            else if (motorState == 1)
            {
                setShutdown(GPIO_PIN_RESET);
                HAL_TIM_Base_Stop(&htim1);
                motorState = 0;
            }
        }
        else if (keyValue == 2)
        {
            if (motorState==0 || motorState==1)
            {
                setShutdown(GPIO_PIN_SET);
                HAL_TIM_Base_Start_IT(&htim1);
                motorCtrl_SetDirection(MOTOR_DIR_REVERSE);
                motorState = 2;
            }
            else if (motorState == 2)
            {
                setShutdown(GPIO_PIN_RESET);
                HAL_TIM_Base_Stop(&htim1);
                motorState = 0;
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY1_Pin)
    {
        xTaskNotifyFromISR(MotorCtrlTaskHandle, 0x01, eSetValueWithOverwrite, pdFALSE);
    }
    else if (GPIO_Pin == KEY2_Pin)
    {
        xTaskNotifyFromISR(MotorCtrlTaskHandle, 0x02, eSetValueWithOverwrite, pdFALSE);
    }

    else if (GPIO_Pin & (HALLU_Pin|HALLV_Pin|HALLW_Pin))
    {
        static uint32_t tickus = 0;
        HAL_TIM_Base_Stop(&htim5);
        tickus = __HAL_TIM_GET_COUNTER(&htim5);
        if (tickus == 0) {   
            HAL_TIM_Base_Start(&htim5);
            return;}
        float speedx100 = (float)100*60/(tickus*6*2*1e-6);
        
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(MonitorTaskHandle, (uint32_t)speedx100, eSetValueWithOverwrite, &higherPriorityTaskWoken);
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

void motorCtrl_PWMCallback(MotorDirection direction)
{
    // 获取hall信号
    uint8_t hallSignal = getHall();
    // 通过hall信号设定磁矢量
    driveMotor(hallSignal, (uint8_t)direction);
}