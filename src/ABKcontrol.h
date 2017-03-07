/*
 * ABKcontrol.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ABKCONTROL_H
#define ABKCONTROL_H

#include "mbed.h"

#define ABK_MOT_MIN_DT      (50)
#define ABK_MOT_MAX_DT      (80)

extern DigitalOut brake;
extern DigitalOut clutch;
extern DigitalOut dir_fw;
extern DigitalOut dir_rw;
extern PwmOut motor_ctl;

typedef struct {
    uint16_t time;
    uint8_t speed;
} ABK_point_t;

typedef struct {
    uint16_t start_time;
    ABK_point_t p1;
    ABK_point_t p2;
    uint16_t stop_time;
} ABK_config_t;

typedef enum {
    ABK_STATE_STANDBY = 0,
    ABK_STATE_NOT_CONFIGURED,
    ABK_STATE_CONFIGURED,
    ABK_STATE_RUN,
    ABK_STATE_RESET,
} ABK_state_t;

typedef enum {
    ABK_DRUM_BRAKED,
    ABK_DRUM_FREEWHEEL,
    ABK_DRUM_ENGAGED,
    ABK_DRUM_FULLSTOP
} ABK_drum_mode_t;

void ABK_set_drum_mode(ABK_drum_mode_t);
int ABK_set_speed(float speed);

float ABK_map(int from_min, int from_max, int to_min, int to_max, int value);

#endif /* !ABKCONTROL_H */
