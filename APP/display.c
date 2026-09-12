#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "monitor.h"
#include <stdio.h>

static void SendMotorDataFireWater(const MotorDatasType *motorData)
{
    char frame[160];
    int frameLength = snprintf(frame, sizeof(frame),
        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u\n",
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
        (unsigned int)motorData->Hallw);

    if (frameLength > 0 && frameLength < (int)sizeof(frame))
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)frame, (uint16_t)frameLength, 10);
    }
}

void StartDisplayTask(void *argument)
{
    for(;;)
    {
        static MotorDatasType MotorData;
        osMessageQueueGet(MotorDatasQueueHandle, &MotorData, 0, osWaitForever);

        SendMotorDataFireWater(&MotorData);
    }
}