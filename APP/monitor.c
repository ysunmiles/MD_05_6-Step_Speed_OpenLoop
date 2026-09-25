#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "monitor.h"
#include "motorCtrl.h"
#include "OLED.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t uartRxBuffer[8];
static char frame[160];

static float calcTemp(uint16_t ADCVtempValue)
{
    const float Vtemp = (float)ADCVtempValue / 4095.0 * 3.3;
    const float Rt = 4.7*3.3/Vtemp - 4.7;

    const float temperatureK = 1.0f /
        (logf(Rt/10)/3380 + 1.0f/(25+273.15f));

    return temperatureK - 273.15f;
}

static void SendMotorDataFireWater(const MotorDataType* pMotorData)
{
    int frameLength = snprintf(frame, sizeof(frame),
        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.3f,%3u\n",
        
        (double)pMotorData->BEMFu,
        (double)pMotorData->BEMFv,
        (double)pMotorData->BEMFw,
        (double)pMotorData->Iu,
        (double)pMotorData->Iv,
        (double)pMotorData->Iw,
        (double)pMotorData->Vbus,
        (double)pMotorData->Temp,
        (unsigned int)pMotorData->HallSignal,
        (double)pMotorData->Speed,
        (unsigned int)pMotorData->Duty
    );

    HAL_UART_Transmit_DMA(&huart1, (uint8_t*)frame, frameLength);
}

void StartMonitorTask(void *argument)
{
    OLED_Init();
    OLED_ShowString(1, 1, "State:");
    OLED_ShowString(2, 1, "Speed Aim:");
    
    static uint16_t ADC1Data[4] = {0};
    static uint16_t ADC3Data[4] = {0};
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC1Data, 4);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)ADC3Data, 4);
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, uartRxBuffer, sizeof(uartRxBuffer));

    MotorDataType* pMotorData = MotorCtrl_GetData();

    for(;;)
    {
        pMotorData->BEMFu = (float)ADC3Data[3]/4095.0 * 3.3 * 25;
        pMotorData->BEMFv = (float)ADC3Data[2]/4095.0 * 3.3 * 25;
        pMotorData->BEMFw = (float)ADC3Data[1]/4095.0 * 3.3 * 25;
        pMotorData->Iu = ((float)ADC1Data[2]/4095.0 * 3.3 - 1.25)/0.12;
        pMotorData->Iv = ((float)ADC1Data[1]/4095.0 * 3.3 - 1.25)/0.12;
        pMotorData->Iw = ((float)ADC1Data[0]/4095.0 * 3.3 - 1.25)/0.12;
        pMotorData->Temp = calcTemp(ADC3Data[0]);
        pMotorData->Vbus = (float)ADC1Data[3]/4095.0 * 3.3 * 25;

        SendMotorDataFireWater(pMotorData);

        // 软件I2C严重影响串口采样率，考虑单开一个任务
        // OLED_ShowNum(1, 8, pMotorData->MotorState, 1);
        // OLED_ShowNum(2, 12, pMotorData->SpeedAim, 4);

        // 不使用延时的情况下，发送频率约为1kHz
        // osDelay(1);
    }

}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1)
    {
        uartRxBuffer[Size < sizeof(uartRxBuffer) ? Size : sizeof(uartRxBuffer) - 1U] = '\0';
        uint16_t uartDuty = (uint16_t)atoi((char *)uartRxBuffer);

        MotorCtrl_SetDuty(uartDuty);
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, uartRxBuffer, sizeof(uartRxBuffer));
    }
}
