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

// CANopen
volatile uint16_t   ABK_timer1ms = 0U;   // variable increments each millisecond

extern "C" void mbed_reset();

Ticker ticker_1ms;
Ticker ticker_sync;
Ticker ticker_leds;

// Serial
Serial USBport(ISP_TXD, ISP_RXD);


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

    while(true) {
        led1 = !led1;
        wdog.kick();
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
