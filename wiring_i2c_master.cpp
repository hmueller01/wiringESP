/**
 * @file  wiring_i2c_master.cpp
 * @brief I2C TWI (Two Wire Interface) class for ESP8266.
 *
 * MIT License
 * Copyright (c) 2018 Holger Mueller
 */

 // C++ wrapper
extern "C" {
// put C includes inside here to avoid undefined references by linker.
#include "wiringESP.h"
#include "wiring_i2c_master.h"
}


/**
 * @brief  Private function to set I2C SDA and SCL (data/clock) bits
 *         and wait the half clock cycle.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  sda - SDA level (LOW/HIGH).
 * @param  scl - SCL level (LOW/HIGH).
 */
void ICACHE_FLASH_ATTR
I2c_master::clock(uint8_t sda, uint8_t scl)
{
	this->m_last_sda = sda;
	this->m_last_scl = scl;

	digitalWrite(this->m_pin_sda, sda);
	digitalWrite(this->m_pin_scl, scl);
	delayMicroseconds(this->m_clock);
}

/**
 * @brief  Send I2C start condition.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::start(void)
{
	this->m_readStretching = false;
	// SDA 1 -> 0 while SCL = 1
	this->clock(1, this->m_last_scl);
	this->clock(1, 1);
	this->clock(0, 1);
}

/**
 * @brief  Send I2C stop condition.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::stop(void)
{
	this->m_readStretching = false;
	// SDA 0 -> 1 while SCL = 1
	this->clock(0, this->m_last_scl);
	this->clock(0, 1);
	this->clock(1, 1);
}

/**
 * @brief  Send I2C acknowledge (ACK/NACK) sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  ack - ACK level (LOW = ACK, HIGH = NACK).
 */
void ICACHE_FLASH_ATTR
I2c_master::writeAck(uint8_t ack)
{
	this->m_readStretching = false;
	this->clock(this->m_last_sda, 0);
	this->clock(ack, 0);
	this->clock(ack, 1);
	this->clock(ack, 0);
	this->clock(1, 0);
}

/**
 * @brief  Send I2C ACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::writeAck(void)
{
	this->writeAck(I2C_ACK);
}

/**
 * @brief  Send I2C NACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::writeNack(void)
{
	this->writeAck(I2C_NACK);
}

/**
 * @brief  Read I2C acknowledge (ACK/NACK).
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @return ACK level (LOW = ACK, HIGH = NACK).
 */
uint8_t ICACHE_FLASH_ATTR
I2c_master::readAck(void)
{
	uint8_t ret;

	this->m_readStretching = false;
	this->clock(this->m_last_sda, 0);
	this->clock(1, 0);
	this->clock(1, 1);
	ret = digitalRead(this->m_pin_sda);
	this->clock(1, 0);

	return ret;
}

/**
 * @brief  Read I2C stretching (hold).
 *         In the hold master mode, the slave pulls down the SCL line
 *         while processing to force the master into a wait state. By
 *         releasing the SCL line the slave indicates that internal
 *         processing is finished and that communication can be continued.
 * @author Holger Mueller
 * @date   2020-11-22
 *
 * @return Stretching level (LOW = wait, HIGH = processing finished).
 */
uint8_t ICACHE_FLASH_ATTR
I2c_master::readStretching(void)
{
	uint8_t ret;

	this->clock(this->m_last_sda, 1);
	ret = digitalRead(this->m_pin_scl);
	// if ret == I2C_HI slave released SCL line
	// read data without additional SCL raising slope,
	// otherwise first bit will be lost!
	this->m_readStretching = true; // readStretching() is active

	return ret;
}

/**
 * @brief  Read byte from I2C bus.
 * @author Holger Mueller
 * @date   2018-05-15, 2020-11-23
 *
 * @param  stretching - true if stretching read is active (optional).
 *                      Default false.
 * @return Read value (byte).
 */
uint8_t ICACHE_FLASH_ATTR
I2c_master::readByte(void)
{
	uint8_t data = 0;

	// if stretching read is active do not pull low SCL at first bit
	if (this->m_readStretching) {
		this->m_readStretching = false;
	} else {
		this->clock(m_last_sda, 0);
	}
	for (uint8_t i = 0; i < 8; i++) {
		this->clock(1, 1);
		data = (data << 1) | digitalRead(this->m_pin_sda);
		this->clock(1, 0);
	}

	return data;
}

/**
 * @brief  Send byte to I2C bus.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  data - Value (byte) to send.
 */
void ICACHE_FLASH_ATTR
I2c_master::writeByte(uint8_t data)
{
	uint8_t sda;
	sint8_t i;

	this->m_readStretching = false;
	this->clock(this->m_last_sda, 0);
	for (i = 7; i >= 0; i--) {
		sda = (data >> i) & 0x01;
		this->clock(sda, 0);
		this->clock(sda, 1);
		this->clock(sda, 0);
	}
}

/**
 * @brief  Initiate the I2c_master library.
 *         This shall be called before any other function.
 *         Configs SDA and SCL GPIO pin to open-drain output mode,
 *         clock speed and sends init sequence.
 * @author Holger Mueller
 * @date   2018-05-22
 *
 * @param  pin_sda - wiringESP SDA pin.
 * @param  pin_scl - wiringESP SCL pin.
 * @param  clock - Clock time of half cycle [µs] (optional).
 *                 1 = 500kHz
 *                 2 = 250kHz
 *                 5 = 100kHz (default)
 */
void ICACHE_FLASH_ATTR
I2c_master::begin(uint8_t pin_sda, uint8_t pin_scl, uint8_t clock)
{
	this->m_pin_sda = pin_sda;
	this->m_pin_scl = pin_scl;
	this->m_clock = clock;
	this->m_readStretching = false;
	pinMode(this->m_pin_sda, OPENDRAIN);
	pinMode(this->m_pin_scl, OPENDRAIN);
	this->clock(I2C_HI, I2C_HI);
}

ICACHE_FLASH_ATTR
I2c_master::I2c_master()
{
}
