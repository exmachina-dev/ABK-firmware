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
I2C i2c_eeprom(I2C0_SDA, I2C0_SCL);
AT24CXX_I2C eeprom(&i2c_eeprom, 0x50);
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

    ABK_set_motor_mode(ABK_MOTOR_DISABLED);
    ABK_set_speed(0);

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
                ABK_state = ABK_STATE_CONFIGURED;
                printf("Valid config\r\n");
            } else {
                ABK_state = ABK_STATE_NOT_CONFIGURED;
            }
        } else if (ABK_config.state == 0 || ABK_config.state == 255) {
            USBport.printf("Erasing!\r\n");
            ABK_eeprom_erase_config(&eeprom);
            ABK_eeprom_read_config(&eeprom, &ABK_config);
            ABK_state = ABK_STATE_NOT_CONFIGURED;
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
    for (int i=0; i<50; i++) {
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

#elif ABK_MOTOR_TEST
    for (int i=0; i<100; i++) {
        clutch = 0;
        brake = 0;
        ABK_set_speed(i);
        dir_fw = 1;
        dir_rw = 0;
        Thread::wait(200);

        wdog.kick();
    }

    for (int i=100; i>0; i--) {
        clutch = 0;
        brake = 0;
        ABK_set_speed(i);
        dir_fw = 1;
        dir_rw = 0;
        Thread::wait(200);

        wdog.kick();
    }

    dir_fw = 0;
    dir_rw = 0;
#else

    ABK_timer.start();


    ABK_app_thread.start(ABK_app_task);
    ABK_serial_thread.start(ABK_serial_task);

    wdog.kick(1); // Set watchdog to 1s

    while(true) {
        led1 = !led1;
        wdog.kick();

#if ABK_SIMULATE
        if (!ac_trigger && ABK_timer.read_ms() > 5000) {
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
    bool _force_drum_stop = false;
    int _trigger_time = 0U;
    int _fabric_detected_time = 0U;
    int _stime = 0U;
    ABK_config_t _config;

    if (ABK_state == ABK_STATE_CONFIGURED) {
        ABK_config_mutex.lock();
        memcpy(&_config, &ABK_config, sizeof(ABK_config_t));
        USBport.printf("Config copied.\r\n");
        ABK_config_mutex.unlock();

        printf("start %dms\r\n", _config.start_time);
        printf("point1 %dms @%d\r\n", _config.p1.time, _config.p1.speed);
        printf("point2 %dms @%d\r\n", _config.p2.time, _config.p2.speed);
        printf("stop %dms\r\n", _config.stop_time);

        ABK_state = ABK_STATE_RUN;
    }

    while (ABK_state != ABK_STATE_RESET) {
        Thread::wait(ABK_INTERVAL);

        if (ABK_state == ABK_STATE_RUN || ABK_state == ABK_STATE_STANDBY) {
            if (!_triggered && ac_trigger == 0 && drive_status == 1) {
                _triggered = true;
                _trigger_time = ABK_timer.read_ms();    // Store and reset timer: This ensure the timer
                ABK_timer.reset();                      // doesn't overflow after the ABK been trigered (undefined behaviour)
            }

            if (emergency_stop != 1) { // Stop motor on emergency input
                ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                ABK_set_speed(0.0);
                ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                continue;
            }

            if (slowfeed_input == 1) { // Overrides default behavior for loading/unloading
                ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                ABK_set_motor_mode(ABK_MOTOR_RW);
                ABK_set_speed(6);
                _triggered = false;
                ABK_timer.reset();
                continue;
            } else if (!_triggered && (slowfeed_input == 0)) {
                ABK_set_drum_mode(ABK_DRUM_BRAKED);
                ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                ABK_set_speed(0);
                continue;
            }


            _force_drum_stop = (drum_limit == 1) ? true : false;

            if (ABK_state == ABK_STATE_RUN && _triggered) {
                _stime = ABK_timer.read_ms(); // Update time since trigger
                led2 = !led2;
                USBport.printf("stime: %d ", _stime);

                if (_fabric_detected_time == 0 && drum_limit == 0)
                    _fabric_detected_time = _stime;

                if (_stime >= _config.start_time && _stime < _config.p1.time) {
                    if (!_force_drum_stop)
                        ABK_set_drum_mode(ABK_DRUM_ENGAGED);
                    else if (_force_drum_stop && (_stime - _fabric_detected_time) <= ABK_CLUTCH_BRAKE_DELAY)
                        ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    else
                        ABK_set_drum_mode(ABK_DRUM_BRAKED);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.start_time, _config.p1.time,
                           0, _config.p1.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T1 %f", rspeed);
                }
                else if (_stime >= _config.p1.time && _stime < _config.p2.time) {
                    if (!_force_drum_stop)
                        ABK_set_drum_mode(ABK_DRUM_ENGAGED);
                    else if (_force_drum_stop && (_stime - _fabric_detected_time) <= ABK_CLUTCH_BRAKE_DELAY)
                        ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    else
                        ABK_set_drum_mode(ABK_DRUM_BRAKED);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.p1.time, _config.p2.time,
                            _config.p1.speed, _config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T2 %f", rspeed);
                }
                else if (_stime >= _config.p2.time && _stime < _config.stop_time) {
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.p2.time, _config.stop_time,
                            _config.p2.speed, 0, _stime);
                    ABK_set_speed(rspeed);
                    USBport.printf("T3 %f", rspeed);
                }
                else if (_stime >= _config.stop_time) {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                    ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                    USBport.printf("S");
                    _triggered = false;
                    ABK_state = ABK_STATE_STANDBY;
                } else {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_FULLSTOP);
                    ABK_set_motor_mode(ABK_MOTOR_DISABLED);
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
    }
}

static void ABK_serial_task(void) {
    unsigned int res = 0;
    char cmd_buf[10];
    std::string line="";
    std::string cmd;

    while (true) {
        char c;
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
                        USBport.printf("Abrakabuki by ExMachina\r\n");
                        USBport.printf("    version: %s\r\n\r\n", ABK_VERSION);
                        USBport.printf("available commands:\r\n");
                        USBport.printf("    set OPTION VALUE     Set the option to the desired value.\r\n");
                        USBport.printf("                         Integer and float are accepted.\r\n");
                        USBport.printf("         start DELAY     Delay from trigger to start.\r\n");
                        USBport.printf("         p1.time DELAY   Delay from trigger to point 1.\r\n");
                        USBport.printf("         p1.speed DELAY  Speed at point 1.\r\n");
                        USBport.printf("         p2.time DELAY   Delay from trigger to point 2.\r\n");
                        USBport.printf("         p2.speed DELAY  Speed at point 2.\r\n");
                        USBport.printf("         stop DELAY      Delay from trigger to full stop.\r\n");
                        USBport.printf("\r\n");
                        USBport.printf("    get                  Return current configuration\r\n");
                        USBport.printf("    save                 Save configuration to eeprom\r\n");
                        USBport.printf("    reset                Reset the microcontroller\r\n");
                        USBport.printf("    help                 Display this help message\r\n");
                    } else if (cmd == "set") {
                        char opt_str[10];
                        int args;
                        int nargs = sscanf(line.c_str(), "%s %s %d", cmd_buf, opt_str, &args);
                        if (nargs > 2) {
                            USBport.printf("%s set to %d\r\n", opt_str, args);

                            ABK_config_mutex.lock();

                            if (strcmp(opt_str, "start") == 0) {
                                ABK_config.start_time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p1.time") == 0) {
                                ABK_config.p1.time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p1.speed") == 0) {
                                ABK_config.p1.speed = (uint16_t) args;
                            } else if (strcmp(opt_str, "p2.time") == 0) {
                                ABK_config.p2.time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p2.speed") == 0) {
                                ABK_config.p2.speed = (uint16_t) args;
                            } else if (strcmp(opt_str, "stop") == 0) {
                                ABK_config.stop_time = (uint16_t) args;
                            } else {
                                USBport.printf("Unrecognized option: %s.", opt_str);
                            }

                            ABK_config_mutex.unlock();
                        }
                    } else if (cmd == "get") {
                        ABK_config_mutex.lock();

                        USBport.printf("start %d\r\n", ABK_config.start_time);
                        USBport.printf("p1.time %d\r\np1.speed %d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
                        USBport.printf("p2.time %d\r\np2.speed %d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
                        USBport.printf("stop %d\r\n", ABK_config.stop_time);

                        ABK_config_mutex.unlock();
                    } else if (cmd == "save") {
                        ABK_config_mutex.lock();

                        ABK_config.state = 1;

                        if (ABK_eeprom_write_config(&eeprom, &ABK_config))
                            USBport.printf("config saved to EEPROM.");
                        else
                            USBport.printf("error occured during writing to EEPROM.");

                        ABK_config_mutex.unlock();
                    } else if (cmd == "erase") {
                        ABK_config_mutex.lock();

                        if (ABK_eeprom_erase_config(&eeprom))
                            USBport.printf("config erased EEPROM.");
                        else
                            USBport.printf("error occured during erasing.");

                        ABK_config_mutex.unlock();
                    } else if (cmd == "reset") {
                        ABK_reset = true;
                    } else if (cmd == "version") {
                        USBport.printf("%s", ABK_VERSION);
                    } else {
                        USBport.printf("unrecognized command: %s. Type 'help' for help", cmd_buf);
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
