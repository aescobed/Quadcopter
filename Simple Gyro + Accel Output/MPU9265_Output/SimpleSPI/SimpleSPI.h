
#include <Arduino.h>


#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif


class SPISettings {
public:
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
        if (__builtin_constant_p(clock)) {
            init_AlwaysInline(clock, bitOrder, dataMode);
        }
        else {
            init_MightInline(clock, bitOrder, dataMode);
        }
    }
    SPISettings() {
        init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0);
    }
private:
    void init_MightInline(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
        init_AlwaysInline(clock, bitOrder, dataMode);
    }
    void init_AlwaysInline(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
        __attribute__((__always_inline__)) {
        // Clock settings are defined as follows. Note that this shows SPI2X
        // inverted, so the bits form increasing numbers. Also note that
        // fosc/64 appears twice
        // SPR1 SPR0 ~SPI2X Freq
        //   0    0     0   fosc/2
        //   0    0     1   fosc/4
        //   0    1     0   fosc/8
        //   0    1     1   fosc/16
        //   1    0     0   fosc/32
        //   1    0     1   fosc/64
        //   1    1     0   fosc/64
        //   1    1     1   fosc/128

        // We find the fastest clock that is less than or equal to the
        // given clock rate. The clock divider that results in clock_setting
        // is 2 ^^ (clock_div + 1). If nothing is slow enough, we'll use the
        // slowest (128 == 2 ^^ 7, so clock_div = 6).
        uint8_t clockDiv;

        // When the clock is known at compiletime, use this if-then-else
        // cascade, which the compiler knows how to completely optimize
        // away. When clock is not known, use a loop instead, which generates
        // shorter code.
        if (__builtin_constant_p(clock)) {
            if (clock >= F_CPU / 2) {
                clockDiv = 0;
            }
            else if (clock >= F_CPU / 4) {
                clockDiv = 1;
            }
            else if (clock >= F_CPU / 8) {
                clockDiv = 2;
            }
            else if (clock >= F_CPU / 16) {
                clockDiv = 3;
            }
            else if (clock >= F_CPU / 32) {
                clockDiv = 4;
            }
            else if (clock >= F_CPU / 64) {
                clockDiv = 5;
            }
            else {
                clockDiv = 6;
            }
        }
        else {
            uint32_t clockSetting = F_CPU / 2;
            clockDiv = 0;
            while (clockDiv < 6 && clock < clockSetting) {
                clockSetting /= 2;
                clockDiv++;
            }
        }

        // Compensate for the duplicate fosc/64
        if (clockDiv == 6)
            clockDiv = 7;

        // Invert the SPI2X bit
        clockDiv ^= 0x1;

        // Pack into the SPISettings class
        spcr = _BV(SPE) | _BV(MSTR) | ((bitOrder == LSBFIRST) ? _BV(DORD) : 0) |
            (dataMode & SPI_MODE_MASK) | ((clockDiv >> 1) & SPI_CLOCK_MASK);
        spsr = clockDiv & SPI_2XCLOCK_MASK;
    }

    uint8_t spcr;
    uint8_t spsr;
    friend class SimpleSPIClass;
};



class SimpleSPIClass {

public:
	static void begin();

    inline static void beginTransaction(SPISettings settings) {
        if (interruptMode > 0) {
            uint8_t sreg = SREG;
            noInterrupts();

#ifdef SPI_AVR_EIMSK
            if (interruptMode == 1) {
                interruptSave = SPI_AVR_EIMSK;
                SPI_AVR_EIMSK &= ~interruptMask;
                SREG = sreg;
            }
            else
#endif
            {
                interruptSave = sreg;
            }
        }

#ifdef SPI_TRANSACTION_MISMATCH_LED
        if (inTransactionFlag) {
            pinMode(SPI_TRANSACTION_MISMATCH_LED, OUTPUT);
            digitalWrite(SPI_TRANSACTION_MISMATCH_LED, HIGH);
        }
        inTransactionFlag = 1;
#endif

        SPCR = settings.spcr;
        SPSR = settings.spsr;
    }

	inline static void setBitOrder(uint8_t bitOrder) {
		if (bitOrder == LSBFIRST) SPCR |= _BV(DORD);
		else SPCR &= ~(_BV(DORD));
	}

    // Write to the SPI bus (MOSI pin) and also receive (MISO pin)
    inline static uint8_t transfer(uint8_t data) {
        SPDR = data;
        /*
         * The following NOP introduces a small delay that can prevent the wait
         * loop form iterating when running at the maximum speed. This gives
         * about 10% more speed, even if it seems counter-intuitive. At lower
         * speeds it is unnoticed.
         */
        asm volatile("nop");
        while (!(SPSR & _BV(SPIF))); // wait
        return SPDR;
    }


protected:
    int writeRegister(uint8_t subAddress, uint8_t data);
	
private: 
	static uint8_t initialized;
	static int initialize();
    static uint8_t interruptMode; // 0=none, 1=mask, 2=global
    static uint8_t interruptMask; // which interrupts to mask
    static uint8_t interruptSave; // temp storage, to restore state

};

extern SimpleSPIClass spi;

