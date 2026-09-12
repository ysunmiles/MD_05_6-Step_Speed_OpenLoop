#ifndef __MOTORCTRL_H
#define __MOTORCTRL_H

#define PWM_DUTY    10

typedef enum
{
    MOTOR_DIR_FORWARD = 1,
    MOTOR_DIR_REVERSE = 2
} MotorDirection;

void motorCtrl_SetDirection(MotorDirection direction);
MotorDirection motorCtrl_GetDirection(void);
void motorCtrl_PWMCallback(MotorDirection direction);

#endif