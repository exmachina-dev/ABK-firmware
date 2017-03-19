/*
 * ABKcontrol.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ABKCONTROL_H
#define ABKCONTROL_H

#include "mbed.h"

#include "24LCxx_I2C.h"

#define ABK_MOT_MIN_DT      (50)
#define ABK_MOT_MAX_DT      (80)

#define ABK_EEPROM_VERSION          (01)
#define ABK_EEPROM_STATE_BLANK      (00)
#define ABK_EEPROM_STATE_PRESENT    (01)
#define ABK_EEPROM_DATA_SIZE        (16)
#define ABK_EEPROM_CONF_SIZE        (14)
#define ABK_EEPROM_START_ADDRESS    (0)

extern DigitalOut brake;
extern DigitalOut clutch;
extern DigitalOut dir_fw;
extern DigitalOut dir_rw;
extern PwmOut motor_ctl;

typedef struct {
    unsigned char eeprom_version;
    unsigned char eeprom_state;
    unsigned char config[ABK_EEPROM_CONF_SIZE];
} ABK_eeprom_data_t;

typedef union {
    unsigned char raw[ABK_EEPROM_DATA_SIZE];
    ABK_eeprom_data_t data;
} ABK_eeprom_t;

typedef struct {
    uint16_t time;
    uint16_t speed;
} ABK_point_t;

typedef struct {
    uint8_t state;
    uint16_t start_time;
    ABK_point_t p1;
    ABK_point_t p2;
    uint16_t stop_time;
    uint8_t unused;
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

float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, int value);
float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, float value);

bool ABK_eeprom_read_config(C24LCXX_I2C *eeprom, ABK_config_t *config);
bool ABK_eeprom_write_config(C24LCXX_I2C *eeprom, ABK_config_t *config);
bool ABK_eeprom_erase_config(C24LCXX_I2C *eeprom);

#endif /* !ABKCONTROL_H */
