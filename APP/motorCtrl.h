#ifndef __MOTORCTRL_H
#define __MOTORCTRL_H

typedef enum
{
    MOTOR_DIR_FORWARD = 1,
    MOTOR_DIR_REVERSE = 2
} MotorDirection;

MotorDirection MotorCtrl_GetDirection(void);
void MotorCtrl_PWMCallback(MotorDirection direction);
uint8_t MotorCtrl_GetHall(void);
void MotorCtrl_SetDuty(uint16_t uartDuty);
uint16_t MotorCtrl_GetDuty(void);

#endif