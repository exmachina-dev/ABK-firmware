/*
 * ABKcontrol.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ABKCONTROL_H
#define ABKCONTROL_H

#include "pins.h"

typedef enum {
    ABK_DRUM_BRAKED,
    ABK_DRUM_FREEWHEEL,
    ABK_DRUM_ENGAGED,
    ABK_DRUM_FULLSTOP
} ABK_drum_mode_t;

void ABK_set_drum_mode(ABK_drum_mode_t);

#endif /* !ABKCONTROL_H */
