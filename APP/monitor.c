#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "monitor.h"
#include "OLED.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "motorCtrl.h"

#define SPEED_AVERAGE_WINDOW 6U

static uint8_t uartRxBuffer[8];
static uint16_t uartDuty;

static float updateSpeedAverage(float sample)
{
    static float samples[SPEED_AVERAGE_WINDOW] = {0.0f};
    static float sum = 0.0f;
    static uint32_t nextSample = 0U;
    static uint32_t sampleCount = 0U;

    sum -= samples[nextSample];
    samples[nextSample] = sample;
    sum += sample;

    nextSample = (nextSample + 1U) % SPEED_AVERAGE_WINDOW;
    if (sampleCount < SPEED_AVERAGE_WINDOW)
    {
        sampleCount++;
    }

    return sum / (float)sampleCount;
}

static float calcTemp(uint16_t ADCVtempValue)
{
    const float Vtemp = (float)ADCVtempValue / 4095.0 * 3.3;
    const float Rt = 4.7*3.3/Vtemp - 4.7;

    const float temperatureK = 1.0f /
        (logf(Rt/10)/3380 + 1.0f/(25+273.15f));

    return temperatureK - 273.15f;
}

static void SendMotorDataFireWater(const MotorDatasType *motorData)
{
    char frame[160];
    int frameLength = snprintf(frame, sizeof(frame),
        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.3f,%3u\n",
        
        (double)motorData->BEMFu,
        (double)motorData->BEMFv,
        (double)motorData->BEMFw,
        (double)motorData->Iu,
        (double)motorData->Iv,
        (double)motorData->Iw,
        (double)motorData->Vbus,
        (double)motorData->temp,
        (unsigned int)motorData->hall,
        (double)motorData->speed,
        (unsigned int)motorData->duty
    );

    if (frameLength > 0 && frameLength < (int)sizeof(frame))
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)frame, (uint16_t)frameLength, 10);
    }
}

void StartMonitorTask(void *argument)
{
    OLED_Init();
    OLED_ShowString(1, 1, "speed:");
    
    static uint16_t ADC1Data[4] = {0};
    static uint16_t ADC3Data[4] = {0};
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC1Data, 4);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)ADC3Data, 4);
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, uartRxBuffer, sizeof(uartRxBuffer));

    for(;;)
    {
        static MotorDatasType MotorData;
        static float speedSample;
        uint32_t tickus;

        if (xTaskNotifyWait(0, UINT32_MAX, &tickus, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            speedSample = (float)60e6/(tickus*6*2);
        }
        else
        {
            speedSample = 0.0f;
        }

        MotorData.speed = updateSpeedAverage(speedSample);
        MotorData.BEMFu = (float)ADC3Data[3]/4095.0 * 3.3 * 25;
        MotorData.BEMFv = (float)ADC3Data[2]/4095.0 * 3.3 * 25;
        MotorData.BEMFw = (float)ADC3Data[1]/4095.0 * 3.3 * 25;
        MotorData.temp = calcTemp(ADC3Data[0]);
        MotorData.Iu = ((float)ADC1Data[2]/4095.0 * 3.3 - 1.25)/0.12;
        MotorData.Iv = ((float)ADC1Data[1]/4095.0 * 3.3 - 1.25)/0.12;
        MotorData.Iw = ((float)ADC1Data[0]/4095.0 * 3.3 - 1.25)/0.12;
        MotorData.Vbus = (float)ADC1Data[3]/4095.0 * 3.3 * 25;
        MotorData.hall = MotorCtrl_GetHall();
        MotorData.duty = MotorCtrl_GetDuty();

        SendMotorDataFireWater(&MotorData);

        // OLED_ShowString(1, 7, "          ");
        // OLED_ShowFloat(1, 7, MotorData.speed, 2);

        // osDelay(20);
    }

}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1)
    {
        uartRxBuffer[Size < sizeof(uartRxBuffer) ? Size : sizeof(uartRxBuffer) - 1U] = '\0';
        uartDuty = (uint16_t)atoi((char *)uartRxBuffer);

        MotorCtrl_SetDuty(uartDuty);
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, uartRxBuffer, sizeof(uartRxBuffer));
    }
}
