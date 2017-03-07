/*
 * ABKcontrol.cpp
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#include "ABKcontrol.h"

void ABK_set_drum_mode(ABK_drum_mode_t mode) {
    switch(mode) {
        case ABK_DRUM_BRAKED:
            clutch = 0;
            brake = 0; // brake is active when 0
            break;
        case ABK_DRUM_FREEWHEEL:
            clutch = 0;
            brake = 1;
            break;
        case ABK_DRUM_FULLSTOP:
            clutch = 1;
            brake = 0;
            break;
        case ABK_DRUM_ENGAGED:
            clutch = 1;
            brake = 1;
            break;
    }
}

int ABK_set_speed(float speed) {
    float dt = 0.0;

    dt = ABK_map(0, 100, ABK_MOT_MIN_DT, ABK_MOT_MAX_DT, speed);
    dt = dt / 100;

    if (dt > 1.0)
        dt = 1.0;
    else if (dt < 0.0)
        dt = 0;

    motor_ctl = dt;
    return 0;
}

float ABK_map(int from_min, int from_max, int to_min, int to_max, int value) {
    return to_min + (float) ((value-from_min)/(from_max-from_min)) * (float) (to_max-to_min);
}
