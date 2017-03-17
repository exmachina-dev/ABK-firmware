/*
 * main.cpp
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.h"

#if MBED_CONF_APP_MEMTRACE
#include "mbed_stats.h"
#include "mbed_mem_trace.h"
#endif

// Global variables and objects

Watchdog wdog;

volatile uint16_t   ABK_timer1ms = 0U;   // variable increments each millisecond

extern "C" void mbed_reset();

#if !ABK_TEST
Ticker ticker_1ms;
Ticker ticker_sync;
Ticker ticker_leds;
#endif

// Serial
Serial USBport(ISP_TXD, ISP_RXD);

// EEPROM
#if ABK_HAS_EEPROM
C24LCXX_I2C eeprom(I2C2_SDA, I2C2_SCL, 0xA0);
#endif

#if !ABK_TEST
Thread ABK_app_thread;
#endif

int main(void) {

#if MBED_CONF_APP_MEMTRACE
    mbed_stats_heap_t heap_stats;
    mbed_mem_trace_set_callback(mbed_mem_trace_default_callback);
#endif

    motor_ctl.period_us(100);
    motor_ctl = 0.0;

    USBport.baud(115200);

#if !ABK_TEST
    ticker_1ms.attach_us(&ABK_timer1ms_task, 1000);
    ticker_leds.attach(&ABK_leds_task, 10);
#endif

    wdog.kick(10); // First watchdog kick to trigger it

    USBport.printf("|=====================|\r\n");
    USBport.printf("|     Abrakabuki      |\r\n");
    USBport.printf("|    by ExMachina     |\r\n");
    USBport.printf("|=====================|\r\n");
    USBport.printf("\r\n");

#if ABK_SIMULATE
    USBport.printf("SIMULATOR MODE ON\r\n");
#endif

#if ABK_HAS_EEPROM
    short data = 0;
    USBport.printf("EEPROM TEST\r\n");
    bool ew = eeprom.Write(0, (short)12);
    bool er = eeprom.Read(0, &data);

    USBport.printf("EEPROM W %d R %d D %d\r\n", ew, er, data);
#endif

#if ABK_TEST
    for (int i=0; i<100; i++) {
        led1 = !led1;
        led2 = !led2;

        clutch = !clutch;
        brake = !brake;
        motor_ctl = !motor_ctl;
        dir_fw = !dir_fw;
        dir_rw = !dir_rw;
        Thread::wait(100);

        wdog.kick();
    }
    for (int i=0; i<100; i++) {
        motor_ctl = (float) i / 100.0;
        Thread::wait(100);

        wdog.kick();
    }
    for (int i=100; i>0; i--) {
        motor_ctl = (float) i / 100.0;
        Thread::wait(100);

        wdog.kick();
    }

#else


    ABK_app_thread.start(ABK_app_task);

    while(true) {
        led1 = !led1;
        wdog.kick();

#if ABK_SIMULATE
        if (!ac_trigger && ABK_timer1ms > 2000) {
            ac_trigger = 1;
            USBport.printf("Simulated trigger");
        }
#endif

        Thread::wait(100);
    }

    // Reset
    mbed_reset();

    return 1;
#endif
}

// Timer update task
static void ABK_timer1ms_task(void) {
    ABK_timer1ms++;
}

// Led update task
static void ABK_leds_task(void) {
    led2 = !led2;
}

static void ABK_app_task(void) {
    ABK_state_t state = ABK_STATE_STANDBY;
    ABK_config_t config;

    bool _triggered = false;
    uint32_t _trigger_time = 0U;
    uint16_t _stime = 0U;
    uint16_t _ptime = 0U;

    // get_eeprom data
    if (ABK_eeprom_read_config(&eeprom, &config)) { // get_eeprom data success
        state = ABK_STATE_CONFIGURED;
        printf("Configured:\r\n");
        printf("state %dms\r\n", config.state);
        printf("start %dms\r\n", config.start_time);
        printf("point1 %dms @%d\r\n", config.p1.time, config.p1.speed);
        printf("point2 %dms @%d\r\n", config.p2.time, config.p2.speed);
        printf("stop %dms\r\n", config.stop_time);
        if (config.state == 1) {
            state = ABK_STATE_RUN;
        } else if (config.state == 32) {
            USBport.printf("Erasing!\r\n");
            ABK_eeprom_erase_config(&eeprom);
        }
    } else {
        state = ABK_STATE_NOT_CONFIGURED;
        printf("Not configured\r\n");
#if ABK_SIMULATE
        config.state = 1;
        config.start_time = 1000;
        config.p1.time = 2500;
        config.p1.speed = 80;
        config.p2.time = 5000;
        config.p2.speed = 100;
        config.stop_time = 10000;
        state = ABK_STATE_RUN;
        printf("Forced config:\r\n");
        printf("start %dms\r\n", config.start_time);
        printf("point1 %dms @%d\r\n", config.p1.time, config.p1.speed);
        printf("point2 %dms @%d\r\n", config.p2.time, config.p2.speed);
        printf("stop %dms\r\n", config.stop_time);
#endif
    }

    while (state != ABK_STATE_RESET) {
        if (state == ABK_STATE_RUN) {
            if (!_triggered && ac_trigger != 0) {
                _triggered = true;
                _trigger_time = ABK_timer1ms;
            }

            if (_triggered) {
                _stime = ABK_timer1ms - _trigger_time; // Update time since trigger
                led1 = !led1;
                USBport.printf("stime: %d ", _stime);

                if (_stime >= config.start_time && _stime < config.p1.time) {
                    ABK_set_drum_mode(ABK_DRUM_ENGAGED);

                    float rspeed = ABK_map(config.start_time, config.p1.time,
                           0, config.p1.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T1 %f", rspeed);
                }
                else if (_stime >= config.p1.time && _stime < config.p2.time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(config.p1.time, config.p2.time,
                            config.p1.speed, config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T2 %f", rspeed);
                }
                else if (_stime >= config.p2.time && _stime < config.stop_time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(config.p2.time, config.stop_time,
                            config.p2.speed, 0, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T3 %f", rspeed);
                }
                else if (_stime >= config.stop_time) {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                    USBport.printf("S");
                } else {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                    USBport.printf("U");
                }
                USBport.printf("\r\n");
            } else {
                ABK_set_speed(0);
                ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
            }

        } else {
            ABK_set_speed(0);
            ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
        }
        Thread::wait(ABK_INTERVAL);
    }
}
