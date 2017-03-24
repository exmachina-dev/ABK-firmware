/*
 * main.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MAIN_H
#define MAIN_H

#include "config.h"

#include "watchdog.h"

#include "ABKcontrol.h"
#include "pins.h"

#include "mbed.h"

#if ABK_HAS_LCD
#include "TextLCD.h"
#endif

#if ABK_HAS_EEPROM
#include "AT24Cxx_I2C.h"
#endif

static void ABK_leds_task(void);
static void ABK_app_task(void);
static void ABK_serial_task(void);

#endif /* !MAIN_H */
