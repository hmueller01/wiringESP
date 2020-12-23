/**
 * @file  wiring_i2c_master.h
 * @brief I2C TWI (Two Wire Interface) class header for ESP8266.
 *
 * MIT License
 * Copyright (c) 2018 Holger Mueller
 */

#ifndef __WIRING_I2C_MASTER_H__
#define __WIRING_I2C_MASTER_H__

#include <c_types.h>


// I2C pin level
typedef enum i2c_lvl {
	I2C_LO = 0,
	I2C_HI = 1,
} i2c_lvl_t;

// I2C acknowledge bit
typedef enum i2c_ack {
	I2C_ACK = 0,
	I2C_NACK = 1,
} i2c_ack_t;

class I2c_master {
  public:
	I2c_master(); // constructor

	void begin(uint8_t pin_sda, uint8_t pin_scl, uint8_t clock = 5);
	void start(void);
	void stop(void);
	void writeAck(uint8_t level);
	void writeAck(void);
	void writeNack(void);
	uint8_t readAck(void);
	uint8_t readStretching(void);
	uint8_t readByte(void);
	void writeByte(uint8_t data);

  private:
	uint8_t m_pin_sda = 0;
	uint8_t m_pin_scl = 0;
	uint8_t m_last_sda = 0;
	uint8_t m_last_scl = 0;
	uint8_t m_clock = 5; // 5µs half cycle = 100kHz clock
	bool m_readStretching = false; // true if readStretching() is active

	void clock(uint8_t sda, uint8_t scl);
};

#endif // __WIRING_I2C_MASTER_H__
