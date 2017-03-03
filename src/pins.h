/*
 * pins.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef PINS_H
#define PINS_H

#include "PinNames.h"
#include "mbed.h"

DigitalIn drive_status1(DRV_STS1);
DigitalIn drive_status2(DRV_STS2);
DigitalIn ac_trigger(TRIGG_INPUT2);
DigitalIn drum_limit(LIMIT_SW1);
DigitalIn emergency_stop(EMERGENCY_STOP);

DigitalOut brake(CTL_BRAKE);
DigitalOut clutch(CTL_CLUTCH);
DigitalOut dir_fw(CTL_FW_DIR);
DigitalOut dir_rw(CTL_RW_DIR);
PwmOut motor_ctl(CTL_PWM_MOT);

#endif /* !PINS_H */
