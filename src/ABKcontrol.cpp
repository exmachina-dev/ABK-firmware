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

float ABK_map(int from_min, int from_max, int to_min, int to_max, int value) {
    float res = 0.0;
    res = to_min + (float) ((value-from_min)/(from_max-from_min)) * (float) (to_max-to_min);

    if (res > (float) to_max)
        return (float) to_max;
    else if (res < (float) to_min)
        return (float) to_min;

    return res;
}

bool ABK_eeprom_read_config(C24LCXX_I2C *eeprom, ABK_config_t *config) {
    bool ret;

    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS, (unsigned char *)config->state);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 1, (short *)config->start_time);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 3, (short *)config->p1.time);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 5, (short *)config->p1.speed);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 7, (short *)config->p2.time);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 9, (short *)config->p2.speed);
    ret = eeprom->Read(ABK_EEPROM_START_ADDRESS + 11, (short *)config->stop_time);

    return ret;
}

bool ABK_eeprom_write_config(C24LCXX_I2C *eeprom, ABK_config_t *config) {
    bool ret;

    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS, (unsigned char)config->state);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 1, (short)config->start_time);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 3, (short)config->p1.time);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 5, (short)config->p1.speed);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 7, (short)config->p2.time);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 9, (short)config->p2.speed);
    ret = eeprom->Write(ABK_EEPROM_START_ADDRESS + 11, (short)config->stop_time);

    return ret;
}

bool ABK_eeprom_erase_config(C24LCXX_I2C *eeprom) {
    return eeprom->EraseMemoryArea(ABK_EEPROM_START_ADDRESS, ABK_EEPROM_START_ADDRESS + 12);
}
