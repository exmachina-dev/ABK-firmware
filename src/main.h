/*
 * main.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MAIN_H
#define MAIN_H

#include "watchdog.h"

#include "ABKcontrol.h"
#include "pins.h"

#include "mbed.h"

static void ABK_timer1ms_task(void);
static void ABK_leds_task(void);
static void ABK_app_task(void);

#endif /* !MAIN_H */
