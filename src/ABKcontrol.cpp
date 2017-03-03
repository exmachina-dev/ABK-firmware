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

