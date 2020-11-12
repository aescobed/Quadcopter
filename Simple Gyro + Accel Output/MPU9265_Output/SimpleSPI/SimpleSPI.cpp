#include "SimpleSPI.h"


/*
 *	MISO (Master In Slave Out) - The Slave line for sending data to the master,
 *	MOSI (Master Out Slave In) - The Master line for sending data to the peripherals,
 *	SCK (Serial Clock) - The clock pulses which synchronize data transmission generated by the master
 *
 *
 *	SS (Slave Select) - the pin on each device that the master can use to enable and disable specific devices.
 *	
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
*/



SimpleSPIClass spi;
uint8_t SimpleSPIClass::initialized = 0;


int SimpleSPIClass::initialize()
{
	// Set SS to high so a connected chip will be "deselected" by default
	uint8_t port = digitalPinToPort(SS);
	uint8_t bit = digitalPinToBitMask(SS);
	volatile uint8_t* reg = portModeRegister(port);

	// if the SS pin is not already configured as an output
	// then set it high (to enable the internal pull-up resistor)
	if (!(*reg & bit)) {
		digitalWrite(SS, HIGH);
	}

	// When the SS pin is set as OUTPUT, it can be used as
	// a general purpose output port (it doesn't influence
	// SPI operations).
	pinMode(SS, OUTPUT);

	// Warning: if the SS pin ever becomes a LOW INPUT then SPI
	// automatically switches to Slave, so the data direction of
	// the SS pin MUST be kept as OUTPUT.
	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);

	// Set direction register for SCK and MOSI pin.
	// MISO pin automatically overrides to INPUT.
	// By doing this AFTER enabling SPI, we avoid accidentally
	// clocking in a single bit since the lines go directly
	// from "input" to SPI control.
	// http://code.google.com/p/arduino/issues/detail?id=888
	pinMode(SCK, OUTPUT);

	// Might not need MOSI (digital 11)
	pinMode(MOSI, OUTPUT);

}



void SimpleSPIClass::begin()
{

	uint8_t sreg = SREG;
	noInterrupts();
	
	if (!initialized)
	{
		initialize();
	}

	initialized++; // reference count
	SREG = sreg;

	setBitOrder(MSBFIRST);

}


int SimpleSPIClass::writeRegister(uint8_t subAddress, uint8_t data) {
	/* write data to device */
		beginTransaction(SPISettings(SPI_LS_CLOCK, MSBFIRST, SPI_MODE3)); // begin the transaction
		digitalWrite(_csPin, LOW); // select the MPU9250 chip
		transfer(subAddress); // write the register address
		transfer(data); // write the data
		digitalWrite(_csPin, HIGH); // deselect the MPU9250 chip
		endTransaction(); // end the transaction
	}



