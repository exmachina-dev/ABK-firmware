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
            brake = 1; // brake is active when 1
            break;
        case ABK_DRUM_FREEWHEEL:
            brake = 0;
            break;
    }
}


void ABK_set_motor_mode(ABK_motor_mode_t mode) {
    switch(mode) {
        case ABK_MOTOR_FW:
            dir_fw = 1;
            dir_rw = 0;
            break;
        case ABK_MOTOR_RW:
            dir_fw = 0;
            dir_rw = 1;
            break;
        case ABK_MOTOR_DISABLED:
            dir_fw = 0;
            dir_rw = 0;
            break;
    }
}

int ABK_set_speed(float speed) {
    float dt = 0.5;
    int period = 0;

    period = (int) (1000000.0 / ABK_map(0, 100, ABK_MOT_MIN_FREQ, ABK_MOT_MAX_FREQ, speed));

    if (period > (1000000 / ABK_MOT_MIN_FREQ))
        period = 1000000 / ABK_MOT_MIN_FREQ;
    else if (period < (1000000 / ABK_MOT_MAX_FREQ))
        period = 1000000 / ABK_MOT_MAX_FREQ;

    motor_ctl.period_us(period);
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
    if (res > (float) to_val1 && res > (float) to_val2) {
        if ((float) to_val2 > (float) to_val1)
            return (float) to_val2;
        else
            return (float) to_val1;
    }
    else if (res < (float) to_val2 && res < (float) to_val1) {
        if ((float) to_val2 < (float) to_val1)
            return (float) to_val2;
        else
            return (float) to_val1;
    }

    return res;
}

bool ABK_validate_config(ABK_config_t *config) {
    uint8_t res = 0;

    if ((config->start_time <= config->p1.time) &&
            (config->p1.time <= config->p2.time) &&
            (config->p2.time <= config->p3.time) &&
            (config->p3.time <= config->stop_time))
        res += 1;
    if (config->p1.speed <= 100)
        res += 2;
    if (config->p2.speed <= 100)
        res += 4;
    if (config->p3.speed <= 100)
        res += 8;

    return (res == (1 + 2 + 4 + 8));
}

bool ABK_eeprom_read_config(AT24CXX_I2C *eeprom, ABK_config_t *config) {
    ABK_eeprom_t eedata;

    bool ret = eeprom->read(ABK_EEPROM_START_ADDRESS, eedata.raw, ABK_EEPROM_DATA_SIZE);
    memcpy(config, &eedata.data.config, ABK_EEPROM_CONF_SIZE);

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
    config.p3.time = 0;
    config.p3.speed = 0;
    config.stop_time = 0;
    config.direction = 0;

    memcpy(&eedata.data.config, &config, ABK_EEPROM_CONF_SIZE);
#if defined(ABK_DEBUG) && ABK_DEBUG != 0
    DEBUG_PRINTF("T: ");
    for (int i=0; i<ABK_EEPROM_CONF_SIZE; i++) {
       DEBUG_PRINTF(" %d", eedata.data.config[i]);
    }
    DEBUG_PRINTF("\r\n");
#endif

    bool ret = eeprom->write(ABK_EEPROM_START_ADDRESS, eedata.raw, ABK_EEPROM_DATA_SIZE);

    return ret;
}
