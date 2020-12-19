/**
 * @file  wiringESP.c
 * @brief Partial implementation of the Arduino Wiring API for the ESP8266.
 *
 * Based on Arduino Wiring API and Gordon Henderson wiringPi.
 *
 * MIT License
 * Copyright (c) 2017 Holger Mueller
 */

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include "wiringESP.h"

#ifndef ERROR
#define ERROR(format, ...) os_printf(format, ## __VA_ARGS__)
#endif

// pinToGpioMux:
//   Take a Wiring pin (0 through X) and re-map it to the ESP8266 SOC GPIO/MUX reg
#define FUNC 0
#define NAME 1
LOCAL const int pinToGpioMux[GPIO_PIN_COUNT][2] = {
	// FUNC, NAME
	{FUNC_GPIO0, PERIPHS_IO_MUX_GPIO0_U}, // 0
	{FUNC_GPIO1, PERIPHS_IO_MUX_U0TXD_U}, // 1
	{FUNC_GPIO2, PERIPHS_IO_MUX_GPIO2_U}, // 2
	{FUNC_GPIO3, PERIPHS_IO_MUX_U0RXD_U}, // 3
	{FUNC_GPIO4, PERIPHS_IO_MUX_GPIO4_U}, // 4
	{FUNC_GPIO5, PERIPHS_IO_MUX_GPIO5_U}, // 5
	{-1, -1}, // 6 - not available
	{-1, -1}, // 7 - not available
	{-1, -1}, // 8 - not available
	{FUNC_GPIO9, PERIPHS_IO_MUX_SD_DATA2_U}, // 9 - needed for flash memory on some boards
	{FUNC_GPIO10, PERIPHS_IO_MUX_SD_DATA3_U}, // 10 - needed for flash memory on some boards
	{-1, -1}, // 11 - not available
	{FUNC_GPIO12, PERIPHS_IO_MUX_MTDI_U}, // 12
	{FUNC_GPIO13, PERIPHS_IO_MUX_MTCK_U}, // 13
	{FUNC_GPIO14, PERIPHS_IO_MUX_MTMS_U}, // 14
	{FUNC_GPIO15, PERIPHS_IO_MUX_MTDO_U} // 15 - needs to be low at start!
};

// ISR Data
LOCAL void (*isrFunctions[GPIO_PIN_COUNT])(void) = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


/**
 * @brief  Check if given pin is supported.
 * @author Holger Mueller
 * @date   2017-12-12
 *
 * @param  pin - Pin number (see pinToGpioMux) to set the mode.
 * @return true - pin supported, false - pin not supported.
 */
LOCAL bool ICACHE_FLASH_ATTR
checkPin(uint8_t pin)
{
	if (!GPIO_ID_IS_PIN_REGISTER(GPIO_ID_PIN(pin))) {
		ERROR("%s: Error. Pin %d not supported.\n", __FUNCTION__, pin);
		return false;
	}
	if (pinToGpioMux[pin][NAME] == -1) {
		ERROR("%s: Error. Pin %d not supported.\n", __FUNCTION__, pin);
		return false;
	}
	return true;
}

/**
 * @brief  Sets the mode of a pin to be input, output or PWM output.
 * @author Holger Mueller
 * @date   2017-12-11, 2017-12-20, 2018-05-15
 *
 * @param  pin - Pin number (see pinToGpioMux) to set the mode.
 * @param  mode - INPUT, INPUT_PULLUP or OUTPUT (see pin_mode_t),
 *                more is not yet supported.
 */
void ICACHE_FLASH_ATTR
pinMode(uint8_t pin, uint8_t mode)
{
	if (!checkPin(pin)) {
		return;
	}
	switch (mode) {
	case INPUT:
		ETS_GPIO_INTR_DISABLE();
		PIN_FUNC_SELECT(pinToGpioMux[pin][NAME], pinToGpioMux[pin][FUNC]);
		GPIO_DIS_OUTPUT(GPIO_ID_PIN(pin));
		PIN_PULLUP_DIS(pinToGpioMux[pin][NAME]);
		ETS_GPIO_INTR_ENABLE();
		break;
	case INPUT_PULLUP:
		ETS_GPIO_INTR_DISABLE();
		PIN_FUNC_SELECT(pinToGpioMux[pin][NAME], pinToGpioMux[pin][FUNC]);
		GPIO_DIS_OUTPUT(GPIO_ID_PIN(pin));
		PIN_PULLUP_EN(pinToGpioMux[pin][NAME]);
		ETS_GPIO_INTR_ENABLE();
		break;
	case OUTPUT:
		ETS_GPIO_INTR_DISABLE();
		PIN_FUNC_SELECT(pinToGpioMux[pin][NAME], pinToGpioMux[pin][FUNC]);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(pin), LOW);
		ETS_GPIO_INTR_ENABLE();
		break;
	case OPENDRAIN:
		ETS_GPIO_INTR_DISABLE();
		PIN_FUNC_SELECT(pinToGpioMux[pin][NAME], pinToGpioMux[pin][FUNC]);
		GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)),
			GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) |
			GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); // open drain
		GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS,
			GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << pin));
		ETS_GPIO_INTR_ENABLE();
		break;
	case PWM_OUTPUT:
	case GPIO_CLOCK:
	case SOFT_PWM_OUTPUT:
	case SOFT_TONE_OUTPUT:
	case PWM_TONE_OUTPUT:
	default:
		ERROR("%s: Error. Unknown mode.\n", __FUNCTION__);
		return;
	}
}


