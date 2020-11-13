#include "SimpleSPI.h"
#include <math.h>


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
*/



SimpleSPIClass spi;
uint8_t SimpleSPIClass::initialized = 0;
uint8_t SimpleSPIClass::interruptMode = 0;
uint8_t SimpleSPIClass::interruptMask = 0;
uint8_t SimpleSPIClass::interruptSave = 0;



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
	pinMode(10, OUTPUT);

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



int SimpleSPIClass::begin()
{

	uint8_t sreg = SREG;
	noInterrupts();
	
	if (!initialized)
	{
		initialize();
	}

	initialized++; // reference count
	SREG = sreg;

	// reset the MPU9250
	writeRegister(PWR_MGMNT_1, PWR_RESET);

	// set accelerometer to low power mode
	writeRegister(PWR_MGMNT_1, PWR_CYCLE);

	// wait for MPU-9250 to come back up
	delay(1);

	// check the WHO AM I byte, expected value is 0x71 (decimal 113) or 0x73 (decimal 115)
	if ((whoAmI() != 113) && (whoAmI() != 115)) {
		return 0;
	}

	// set accel scale (default = 16g)
	//writeRegister(ACCEL_CONFIG, ACCEL_FS_SEL_8G);

	// Set gravitational constant
	setG();

	return 1;

}


int SimpleSPIClass::writeRegister(uint8_t subAddress, uint8_t data) {
	/* write data to device */
		beginTransaction(SPISettings(LS_CLOCK, MSBFIRST, SPI_MODE3)); // begin the transaction
		digitalWrite(SSPin, LOW); // select the MPU9250 chip
		transfer(subAddress); // write the register address
		transfer(data); // write the data
		digitalWrite(SSPin, HIGH); // deselect the MPU9250 chip
		endTransaction(); // end the transaction
	}


int SimpleSPIClass::readRegisters(uint8_t subAddress, uint8_t count, uint8_t* dest) {

		interruptMode = 0;

		// begin the transaction
		beginTransaction(SPISettings(HS_CLOCK, MSBFIRST, SPI_MODE3));

		digitalWrite(SSPin, LOW); // select the MPU9250 chip
		transfer(subAddress | SPI_READ); // specify the starting register address
		for (uint8_t i = 0; i < count; i++) {
			dest[i] = transfer(0x00); // read the data
		}
		digitalWrite(SSPin, HIGH); // deselect the MPU9250 chip
		endTransaction(); // end the transaction

		return 1;
	
}

/* reads the most current data from MPU9250 and stores in buffer */
int SimpleSPIClass::readSensor() {

	// grab the data from the MPU9250
	if (readRegisters(ACCEL_OUT, 21, buffer) < 0) {
		return -1;
	}

	// combine into 16 bit values
	_ax = (((int16_t)buffer[0]) << 8) | buffer[1];
	_ay= (((int16_t)buffer[2]) << 8) | buffer[3];
	_az= (((int16_t)buffer[4]) << 8) | buffer[5];
	_tcounts = (((int16_t)buffer[6]) << 8) | buffer[7];
	_gx = (((int16_t)buffer[8]) << 8) | buffer[9];
	_gycounts = (((int16_t)buffer[10]) << 8) | buffer[11];
	_gzcounts = (((int16_t)buffer[12]) << 8) | buffer[13];
	_hxcounts = (((int16_t)buffer[15]) << 8) | buffer[14];
	_hycounts = (((int16_t)buffer[17]) << 8) | buffer[16];
	_hzcounts = (((int16_t)buffer[19]) << 8) | buffer[18];

	// transform and convert to float values
	_hx = (((float)(_hxcounts)*_magScaleX) - _hxb) * _hxs;
	_hy = (((float)(_hycounts)*_magScaleY) - _hyb) * _hys;
	_hz = (((float)(_hzcounts)*_magScaleZ) - _hzb) * _hzs;
	_t = ((((float)_tcounts) - _tempOffset) / _tempScale) + _tempOffset;

	return 0;
}

float SimpleSPIClass::returnVar()
{

	readSensor();
	xAng += _gx / 1000;
	return (float) xAng;

}

void SimpleSPIClass::setG()
{

	int count = 0;
	double magnitude = 0;

	while (count < 500)
	{
		readSensor();

		float x = (float)_ax;
		float y = (float)_ay;
		float z = (float)_az;

		magnitude += sqrt(x * x + y * y + z * z);

		count++;

	}

	magnitude /= count;

	G = magnitude;

}


/* gets the MPU9250 WHO_AM_I register value, expected to be 0x71 */
int SimpleSPIClass::whoAmI() {
	// read the WHO AM I register
	if (readRegisters(WHO_AM_I, 1, buffer) < 0) {
		return -1;
	}
	// return the register value
	return buffer[0];
}