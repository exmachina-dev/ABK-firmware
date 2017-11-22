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
Timer ABK_leds_timer;

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
ABK_state_t ABK_state = ABK_STATE_NOT_CONFIGURED;
uint8_t ABK_error = ABK_ERROR_NONE;
unsigned int EXM_previous_time[3];

bool ABK_reset = false;
uint8_t ABK_slowfeed = 0;

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

    brake = 0;
    motor_ctl = 0;
    dir_fw = 0;
    dir_rw = 0;

    USBport.baud(115200);

#if ABK_TEST
    DigitalOut output1_1(OUTPUT1_1);
    DigitalOut output1_2(OUTPUT1_2);
    DigitalOut output1_3(OUTPUT1_3);
    DigitalOut output1_4(OUTPUT1_4);

    DigitalOut output2_1(OUTPUT2_1);
    DigitalOut output2_2(OUTPUT2_2);
    DigitalOut output2_3(OUTPUT2_3);
    DigitalOut output2_4(OUTPUT2_4);
#else
    ticker_leds.attach_us(&ABK_leds_task, 50000);
    ABK_leds_timer.start();
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
        ABK_error = REMOVE_FLAG(ABK_error, ABK_ERROR_NOT_CONFIGURED);
        printf("Configured:\r\n");
        printf("state %d\r\n", ABK_config.state);
        printf("start %dms\r\n", ABK_config.start_time);
        printf("point1 %dms @%d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
        printf("point2 %dms @%d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
        printf("point3 %dms @%d\r\n", ABK_config.p3.time, ABK_config.p3.speed);
        printf("stop %dms\r\n", ABK_config.stop_time);
        if (ABK_config.state == 1) {
            if (ABK_validate_config(&ABK_config)) {
                ABK_state = ABK_STATE_CONFIGURED;
                printf("Valid config\r\n");
            } else {
                ABK_state = ABK_STATE_NOT_CONFIGURED;
                ABK_error = ADD_FLAG(ABK_error, ABK_ERROR_INVALID_CONFIG);
            }
        } else if (ABK_config.state == 0 || ABK_config.state == 255) {
            USBport.printf("Erasing!\r\n");
            ABK_eeprom_erase_config(&eeprom);
            ABK_eeprom_read_config(&eeprom, &ABK_config);
            ABK_state = ABK_STATE_NOT_CONFIGURED;
        }
    } else {
        ABK_state = ABK_STATE_NOT_CONFIGURED;
        ABK_error = ADD_FLAG(ABK_error, ABK_ERROR_NOT_CONFIGURED);
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
        DEBUG_PRINTF("Forced ABK_config:\r\n");
        DEBUG_PRINTF("start %dms\r\n", ABK_config.start_time);
        DEBUG_PRINTF("point1 %dms @%d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
        DEBUG_PRINTF("point2 %dms @%d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
        DEBUG_PRINTF("point3 %dms @%d\r\n", ABK_config.p3.time, ABK_config.p3.speed);
        DEBUG_PRINTF("stop %dms\r\n", ABK_config.stop_time);
#endif
    }

    ABK_config_mutex.unlock();
#endif

#if ABK_TEST
    uint16_t period = 0;
    while (true) {
        led1 = (period % 2 == 0) ? true : false;
        led2 = (period % 4 == 0) ? true : false;

        output1_1 = !output1_1;
        output1_2 = !output1_2;
        output1_3 = !output1_3;
        output1_4 = !output1_4;

        output2_1 = !output2_1;
        output2_2 = !output2_2;
        output2_3 = !output2_3;
        output2_4 = !output2_4;

        period++;
        Thread::wait(2000);

        wdog.kick();
    }

#elif ABK_MOTOR_TEST
    for (int i=0; i<100; i++) {
        brake = 0;
        ABK_set_speed(i);
        dir_fw = 1;
        dir_rw = 0;
        Thread::wait(200);

        wdog.kick();
    }

    for (int i=100; i>0; i--) {
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

void EXM_blink_led(DigitalOut led, uint8_t led_index, unsigned int interval, int time) {
    if (interval == 0) {
        led = 0;
        return;
    }

    if ((time - EXM_previous_time[led_index]) >= interval){ 
        led = !led;
        EXM_previous_time[led_index]= time;
    }
}
// Led update task
static void ABK_leds_task(void) {
    int current_time = ABK_leds_timer.read_ms();
    EXM_blink_led(led2, 0, ABK_state * 100, current_time);

    if (ABK_error == ABK_ERROR_NONE)
        switch (ABK_state) {
            case ABK_STATE_READY:
                led_sts = 1;
                break;
            case ABK_STATE_SLOWFEED:
                EXM_blink_led(led_sts, 1, 500, current_time);
                break;
            case ABK_STATE_RUN:
                EXM_blink_led(led_sts, 1, 250, current_time);
                break;
            default:
                led_sts = 0;
        }
    else
        led_sts = 0;

    EXM_blink_led(led_err, 2, ABK_error * 100, current_time);
}

static void ABK_app_task(void) {
    bool _triggered = false;
    int _trigger_time = 0U;
    ABK_config_t _config;

    if (ABK_state == ABK_STATE_CONFIGURED) {
        ABK_config_mutex.lock();
        memcpy(&_config, &ABK_config, sizeof(ABK_config_t));
        USBport.printf("Config copied.\r\n");
        ABK_config_mutex.unlock();

        printf("start %dms\r\n", _config.start_time);
        printf("point1 %dms @%d\r\n", _config.p1.time, _config.p1.speed);
        printf("point2 %dms @%d\r\n", _config.p2.time, _config.p2.speed);
        printf("point3 %dms @%d\r\n", _config.p3.time, _config.p3.speed);
        printf("stop %dms\r\n", _config.stop_time);

        ABK_state = ABK_STATE_READY;
    }

    while (ABK_state != ABK_STATE_RESET) {
        Thread::wait(ABK_INTERVAL);

        if (ABK_state == ABK_STATE_NOT_CONFIGURED) {
            ABK_error = ADD_FLAG(ABK_error, ABK_ERROR_INVALID_CONFIG);
        } else {
            ABK_error = REMOVE_FLAG(ABK_error, ABK_ERROR_INVALID_CONFIG);
        }

        if ((bool) !drive_status) { // Stop motor on VFD error
            ABK_error = ADD_FLAG(ABK_error, ABK_ERROR_VFD_ERROR);
            ABK_set_drum_mode(ABK_DRUM_BRAKED);
            ABK_set_speed(0.0);
            ABK_set_motor_mode(ABK_MOTOR_DISABLED);
        } else {
            ABK_error = REMOVE_FLAG(ABK_error, ABK_ERROR_VFD_ERROR);
        }

        if ((bool) !emergency_stop) { // Stop motor on emergency input
            ABK_error = ADD_FLAG(ABK_error, ABK_ERROR_EMERGENCY_STOP);
            ABK_set_drum_mode(ABK_DRUM_BRAKED);
            ABK_set_speed(0.0);
            ABK_set_motor_mode(ABK_MOTOR_DISABLED);
        } else {
            ABK_error = REMOVE_FLAG(ABK_error, ABK_ERROR_EMERGENCY_STOP);
        }

        if (ABK_error != ABK_ERROR_NONE) // Block here if we have any error.
            continue;

        if ((bool) slowfeed_fw_input || (bool) slowfeed_rw_input) { // Overrides default behavior for loading/unloading
            ABK_state = ABK_STATE_SLOWFEED;
            ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
            if (slowfeed_fw_input)
                ABK_set_motor_mode(ABK_MOTOR_FW);
            else
                ABK_set_motor_mode(ABK_MOTOR_RW);
            ABK_set_speed(ABK_SLOWFEED_SPEED);
            _triggered = false;
            ABK_timer.reset();
            continue;
        } else if (ABK_slowfeed != 0) { // Overrides default behavior for loading/unloading (Serial command)
            ABK_state = ABK_STATE_SLOWFEED;
            ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
            if (ABK_slowfeed == 1)
                ABK_set_motor_mode(ABK_MOTOR_FW);
            else
                ABK_set_motor_mode(ABK_MOTOR_RW);
            ABK_set_speed(ABK_SLOWFEED_SPEED);
            _triggered = false;
            ABK_timer.reset();
            continue;
        } else if (ABK_state == ABK_STATE_SLOWFEED) {
            if ((bool) !slowfeed_fw_input && (bool) !slowfeed_rw_input)
                ABK_state = (_triggered) ? ABK_STATE_STANDBY : ABK_STATE_READY;
        }


        if (ABK_state == ABK_STATE_RUN || ABK_state == ABK_STATE_READY) {
            if (!_triggered && (ac_trigger == 0) && (ABK_error == ABK_ERROR_NONE)) {
                ABK_state = ABK_STATE_RUN;
                _triggered = true;
                _trigger_time = ABK_timer.read_ms();    // Store and reset timer: This ensure the timer
                ABK_timer.reset();                      // doesn't overflow after the ABK been trigered (undefined behaviour)
                printf("status trigger\r\n");
            } else if (!_triggered) {
                ABK_set_drum_mode(ABK_DRUM_BRAKED);
                ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                ABK_set_speed(0);
                continue;
            }


            if (ABK_state == ABK_STATE_RUN) {
                int _stime = ABK_timer.read_ms(); // Update time since trigger
                led2 = !led2;
                DEBUG_PRINTF("stime: %d \r\n", _stime);

                if (_stime >= _config.start_time && _stime < _config.p1.time) {
                    ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.start_time, _config.p1.time,
                           0, _config.p1.speed, _stime);
                    ABK_set_speed(rspeed);
                    DEBUG_PRINTF("T0 %f\r\n", rspeed);
                }
                else if (_stime >= _config.p1.time && _stime < _config.p2.time) {
                    ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.p1.time, _config.p2.time,
                            _config.p1.speed, _config.p2.speed, _stime);
                    ABK_set_speed(rspeed);
                    DEBUG_PRINTF("T1 %f\r\n", rspeed);
                }
                else if (_stime >= _config.p2.time && _stime < _config.p3.time) {
                    ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.p2.time, _config.p3.time,
                            _config.p2.speed, _config.p3.speed, _stime);
                    ABK_set_speed(rspeed);
                    DEBUG_PRINTF("T2 %f\r\n", rspeed);
                }
                else if (_stime >= _config.p3.time && _stime < _config.stop_time) {
                    ABK_set_drum_mode(ABK_DRUM_FREEWHEEL);
                    ABK_set_motor_mode(ABK_MOTOR_FW);

                    float rspeed = ABK_map(_config.p3.time, _config.stop_time,
                            _config.p3.speed, 0, _stime);
                    ABK_set_speed(rspeed);
                    DEBUG_PRINTF("T3 %f\r\n", rspeed);
                }
                else if (_stime >= _config.stop_time) {
                    ABK_state = ABK_STATE_STANDBY;
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);
                    ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                    DEBUG_PRINTF("S\r\n");
                    _triggered = false;
                    ABK_state = ABK_STATE_STANDBY;
                } else {
                    ABK_set_speed(0);
                    ABK_set_drum_mode(ABK_DRUM_BRAKED);
                    ABK_set_motor_mode(ABK_MOTOR_DISABLED);
                    DEBUG_PRINTF("U\r\n");
                }
            } else {
                ABK_set_speed(0);
                ABK_set_drum_mode(ABK_DRUM_BRAKED);
                ABK_set_motor_mode(ABK_MOTOR_DISABLED);
            }

        } else {
            ABK_set_speed(0);
            ABK_set_drum_mode(ABK_DRUM_BRAKED);
            ABK_set_motor_mode(ABK_MOTOR_DISABLED);
        }
    }
}

