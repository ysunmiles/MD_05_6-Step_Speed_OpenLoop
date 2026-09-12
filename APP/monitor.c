#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "monitor.h"
#include "OLED.h"
#include <math.h>
#include <stdio.h>

#define SPEED_AVERAGE_WINDOW 6U

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
        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%.3f\n",
        
        (double)motorData->BEMFu,
        (double)motorData->BEMFv,
        (double)motorData->BEMFw,
        (double)motorData->Iu,
        (double)motorData->Iv,
        (double)motorData->Iw,
        (double)motorData->Vbus,
        (double)motorData->temp,
        (unsigned int)motorData->Hallu,
        (unsigned int)motorData->Hallv,
        (unsigned int)motorData->Hallw,
        (double)motorData->speed);

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

    for(;;)
    {
        MotorDatasType MotorData;
        uint32_t speedx100;
        static float speedSample;

        if (xTaskNotifyWait(0, UINT32_MAX, &speedx100, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            speedSample = (float)speedx100 / 100.0f;
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

        MotorData.Hallu = HAL_GPIO_ReadPin(HALLU_GPIO_Port, HALLU_Pin);
        MotorData.Hallv = HAL_GPIO_ReadPin(HALLV_GPIO_Port, HALLV_Pin);
        MotorData.Hallw = HAL_GPIO_ReadPin(HALLW_GPIO_Port, HALLW_Pin);

        SendMotorDataFireWater(&MotorData);

        OLED_ShowString(1, 7, "          ");
        OLED_ShowFloat(1, 7, MotorData.speed, 2);

        osDelay(20);
    }

}
