/*
 * main.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MAIN_H
#define MAIN_H

#include "PinNames.h"
#include "mbed.h"

#include "watchdog.h"

#include "ABKcontrol.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);

static void ABK_timer1ms_task(void);
static void ABK_leds_task(void);

#endif /* !MAIN_H */
