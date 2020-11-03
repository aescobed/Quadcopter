
#include <Arduino.h>

class SimpleSPIClass {

public:
	static void begin();

	
private: 
	static uint8_t initialized;
	static int initialize();

};

extern SimpleSPIClass spi;

