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

	writeRegister(PWR_MGMNT_1, PWR_CYCLE);

	setBitOrder(MSBFIRST);

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
	_axcounts = (((int16_t)buffer[0]) << 8) | buffer[1];
	_aycounts = (((int16_t)buffer[2]) << 8) | buffer[3];
	_azcounts = (((int16_t)buffer[4]) << 8) | buffer[5];
	_tcounts = (((int16_t)buffer[6]) << 8) | buffer[7];
	_gxcounts = (((int16_t)buffer[8]) << 8) | buffer[9];
	_gycounts = (((int16_t)buffer[10]) << 8) | buffer[11];
	_gzcounts = (((int16_t)buffer[12]) << 8) | buffer[13];
	_hxcounts = (((int16_t)buffer[15]) << 8) | buffer[14];
	_hycounts = (((int16_t)buffer[17]) << 8) | buffer[16];
	_hzcounts = (((int16_t)buffer[19]) << 8) | buffer[18];
	// transform and convert to float values
	_ax = (((float)(tX[0] * _axcounts + tX[1] * _aycounts + tX[2] * _azcounts) * _accelScale) - _axb) * _axs;
	_ay = (((float)(tY[0] * _axcounts + tY[1] * _aycounts + tY[2] * _azcounts) * _accelScale) - _ayb) * _ays;
	_az = (((float)(tZ[0] * _axcounts + tZ[1] * _aycounts + tZ[2] * _azcounts) * _accelScale) - _azb) * _azs;
	_gx = ((float)(tX[0] * _gxcounts + tX[1] * _gycounts + tX[2] * _gzcounts) * _gyroScale) - _gxb;
	_gy = ((float)(tY[0] * _gxcounts + tY[1] * _gycounts + tY[2] * _gzcounts) * _gyroScale) - _gyb;
	_gz = ((float)(tZ[0] * _gxcounts + tZ[1] * _gycounts + tZ[2] * _gzcounts) * _gyroScale) - _gzb;
	_hx = (((float)(_hxcounts)*_magScaleX) - _hxb) * _hxs;
	_hy = (((float)(_hycounts)*_magScaleY) - _hyb) * _hys;

	_hz = (((float)(_hzcounts)*_magScaleZ) - _hzb) * _hzs;
	_t = ((((float)_tcounts) - _tempOffset) / _tempScale) + _tempOffset;
	return _ax;
}

