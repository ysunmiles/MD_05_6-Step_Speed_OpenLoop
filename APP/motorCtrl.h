#ifndef __MOTORCTRL_H
#define __MOTORCTRL_H

#define PWM_DUTY    10

typedef enum
{
    MOTOR_DIR_FORWARD = 1,
    MOTOR_DIR_REVERSE = 2
} MotorDirection;

MotorDirection MotorCtrl_GetDirection(void);
void MotorCtrl_PWMCallback(MotorDirection direction);

#endif