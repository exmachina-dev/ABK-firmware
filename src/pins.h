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

#define LED1            P1_21
#define LED2            P1_18
#define LED3            NC
#define LED4            NC

#define LED_HMI1        P2_2
#define LED_HMI2        P2_3
#define LED_HMI3        P2_4
#define LED_HMI4        P2_5

#define BUTTON1         P0_16
#define BUTTON2         P0_15
#define BUTTON3         P0_17
#define BUTTON4         P0_18
#define BUTTON5         P0_19

#define LCD_BACKLIGHT   P0_22

#define FAN_S1          P0_8
#define FAN_S2          P0_9
#define FAN_P1          P2_0
#define FAN_P2          P2_1

#define DRV_STS1        P0_18
#define DRV_STS2        P0_19
#define TRIGG_INPUT1    P0_20
#define TRIGG_INPUT2    P1_27 // Need to be removed in future, only there to fix a wiring error
#define LIMIT_SW1       P0_21
#define EMERGENCY_STOP  P0_22

#define CTL_BRAKE       P1_22
#define CTL_CLUTCH      P1_23
#define CTL_FW_DIR      P1_24
#define CTL_RW_DIR      P1_25
#define CTL_PWM_MOT     P1_26

#define ISP_RXD         P0_3
#define ISP_TXD         P0_2
#define USBRX           ISP_RXD
#define USBTX           ISP_TXD

#define ADC_SLACK       P0_25
#define ADC_TEMP0       P0_24
#define ADC_TEMP1       P0_23

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

// Leds
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED_HMI1);
DigitalOut led4(LED_HMI2);
DigitalOut led5(LED_HMI3);
DigitalOut led6(LED_HMI4);

// Inputs
DigitalIn drive_status(DRV_STS1);
DigitalIn slowfeed_input(DRV_STS2);
#if ABK_SIMULATE
bool ac_trigger;
#else
DigitalIn ac_trigger(TRIGG_INPUT2);
#endif
DigitalIn drum_limit(LIMIT_SW1);
DigitalIn emergency_stop(EMERGENCY_STOP);

// Outputs
DigitalOut brake(CTL_BRAKE);
DigitalOut clutch(CTL_CLUTCH);
DigitalOut dir_fw(CTL_FW_DIR);
DigitalOut dir_rw(CTL_RW_DIR);
PwmOut motor_ctl(CTL_PWM_MOT);

#endif /* !PINS_H */
