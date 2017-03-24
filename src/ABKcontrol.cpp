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

float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, int value) {
    return ABK_map(from_val1, from_val2, to_val1, to_val2, (float) value);
}

float ABK_map(int from_val1, int from_val2, int to_val1, int to_val2, float value) {
    float res = 0.0;
    res = to_val1 + (float) ((value-from_val1)/(from_val2-from_val1)) * (float) (to_val2-to_val1);

    // Don't clamp to to_val1 and to_val2, check to_val2 < res < to_val1 also
    if (res < (float) to_val1 || res > (float) to_val2) {
        if (res < (float) to_val1)
            return (float) to_val1;
        else if (res > (float) to_val2)
            return (float) to_val2;
    }
    else if (res < (float) to_val2 || res > (float) to_val1) {
        if (res < (float) to_val2)
            return (float) to_val2;
        else if (res > (float) to_val1)
            return (float) to_val1;
    }

    return res;
}

bool ABK_eeprom_read_config(AT24CXX_I2C *eeprom, ABK_config_t *config) {
    ABK_eeprom_t eedata;

    bool ret = eeprom->read(ABK_EEPROM_START_ADDRESS, eedata.raw, ABK_EEPROM_DATA_SIZE);
    memcpy(config, &eedata.data.config, ABK_EEPROM_CONF_SIZE);

    printf("ptr: %d\r\n", (int)config);
    printf("ptr: %d\r\n", (int)eedata.raw);
    printf("raw");
    for (int i=0; i<ABK_EEPROM_DATA_SIZE; i++)
        printf(" %d", eedata.raw[i]);
    printf("\r\n");

    return ret;
}

bool ABK_eeprom_write_config(AT24CXX_I2C *eeprom, ABK_config_t *config) {
    ABK_eeprom_t eedata;
    eedata.data.eeprom_version = ABK_EEPROM_VERSION;
    eedata.data.eeprom_state = ABK_EEPROM_STATE_PRESENT;
    memcpy(eedata.data.config, config, ABK_EEPROM_CONF_SIZE);

    bool ret = eeprom->write(ABK_EEPROM_START_ADDRESS, eedata.raw, ABK_EEPROM_DATA_SIZE);

    return ret;
}

bool ABK_eeprom_erase_config(AT24CXX_I2C *eeprom) {
    bool ret;
    char zdata[ABK_EEPROM_DATA_SIZE];
    for (int i=0; i<ABK_EEPROM_DATA_SIZE; i++)
        zdata[i] = 1;

    ret = eeprom->write(ABK_EEPROM_START_ADDRESS, zdata, ABK_EEPROM_DATA_SIZE);
    printf("%d", ret);
    ABK_eeprom_t eedata;
    ABK_config_t config;
    eedata.data.eeprom_version = ABK_EEPROM_VERSION;
    eedata.data.eeprom_state = ABK_EEPROM_STATE_BLANK;

    config.state = 0;
    config.start_time = 0;
    config.p1.time = 0;
    config.p1.speed = 0;
    config.p2.time = 0;
    config.p2.speed = 0;
    config.stop_time = 0;

    memcpy(&eedata.data.config, &config, ABK_EEPROM_CONF_SIZE);

    printf("orig");
    for (int i=0; i<ABK_EEPROM_DATA_SIZE; i++)
        printf(" %d", eedata.raw[i]);
    printf("\r\n");

    ret = eeprom->write(ABK_EEPROM_START_ADDRESS, eedata.raw, ABK_EEPROM_DATA_SIZE);
    printf("%d", ret);

    unsigned char rdata[ABK_EEPROM_DATA_SIZE];
    eeprom->read(ABK_EEPROM_START_ADDRESS, rdata, ABK_EEPROM_DATA_SIZE);
    printf("%d", ret);

    printf("raw");
    for (int i=0; i<ABK_EEPROM_DATA_SIZE; i++)
        printf(" %d", rdata[i]);
    printf("\r\n");
    return ret;
}