/**
 * @brief  Control the internal pull-up/down resistors on a GPIO pin.
 * @author Holger Mueller
 * @date   2017-12-12
 *
 * Note: The ESP8266 only has pull-ups, so PUD_DOWN is not supported here.
 *
 * @param  pin - Pin number (see pinToGpioMux) to set the mode.
 * @param  pud - Enable (PUD_UP) or disable (PUD_OFF) pull-up.
 */
void ICACHE_FLASH_ATTR
pullUpDnControl(uint8_t pin, uint8_t pud)
{
	if (!checkPin(pin)) {
		return;
	}
	if (pud == PUD_UP) {
		PIN_PULLUP_EN(pinToGpioMux[pin][NAME]);
	} else {
		PIN_PULLUP_DIS(pinToGpioMux[pin][NAME]);
		if (pud == PUD_DOWN) {
			ERROR("%s: Error. PUD_DOWN not supported.\n", __FUNCTION__);
		}
	}
}


/**
 * @brief  Read the value of a given pin, returning HIGH or LOW.
 * @author Holger Mueller
 * @date   2017-12-12
 *
 * @param  pin - Pin number to read from.
 * @return Status of pin (LOW or HIGH).
 */
uint8_t ICACHE_FLASH_ATTR
digitalRead(uint8_t pin)
{
	if (!checkPin(pin)) {
		return LOW;
	}
	return GPIO_INPUT_GET(pin);
}


/**
 * @brief  Set an output pin (LOW or HIGH).
 * @author Holger Mueller
 * @date   2017-12-12
 *
 * @param  pin - Pin number to write to.
 * @param  value - Level of pin (LOW or HIGH).
 */
void ICACHE_FLASH_ATTR
digitalWrite(uint8_t pin, uint8_t value)
{
	if (!checkPin(pin)) {
		return;
	}
	GPIO_OUTPUT_SET(pin, (value == LOW) ? LOW : HIGH);
}


/**
 * @brief  Handles GPIO interrupt by calling the user interrupt function.
 * @author Holger Mueller
 * @date   2017-12-13
 *
 * @param  *arg - Not used.
 */
LOCAL void
interruptHandler(void *arg)
{
	uint8_t pin;
	uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	for (pin = GPIO_ID_PIN0; pin < GPIO_PIN_COUNT; pin++) {
		if ((isrFunctions[pin] != NULL) && (gpio_status & BIT(pin))) {
			// clear interrupt status
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(pin));
			// call user interrupt function
			isrFunctions[pin]();
		}
	}
}


/**
 * @brief  Attach an user interrupt function.
 * @author Holger Mueller
 * @date   2017-12-13, 2017-12-20
 *
 * @param  pin - Pin number (see pinToGpioMux) to set the mode.
 * @param  *function - User interrupt function.
 * @param  mode - Interrupt type (see GPIO_INT_TYPE).
 * @return true if ok, otherwise false.
 */
bool ICACHE_FLASH_ATTR
attachInterrupt(uint8_t pin, void (*function)(void), uint8_t mode)
{
	if (!checkPin(pin)) {
		return false;
	}
	ETS_GPIO_INTR_ATTACH(interruptHandler, NULL);

	ETS_GPIO_INTR_DISABLE();

	// Set pin to input
	pinMode(pin, INPUT_PULLUP);
/* TODO: needed?
	gpio_register_set(GPIO_PIN_ADDR(pin), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
					  | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
					  | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));
*/
	// clear interrupt status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));

	// enable interrupt
	gpio_pin_intr_state_set(GPIO_ID_PIN(pin), mode);
	isrFunctions[pin] = function;

	ETS_GPIO_INTR_ENABLE();
	return true;
}


/**
 * @brief  Dettach an user interrupt function.
 * @author Holger Mueller
 * @date   2017-12-13
 *
 * @param  pin - Pin number (see pinToGpioMux) to set the mode.
 * @return true if ok, otherwise false.
 */
bool ICACHE_FLASH_ATTR
detachInterrupt(uint8_t pin)
{
	if (!checkPin(pin)) {
		return false;
	}
	ETS_GPIO_INTR_DISABLE();

	// disable interrupt
	gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);

	// clear interrupt status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));

	isrFunctions[pin] = NULL;

	ETS_GPIO_INTR_ENABLE();
	return true;
}


/**
 * @brief  Delay in milliseconds (ms).
 * @author Holger Mueller
 * @date   2017-12-14, 2018-03-26
 *
 * @param  ms - Delay time in milliseconds (ms).
 */
void ICACHE_FLASH_ATTR
delay(unsigned long ms)
{
	uint32_t i;

	for (i = ms; i > 0; i--) {
		os_delay_us(1000); // delay 1 ms
	}
}

/**
 * NOT YET IMPLEMENTED!
 * Unfortunately ESP8266 SDK defines millis() in libmain.a(time.o), without
 * header and documentation, so this breaks linking if implemented ...
 * @brief  Get time in milliseconds (ms). Wraps at 49 days.
 * @author Holger Mueller
 * @date   2017-12-14
 *
 * @return Time in milliseconds (ms).
 */
/*
unsigned int ICACHE_FLASH_ATTR
millis(void)
{
	return 0;
}
*/
