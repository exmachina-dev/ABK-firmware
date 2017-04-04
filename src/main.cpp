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

volatile uint32_t   ABK_timer1ms = 0U;   // variable increments each millisecond

extern "C" void mbed_reset();

#if !ABK_TEST
Ticker ticker_sync;
Ticker ticker_leds;
#endif

Timer ABK_timer;

// Serial
#if ABK_HAS_USBSERIAL
Serial USBport(ISP_TXD, ISP_RXD);
#else
Serial USBport(ISP_TXD, ISP_RXD);
#endif

// EEPROM
#if ABK_HAS_EEPROM
AT24CXX_I2C eeprom(I2C0_SDA, I2C0_SCL, 0x50);
#endif

Mutex ABK_config_mutex;
ABK_config_t ABK_config;
ABK_state_t ABK_state;
bool ABK_reset = false;

#if !ABK_TEST
Thread ABK_app_thread;
Thread ABK_serial_thread;
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
    ABK_config_mutex.lock();

    // get_eeprom data
    if (ABK_eeprom_read_config(&eeprom, &ABK_config)) { // get_eeprom data success
        ABK_state = ABK_STATE_CONFIGURED;
        printf("Configured:\r\n");
        printf("state %d\r\n", ABK_config.state);
        printf("start %dms\r\n", ABK_config.start_time);
        printf("point1 %dms @%d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
        printf("point2 %dms @%d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
        printf("stop %dms\r\n", ABK_config.stop_time);
        if (ABK_config.state == 1) {
            if (ABK_validate_config(&ABK_config)) {
                ABK_state = ABK_STATE_RUN;
            } else {
                ABK_state = ABK_STATE_NOT_CONFIGURED;
            }
        } else if (ABK_config.state == 0 || ABK_config.state == 255) {
            USBport.printf("Erasing!\r\n");
            ABK_eeprom_erase_config(&eeprom);
        }
    } else {
        ABK_state = ABK_STATE_NOT_CONFIGURED;
        printf("Not ABK_configured\r\n");
#if ABK_SIMULATE
        ABK_config.state = 1;
        ABK_config.start_time = 1000;
        ABK_config.p1.time = 2500;
        ABK_config.p1.speed = 80;
        ABK_config.p2.time = 5000;
        ABK_config.p2.speed = 100;
        ABK_config.stop_time = 10000;
        ABK_state = ABK_STATE_RUN;
        printf("Forced ABK_config:\r\n");
        printf("start %dms\r\n", ABK_config.start_time);
        printf("point1 %dms @%d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
        printf("point2 %dms @%d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
        printf("stop %dms\r\n", ABK_config.stop_time);
#endif
    }

    ABK_config_mutex.unlock();
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
        Thread::wait(50);

        wdog.kick();
    }
    for (int i=100; i>0; i--) {
        motor_ctl = (float) i / 100.0;
        Thread::wait(50);

        wdog.kick();
    }

    short test_data = 0;
    bool test_ew = eeprom.write(0, (short)12);
    bool test_er = eeprom.read(0, &test_data);

    if (test_ew && test_er) {
        led1 = 1;
        brake = 1;
        dir_fw = 1;
    } else {
        dir_fw = 1;
        dir_rw = 1;
    }

#else

    ABK_timer.start();


    ABK_app_thread.start(ABK_app_task);
    ABK_serial_thread.start(ABK_serial_task);

    wdog.kick(1); // Set watchdog to 1s

    while(true) {
        led1 = !led1;
        wdog.kick();

#if ABK_SIMULATE
        if (!ac_trigger && ABK_timer1ms > 2000) {
            ac_trigger = 1;
            USBport.printf("Simulated trigger");
        }
#endif

        if (ABK_reset == true) {
            mbed_reset();
            break;
        }

        Thread::wait(100);
    }

    // Reset
    mbed_reset();

    return 1;
#endif
}

// Led update task
static void ABK_leds_task(void) {
    led2 = !led2;
}

static void ABK_app_task(void) {
    bool _triggered = false;
    int _trigger_time = 0U;
    int _stime = 0U;
    ABK_config_t _config;

    if (ABK_state == ABK_STATE_CONFIGURED) {
        ABK_config_mutex.lock();
        memcpy(&_config, &ABK_config, sizeof(ABK_config_t));
        USBport.printf("Config copied.\r\n");
        ABK_config_mutex.unlock();
    }

    while (ABK_state != ABK_STATE_RESET) {
        if (ABK_state == ABK_STATE_RUN) {
            if (!_triggered && ac_trigger != 0) {
                _triggered = true;
                _trigger_time = ABK_timer.read_ms();
                ABK_timer.reset();
            }

            if (_triggered) {
                _stime = ABK_timer.read_ms(); // Update time since trigger
                led1 = !led1;
                USBport.printf("stime: %d ", _stime);

                if (_stime >= _config.start_time && _stime < _config.p1.time) {
                    ABK_set_drum_mode(ABK_DRUM_ENGAGED);

                    float rspeed = ABK_map(_config.start_time, _config.p1.time,
                           0, _config.p1.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T1 %f", rspeed);
                }
                else if (_stime >= _config.p1.time && _stime < _config.p2.time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(_config.p1.time, _config.p2.time,
                            _config.p1.speed, _config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T2 %f", rspeed);
                }
                else if (_stime >= _config.p2.time && _stime < _config.stop_time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);

                    float rspeed = ABK_map(_config.p2.time, _config.stop_time,
                            _config.p2.speed, 0, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T3 %f", rspeed);
                }
                else if (_stime >= _config.stop_time) {
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

static void ABK_serial_task(void) {
    unsigned int res = 0;
    char cmd_buf[10];
    std::string line="";
    std::string cmd;

    while (true) {
        char c;
        float args[4];
        if (USBport.readable() > 0) {
            c = USBport.getc();
            USBport.putc(c);
            if (c == '\r' || c == '\n') {
                if (c == '\r')
                    USBport.putc('\n');

                res = sscanf(line.c_str(), "%s", cmd_buf);

                if (res > 0) {
                    cmd = cmd_buf;

                    if (cmd == "help") {
                        USBport.printf("Help !");
                    } else if (cmd == "set") {
                        char opt_str[10];
                        int nargs = sscanf(line.c_str(), "%s %s %f %f %f %f", cmd_buf, opt_str, &args[0], &args[1], &args[2], &args[3]);
                        if (nargs > 2) {
                            USBport.printf("Set %s to  %f !", opt_str, args[0]);

                            ABK_config_mutex.lock();

                            if (strcmp(opt_str, "start") == 0) {
                                ABK_config.start_time = (uint16_t) args[0];
                            } else if (strcmp(opt_str, "p1.time") == 0) {
                                ABK_config.p1.time = (uint16_t) args[0];
                            } else if (strcmp(opt_str, "p1.speed") == 0) {
                                ABK_config.p1.speed = (uint16_t) args[0];
                            } else if (strcmp(opt_str, "p2.time") == 0) {
                                ABK_config.p2.time = (uint16_t) args[0];
                            } else if (strcmp(opt_str, "p2.speed") == 0) {
                                ABK_config.p2.speed = (uint16_t) args[0];
                            } else if (strcmp(opt_str, "stop") == 0) {
                                ABK_config.stop_time = (uint16_t) args[0];
                            } else {
                                USBport.printf("Unrecognized option: %s.", opt_str);
                            }

                            ABK_config_mutex.unlock();
                        }
                    } else if (cmd == "save") {
                        ABK_config_mutex.lock();

                        if (ABK_eeprom_write_config(&eeprom, &ABK_config))
                            USBport.printf("Config saved to EEPROM.\r\n");
                        else
                            USBport.printf("Error occured during writing to EEPROM.\r\n");


                        ABK_config_mutex.unlock();
                    } else if (cmd == "reset") {
                        ABK_reset = true;
                    } else {
                        USBport.printf("Unrecognized command: %s. Type 'help' for help", cmd_buf);
                    }
                    USBport.printf("\r\n");
                }

                line = ""; // Empty buffer
            } else {
                line += c;
            }
        }


        Thread::wait(50);
    }
}
