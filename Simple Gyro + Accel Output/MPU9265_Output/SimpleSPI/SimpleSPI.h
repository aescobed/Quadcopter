
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
    
    // Registers
    const uint8_t ACCEL_OUT = 0x3B;
    const uint8_t GYRO_OUT = 0x43;
    const uint8_t TEMP_OUT = 0x41;

    const uint8_t SPI_READ = 0x80;
    const uint32_t LS_CLOCK = 1000000;  // 1 MHz
    const uint32_t HS_CLOCK = 15000000; // 15 MHz
    const uint8_t SSPin = 10;
    

public:
	static void begin();


    // Before using SPI.transfer() or asserting chip select pins,
    // this function is used to gain exclusive access to the SPI bus
    // and configure the correct settings.
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



    // After performing a group of transfers and releasing the chip select
    // signal, this function allows others to access the SPI bus
    inline static void endTransaction(void) {
#ifdef SPI_TRANSACTION_MISMATCH_LED
        if (!inTransactionFlag) {
            pinMode(SPI_TRANSACTION_MISMATCH_LED, OUTPUT);
            digitalWrite(SPI_TRANSACTION_MISMATCH_LED, HIGH);
        }
        inTransactionFlag = 0;
#endif

        if (interruptMode > 0) {
#ifdef SPI_AVR_EIMSK
            uint8_t sreg = SREG;
#endif
            noInterrupts();
#ifdef SPI_AVR_EIMSK
            if (interruptMode == 1) {
                SPI_AVR_EIMSK = interruptSave;
                SREG = sreg;
            }
            else
#endif
            {
                SREG = interruptSave;
            }
        }
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
    int readRegisters(uint8_t subAddress, uint8_t count, uint8_t* dest);
    int readSensor();

    enum GyroRange
    {
        GYRO_RANGE_250DPS,
        GYRO_RANGE_500DPS,
        GYRO_RANGE_1000DPS,
        GYRO_RANGE_2000DPS
    };
    enum AccelRange
    {
        ACCEL_RANGE_2G,
        ACCEL_RANGE_4G,
        ACCEL_RANGE_8G,
        ACCEL_RANGE_16G
    };
    enum DlpfBandwidth
    {
        DLPF_BANDWIDTH_184HZ,
        DLPF_BANDWIDTH_92HZ,
        DLPF_BANDWIDTH_41HZ,
        DLPF_BANDWIDTH_20HZ,
        DLPF_BANDWIDTH_10HZ,
        DLPF_BANDWIDTH_5HZ
    };

    // buffer for sensor data
    uint8_t buffer[21];

    // data counts
    int16_t _axcounts, _aycounts, _azcounts;
    int16_t _gxcounts, _gycounts, _gzcounts;
    int16_t _hxcounts, _hycounts, _hzcounts;
    int16_t _tcounts;

    // data buffer
    float _ax, _ay, _az;
    float _gx, _gy, _gz;
    float _hx, _hy, _hz;
    float _t;

    // scale factors
    float _accelScale;
    float _gyroScale;
    float _magScaleX, _magScaleY, _magScaleZ;
    const float _tempScale = 333.87f;
    const float _tempOffset = 21.0f;
    // configuration
    AccelRange _accelRange;
    GyroRange _gyroRange;
    DlpfBandwidth _bandwidth;
    uint8_t _srd;
    // gyro bias estimation
    size_t _numSamples = 100;
    double _gxbD, _gybD, _gzbD;
    float _gxb, _gyb, _gzb;
    // accel bias and scale factor estimation
    double _axbD, _aybD, _azbD;
    float _axmax, _aymax, _azmax;
    float _axmin, _aymin, _azmin;
    float _axb, _ayb, _azb;
    float _axs = 1.0f;
    float _ays = 1.0f;
    float _azs = 1.0f;
    float _hxb, _hyb, _hzb;
    float _hxs = 1.0f;
    float _hys = 1.0f;
    float _hzs = 1.0f;
    float _avgs;
    // transformation matrix
    /* transform the accel and gyro axes to match the magnetometer axes */
    const int16_t tX[3] = { 0,  1,  0 };
    const int16_t tY[3] = { 1,  0,  0 };
    const int16_t tZ[3] = { 0,  0, -1 };
	
private: 
	static uint8_t initialized;
	static int initialize();
    static uint8_t interruptMode; // 0=none, 1=mask, 2=global
    static uint8_t interruptMask; // which interrupts to mask
    static uint8_t interruptSave; // temp storage, to restore state

};

extern SimpleSPIClass spi;

