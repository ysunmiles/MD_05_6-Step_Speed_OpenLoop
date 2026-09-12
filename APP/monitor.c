#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "monitor.h"
#include <math.h>

static float calcTemp(uint16_t ADCVtempValue)
{
    const float Vtemp = (float)ADCVtempValue / 4095.0 * 3.3;
    const float Rt = 4.7*3.3/Vtemp - 4.7;

    const float temperatureK = 1.0f /
        (logf(Rt/10)/3380 + 1.0f/(25+273.15f));

    return temperatureK - 273.15f;
}

void StartMonitorTask(void *argument)
{
    static uint16_t ADC1Data[4] = {0};
    static uint16_t ADC3Data[4] = {0};
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC1Data, 4);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)ADC3Data, 4);
    

    for(;;)
    {
        static MotorDatasType MotorData;
        
        MotorData.speed = (float)ulTaskNotifyTake(pdTRUE, osWaitForever);
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

        osMessageQueuePut(MotorDatasQueueHandle, &MotorData, 0, osWaitForever);
    }

}

