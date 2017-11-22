/*
 * pins.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef PINS_H
#define PINS_H

#include "config.h"

#include "PinNames.h"
#include "mbed.h"

// Generic IO map
#define INPUT1_1        P2_9
#define INPUT1_2        P0_16
#define INPUT1_3        P0_15
#define INPUT1_4        P0_17
#define INPUT2_1        P0_19
#define INPUT2_2        P0_18
#define INPUT2_3        P0_21
#define INPUT2_4        P0_22
#define INPUT3_1        P0_20

#define OUTPUT1_1       P0_0
#define OUTPUT1_2       P1_29
#define OUTPUT1_3       P1_28
#define OUTPUT1_4       P1_27
#define OUTPUT2_1       P1_25
#define OUTPUT2_2       P1_24
#define OUTPUT2_3       P1_23
#define OUTPUT2_4       P1_22
#define OUTPUT3_1       P1_26

#define LED1            P1_18
#define LED2            P1_21

#define I2C0_SDA        P0_27
#define I2C0_SCL        P0_28
#define I2C1_SDA        P0_0
#define I2C1_SCL        P0_1
#define I2C2_SDA        P0_10
#define I2C2_SCL        P0_11

#define USB_DP          P0_29
#define USB_DN          P0_30

#define ENC_A           P1_20
#define ENC_B           P1_23
#define ENC_I           P1_24

#define CAN1_RXD        P0_0
#define CAN1_TXD        P0_1

// Application specific IO map

#define SLOWFEED_FW     INPUT1_1
#define SLOWFEED_RW     INPUT1_2
#define VFD_STS         INPUT1_3
#define EMERGENCY_STOP  INPUT1_4

#define TRIGGER_INPUT   INPUT3_1

#define CTL_FW_DIR      OUTPUT1_1
#define CTL_RW_DIR      OUTPUT1_2
#define CTL_BRAKE       OUTPUT1_3

#define LED_STS         OUTPUT2_1
#define LED_ERR         OUTPUT2_2

#define CTL_PWM_VFD     OUTPUT3_1

#define ISP_RXD         P0_3
#define ISP_TXD         P0_2
#define USBRX           ISP_RXD
#define USBTX           ISP_TXD

// Leds
DigitalOut led1(LED1);
DigitalOut led2(LED2);

DigitalOut led_sts(LED_STS);
DigitalOut led_err(LED_ERR);
#if ABK_TEST
DigitalOut output1_4(OUTPUT1_4);
#endif

// Inputs
DigitalIn slowfeed_fw_input(SLOWFEED_FW);
DigitalIn slowfeed_rw_input(SLOWFEED_RW);
DigitalIn drive_status(VFD_STS);
DigitalIn emergency_stop(EMERGENCY_STOP);

#if ABK_SIMULATE
bool ac_trigger;
#else
DigitalIn ac_trigger(TRIGGER_INPUT);
#endif

// Outputs
DigitalOut dir_fw(CTL_FW_DIR);
DigitalOut dir_rw(CTL_RW_DIR);
// DigitalOut brake(CTL_BRAKE);
bool brake;

PwmOut motor_ctl(CTL_PWM_VFD);

#endif /* !PINS_H */
