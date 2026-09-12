#ifndef __MONITOR_H
#define __MONITOR_H

#include <stdint.h>

typedef struct {
    float BEMFu, BEMFv, BEMFw, Iu, Iv, Iw, Vbus, temp, speed;
    uint8_t Hallu, Hallv, Hallw;
} MotorDatasType;

void StartMonitorTask(void *argument);

#endif