/*
 * config.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CONFIG_H
#define CONFIG_H

#define ABK_VERSION         "v2.0"
#define ABK_HAS_LCD         0
#define ABK_HAS_EEPROM      1
#define ABK_HAS_USBSERIAL   1

#define ABK_SIMULATE        0
#define ABK_TEST            0
#define ABK_MOTOR_TEST      0
#define ABK_DEBUG           1

#define ABK_INTERVAL        (10)
#define ABK_SERIAL_INTERVAL (10)

#endif /* !CONFIG_H */
