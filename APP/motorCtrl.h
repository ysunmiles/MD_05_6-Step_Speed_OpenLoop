#ifndef __MOTORCTRL_H
#define __MOTORCTRL_H

#include <stdint.h>

#define SPEED_AVERAGE_WINDOW 6U

typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_ROTATE_FORWARD = 1,
    MOTOR_ROTATE_REVERSE = 2
} MotorStateType;

// 电机参数结构体
typedef struct {
    MotorStateType MotorState;
    float Speed, SpeedAim;
    uint16_t Duty;
    uint8_t HallSignal;

    float BEMFu, BEMFv, BEMFw;
    float Iu, Iv, Iw;
    float Vbus, Temp;
} MotorDataType;

// 外部调用API
void MotorCtrl_SetDuty(uint16_t cmdDuty);
void MotorCtrl_SetSpeedZero(void);
void MotorCtrl_PWMCallback(void);
MotorDataType* MotorCtrl_GetData(void);

void StartBtnStateTask(void *argument);

#endif