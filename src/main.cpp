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

Ticker ticker_1ms;
Ticker ticker_sync;
Ticker ticker_leds;

// Serial
Serial USBport(ISP_TXD, ISP_RXD);

Thread ABK_app_thread;

int main(void) {

#if MBED_CONF_APP_MEMTRACE
    mbed_stats_heap_t heap_stats;
    mbed_mem_trace_set_callback(mbed_mem_trace_default_callback);
#endif

    motor_ctl.period_us(100);
    motor_ctl = 0.0;

    USBport.baud(115200);

    ticker_1ms.attach_us(&ABK_timer1ms_task, 1000);
    ticker_leds.attach(&ABK_leds_task, 1);

    wdog.kick(10); // First watchdog kick to trigger it

    USBport.printf("|=====================|\r\n");
    USBport.printf("|     Abrakabuki      |\r\n");
    USBport.printf("|    by ExMachina     |\r\n");
    USBport.printf("|=====================|\r\n");
    USBport.printf("\r\n");

#if ABK_SIMULATE
    USBport.printf("SIMULATOR MODE ON\r\n");
#endif

    ABK_app_thread.start(ABK_app_task);

    while(true) {
        led1 = !led1;
        wdog.kick();

        Thread::wait(10);
    }

    // Reset
    mbed_reset();

    return 1;
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
    uint16_t _trigger_time = 0U;
    uint16_t _stime = 0U;
    uint16_t _ptime = 0U;

    // get_eeprom data
    if (true) { // get_eeprom data success
        state = ABK_STATE_CONFIGURED;
    } else {
        state = ABK_STATE_NOT_CONFIGURED;
    }

    while (state != ABK_STATE_RESET) {
        if (state == ABK_STATE_RUN) {
            if (ac_trigger != 0) {
                _triggered = true;
                _trigger_time = ABK_timer1ms;
            }

            if (_triggered) {
                _stime = ABK_timer1ms - _trigger_time; // Update time since trigger

                if (_stime >= config.start_time && _stime < config.p1.time) {
                    ABK_set_drum_mode(ABK_DRUM_ENGAGED);

                    float rspeed = ABK_map(config.start_time, config.p1.time,
                           0, config.p1.speed, _stime);
                    ABK_set_speed(rspeed);
                }
                else if (_stime >= config.p1.time && _stime < config.p2.time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(config.p1.time, config.p2.time,
                            config.p1.speed, config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                }
                else if (_stime >= config.p2.time && _stime < config.stop_time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(config.p1.time, config.p2.time,
                            config.p1.speed, config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                }
                else if (_stime >= config.stop_time) {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                } else {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                }
            } else {
                ABK_set_speed(0);
                ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
            }

        } else {
            ABK_set_speed(0);
            ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
        }
        Thread::wait(1);
    }
}
