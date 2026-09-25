#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "OLED.h"
#include "motorCtrl.h"

static MotorDataType MotorData = {
	.MotorState = MOTOR_STOP,
    .Speed = 0,
	.SpeedAim = 500,
	.HallSignal = 0xFF,
    .Duty = 1000,
};

static uint8_t lastHallSignal = 0xFF;
static float speedSample = 0;

static void updateSpeedAverage(void);
static void MotorCtrl_SetCCR(void);

// 从串口接收速度指令
void MotorCtrl_SetDuty(uint16_t cmdDuty)
{
    MotorData.Duty = cmdDuty;
    MotorCtrl_SetCCR();
}

// 外部文件调用状态信息结构体接口
MotorDataType* MotorCtrl_GetData(void)
{
	return &MotorData;
}

static void MotorCtrl_SetShutdown(GPIO_PinState State)
{
    HAL_GPIO_WritePin(CTRL_SD_GPIO_Port, CTRL_SD_Pin, State);
}

static void updateSpeedAverage(void)
{
    static float samples[SPEED_AVERAGE_WINDOW] = {0.0f};
    static float sum = 0.0f;
    static uint32_t nextSample = 0U;
    static uint32_t sampleCount = 0U;

    sum -= samples[nextSample];
    samples[nextSample] = speedSample;
    sum += speedSample;

    nextSample = (nextSample + 1U) % SPEED_AVERAGE_WINDOW;
    if (sampleCount < SPEED_AVERAGE_WINDOW)
    {
        sampleCount++;
    }

    MotorData.Speed = sum / (float)sampleCount;
}

static void MotorCtrl_Reset(void)
{
    HAL_TIM_Base_Stop(&htim1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    HAL_GPIO_WritePin(PWM_UL_GPIO_Port, PWM_UL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_VL_GPIO_Port, PWM_VL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PWM_WL_GPIO_Port, PWM_WL_Pin, GPIO_PIN_RESET);
}

static void MotorCtrl_Commutate(void)
{
    MotorCtrl_Reset();
    if (MotorData.MotorState == MOTOR_ROTATE_FORWARD)
    {
        switch (MotorData.HallSignal) {
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
    else if (MotorData.MotorState == MOTOR_ROTATE_REVERSE)
    {
        switch (MotorData.HallSignal) {
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
    HAL_TIM_Base_Start_IT(&htim1);
}

static void MotorCtrl_CalcSpeed(void)
{
    static uint32_t tickus = 0;
    HAL_TIM_Base_Stop_IT(&htim5);
    tickus = __HAL_TIM_GET_COUNTER(&htim5);
    if (tickus == 0) {   
        HAL_TIM_Base_Start_IT(&htim5);
        return;
    }
    speedSample = (float)60e6/(tickus*6*2);
    updateSpeedAverage();
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    HAL_TIM_Base_Start_IT(&htim5);
}

static void MotorCtrl_GetHall(void)
{
    uint8_t hallu, hallv, hallw;
    hallu = HAL_GPIO_ReadPin(HALLU_GPIO_Port, HALLU_Pin);
    hallv = HAL_GPIO_ReadPin(HALLV_GPIO_Port, HALLV_Pin);
    hallw = HAL_GPIO_ReadPin(HALLW_GPIO_Port, HALLW_Pin);

    MotorData.HallSignal = (hallw<<2)|(hallv<<1)|(hallu);
}

static void MotorCtrl_SetCCR(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MotorData.Duty);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, MotorData.Duty);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, MotorData.Duty);
}

void StartBtnStateTask(void *argument)
{
    for(;;)
    {
        uint8_t keyValue = ulTaskNotifyTake(pdTRUE, osWaitForever);
        lastHallSignal = 0xFF;

        if (keyValue == 1)
        {
            if (MotorData.MotorState==MOTOR_STOP || MotorData.MotorState==MOTOR_ROTATE_REVERSE)
            {
                MotorData.MotorState = MOTOR_ROTATE_FORWARD;
                MotorCtrl_SetShutdown(GPIO_PIN_SET);
                MotorCtrl_SetCCR();
                MotorCtrl_GetHall();
                MotorCtrl_Commutate();
                HAL_TIM_Base_Start_IT(&htim5);
            }
            else if (MotorData.MotorState == MOTOR_ROTATE_FORWARD)
            {
				MotorData.MotorState = MOTOR_STOP;
                MotorCtrl_Reset();
                MotorCtrl_SetShutdown(GPIO_PIN_RESET);
            }
        }
        else if (keyValue == 2)
        {
            if (MotorData.MotorState==MOTOR_STOP || MotorData.MotorState==MOTOR_ROTATE_FORWARD)
            {
                MotorData.MotorState = MOTOR_ROTATE_REVERSE;
                MotorCtrl_SetShutdown(GPIO_PIN_SET);
                MotorCtrl_SetCCR();
                MotorCtrl_GetHall();
                MotorCtrl_Commutate();
				HAL_TIM_Base_Start_IT(&htim5);
            }
            else if (MotorData.MotorState == MOTOR_ROTATE_REVERSE)
            {
				MotorData.MotorState = MOTOR_STOP;
                MotorCtrl_Reset();
                MotorCtrl_SetShutdown(GPIO_PIN_RESET);
			}
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY1_Pin)
    {
        xTaskNotifyFromISR(BtnStateTaskHandle, 0x01, eSetValueWithOverwrite, pdFALSE);
    }
    else if (GPIO_Pin == KEY2_Pin)
    {
        xTaskNotifyFromISR(BtnStateTaskHandle, 0x02, eSetValueWithOverwrite, pdFALSE);
    }

    else if (GPIO_Pin & (HALLU_Pin|HALLV_Pin|HALLW_Pin))
    {
        // 速度采样、计算、滤波
        MotorCtrl_CalcSpeed();

        // 获取hall信号
	    MotorCtrl_GetHall();
        // 基于hall信号切换六步磁矢量
	    MotorCtrl_Commutate();
    }
}

// 转速计时器溢出时调用
void MotorCtrl_SetSpeedZero(void)
{
    speedSample = 0;
    updateSpeedAverage();
    HAL_TIM_Base_Start_IT(&htim5);
}

void MotorCtrl_PWMCallback(void)
{
    // 不需要在20kHz中控制
}