static void ABK_serial_task(void) {
    char cmd_buf[10];
    std::string line="";
    std::string cmd;
    char c='\0';

    ABK_config_t tmp_config;

    ABK_config_mutex.lock();
    ABK_eeprom_read_config(&eeprom, &tmp_config);
    ABK_config_mutex.unlock();

    while (true) {
        if (USBport.readable() > 0) {
            while (USBport.readable() > 0) {
                c = USBport.getc();
                line += c;
                USBport.putc(c);
                if (c == 0x08) { // Backspace character
                    line.erase(line.end()-1);
                }
                if (c == '\r' || c == '\n') {
                    if (c == '\r')
                        USBport.putc('\n');

                    break;
                }
            }

            if (c == '\r' || c == '\n') {
                char opt_str[10];
                int args;
                unsigned int nargs = sscanf(line.c_str(), "%s %s %d", cmd_buf, opt_str, &args);

                if (nargs > 0) {
                    cmd = cmd_buf;

                    if (cmd == "help") {
                        USBport.printf("Abrakabuki by ExMachina\r\n");
                        USBport.printf("    version: %s\r\n\r\n", ABK_VERSION);
                        USBport.printf("available commands:\r\n");
                        USBport.printf("    set OPTION VALUE     Set the option to the desired value.\r\n");
                        USBport.printf("                         Integer and float are accepted.\r\n");
                        USBport.printf("                         DELAYs are in milliseconds, SPEEDs in percent.\r\n");
                        USBport.printf("         start DELAY     Delay from trigger to start.\r\n");
                        USBport.printf("         p1.time DELAY   Delay from trigger to point 1.\r\n");
                        USBport.printf("         p1.speed SPEED  Speed at point 1.\r\n");
                        USBport.printf("         p2.time DELAY   Delay from trigger to point 2.\r\n");
                        USBport.printf("         p2.speed SPEED  Speed at point 2.\r\n");
                        USBport.printf("         stop DELAY      Delay from trigger to full stop.\r\n");
                        USBport.printf("\r\n");
                        USBport.printf("    get                  Return current configuration\r\n");
                        USBport.printf("    save                 Save configuration to eeprom\r\n");
                        USBport.printf("    erase                Erase configuration from eeprom\r\n");
                        USBport.printf("    reset                Reset the microcontroller\r\n");
                        USBport.printf("    help                 Display this help message\r\n");
                    } else if (cmd == "set") {
                        if (nargs > 1) {
                            USBport.printf("%s set to %d\r\n", opt_str, args);

                            if (strcmp(opt_str, "start") == 0) {
                                tmp_config.start_time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p1.time") == 0) {
                                tmp_config.p1.time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p1.speed") == 0) {
                                tmp_config.p1.speed = (uint16_t) args;
                            } else if (strcmp(opt_str, "p2.time") == 0) {
                                tmp_config.p2.time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p2.speed") == 0) {
                                tmp_config.p2.speed = (uint16_t) args;
                            } else if (strcmp(opt_str, "p3.time") == 0) {
                                tmp_config.p3.time = (uint16_t) args;
                            } else if (strcmp(opt_str, "p3.speed") == 0) {
                                tmp_config.p3.speed = (uint16_t) args;
                            } else if (strcmp(opt_str, "stop") == 0) {
                                tmp_config.stop_time = (uint16_t) args;
                            } else {
                                USBport.printf("unrecognized option: %s.\r\n", opt_str);
                            }
                        } else {
                            USBport.printf("misformatted command: %s.\r\n", cmd_buf);
                        }
                    } else if (cmd == "get") {
                        ABK_config_mutex.lock();

                        USBport.printf("start %d\r\n", ABK_config.start_time);
                        USBport.printf("p1.time %d\r\np1.speed %d\r\n", ABK_config.p1.time, ABK_config.p1.speed);
                        USBport.printf("p2.time %d\r\np2.speed %d\r\n", ABK_config.p2.time, ABK_config.p2.speed);
                        USBport.printf("p3.time %d\r\np3.speed %d\r\n", ABK_config.p3.time, ABK_config.p3.speed);
                        USBport.printf("stop %d\r\n", ABK_config.stop_time);

                        ABK_config_mutex.unlock();
                    } else if (cmd == "gett") {
                        USBport.printf("start %d\r\n", tmp_config.start_time);
                        USBport.printf("p1.time %d\r\np1.speed %d\r\n", tmp_config.p1.time, tmp_config.p1.speed);
                        USBport.printf("p2.time %d\r\np2.speed %d\r\n", tmp_config.p2.time, tmp_config.p2.speed);
                        USBport.printf("p3.time %d\r\np3.speed %d\r\n", tmp_config.p3.time, tmp_config.p3.speed);
                        USBport.printf("stop %d\r\n", tmp_config.stop_time);
                    } else if (cmd == "save") {
                        ABK_config_mutex.lock();

                        tmp_config.state = 1;

                        if (ABK_eeprom_write_config(&eeprom, &tmp_config))
                            USBport.printf("config saved to EEPROM.\r\n");
                        else
                            USBport.printf("error occured during writing to EEPROM.\r\n");

                        ABK_config_mutex.unlock();
                    } else if (cmd == "erase") {
                        ABK_config_mutex.lock();

                        if (ABK_eeprom_erase_config(&eeprom))
                            USBport.printf("config erased EEPROM.\r\n");
                        else
                            USBport.printf("error occured during erasing.\r\n");

                        ABK_config_mutex.unlock();
                    } else if (cmd == "reset") {
                        ABK_reset = true;
                    } else if (cmd == "version") {
                        USBport.printf("%s\r\n", ABK_VERSION);
                    } else {
                        USBport.printf("unrecognized command: %s. Type 'help' for help\r\n", cmd_buf);
                    }
                }

                line = "\0"; // Empty buffer
                cmd = "\0";
            }
        }


        Thread::wait(ABK_SERIAL_INTERVAL);
    }
}
