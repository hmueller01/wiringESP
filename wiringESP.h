/**
 * @file  wiringESP.h
 * @brief Partial implementation of the Arduino Wiring API for the ESP8266.
 *
 * Based on Arduino Wiring API and Gordon Henderson wiringPi.
 *
 * MIT License
 * Copyright (c) 2017 Holger Mueller
 */

#ifndef __WIRING_ESP_H__
#define __WIRING_ESP_H__

// C++ wrapper
#ifdef __cplusplus
extern "C" {
#endif

#include <gpio.h>
#include <osapi.h>
#include <user_interface.h>


// Pin modes
enum pin_mode {
	INPUT = 0,
	OUTPUT,
	INPUT_PULLUP,
	OPENDRAIN,
	PWM_OUTPUT,
	GPIO_CLOCK,
	SOFT_PWM_OUTPUT,
	SOFT_TONE_OUTPUT,
	PWM_TONE_OUTPUT
};

#define LOW  0
#define HIGH 1

#ifndef OFF
#define OFF  LOW
#endif
#ifndef ON
#define ON   HIGH
#endif

// Pull up/down/none
#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

// Interrupt levels. Mimic Arduino like interrupt modes.
#define CHANGE  GPIO_PIN_INTR_ANYEDGE
#define RISING  GPIO_PIN_INTR_POSEDGE
#define FALLING GPIO_PIN_INTR_NEGEDGE


// Function prototypes

// Missing ESP8266 SDK function prototypes. GCC will warn on these if in c99 mode.
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);

// Core wiringESP functions
void pinMode(uint8_t pin, uint8_t mode);
void pullUpDnControl(uint8_t pin, uint8_t pud);
uint8_t digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
//void pwmWrite(uint8_t pin, int value);
//int analogRead(uint8_t pin);
//void analogReference(uint8_t mode);
//void analogWrite(uint8_t pin, int value);

// Interrupts
bool attachInterrupt(uint8_t pin, void (*function)(void), uint8_t mode);
bool detachInterrupt(uint8_t pin);

// Timing extras from Arduino land
void delay(unsigned long ms);
// delayMicroseconds(us): Delay in microseconds (µs). max: 65535 μs.
#define delayMicroseconds os_delay_us
//unsigned int millis(void);
// micros(): Return a number of microseconds as an unsigned int. Wraps after 71 minutes.
#define micros system_get_time

#ifdef __cplusplus
}
#endif

#endif
