/*
 * ABKcontrol.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ABKCONTROL_H
#define ABKCONTROL_H

#include "mbed.h"

#include "AT24Cxx_I2C.h"

#define ABK_MOT_MIN_DT      (10)
#define ABK_MOT_MAX_DT      (80)

#define ABK_EEPROM_VERSION          (2)
#define ABK_EEPROM_STATE_BLANK      (0)
#define ABK_EEPROM_STATE_PRESENT    (1)
#define ABK_EEPROM_CONF_SIZE        (16)
#define ABK_EEPROM_DATA_SIZE        (ABK_EEPROM_CONF_SIZE + 2)
#define ABK_EEPROM_START_ADDRESS    (0)

extern DigitalOut brake;
extern DigitalOut clutch;
extern DigitalOut dir_fw;
extern DigitalOut dir_rw;
extern PwmOut motor_ctl;

struct ABK_eeprom_data_s {
    unsigned char eeprom_version;
    unsigned char eeprom_state;
    unsigned char config[ABK_EEPROM_CONF_SIZE];
} __attribute__((packed));

typedef ABK_eeprom_data_s ABK_eeprom_data_t;

typedef union {
    unsigned char raw[ABK_EEPROM_DATA_SIZE];
    ABK_eeprom_data_t data;
} ABK_eeprom_t;

struct ABK_point_s {
    uint16_t time;
    uint16_t speed;
} __attribute__((packed));

typedef struct ABK_point_s ABK_point_t;

struct ABK_config_s {
    uint8_t state;
    uint16_t start_time;
    ABK_point_t p1;
    ABK_point_t p2;
    uint16_t stop_time;
    uint8_t unused;
} __attribute__((packed));

typedef struct ABK_config_s ABK_config_t;

typedef enum {
    ABK_STATE_STANDBY = 0,
    ABK_STATE_NOT_CONFIGURED,
    ABK_STATE_CONFIGURED,
    ABK_STATE_RUN,
    ABK_STATE_RESET,
} ABK_state_t;

typedef enum {
    ABK_DRUM_BRAKED = 0,
    ABK_DRUM_FREEWHEEL,
    ABK_DRUM_ENGAGED,
    ABK_DRUM_FULLSTOP
} ABK_drum_mode_t;

typedef enum {
    ABK_MOTOR_DISABLED = 0,
    ABK_MOTOR_FW,
    ABK_MOTOR_RW
} ABK_motor_mode_t;

void ABK_set_drum_mode(ABK_drum_mode_t);
void ABK_set_motor_mode(ABK_motor_mode_t);
int ABK_set_speed(float speed);

float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, int value);
float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, float value);

bool ABK_validate_config(ABK_config_t *config);

bool ABK_eeprom_read_config(AT24CXX_I2C *eeprom, ABK_config_t *config);
bool ABK_eeprom_write_config(AT24CXX_I2C *eeprom, ABK_config_t *config);
bool ABK_eeprom_erase_config(AT24CXX_I2C *eeprom);

#endif /* !ABKCONTROL_H */
