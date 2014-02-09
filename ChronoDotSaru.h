#pragma once

// macetech ChronoDot v2.0 datasheet: http://datasheets.maximintegrated.com/en/ds/DS3231.pdf
struct ChronoDotSaru {

//=============================================================================
// Constants and types

    static const uint8_t I2C_ADDRESS = 0x68; // ChronoDot v2.0 (DS3231) I2C address

    enum ERegister {
        REGISTER_SECONDS = 0x00,
        REGISTER_MINUTES,
        REGISTER_HOURS,
        REGISTER_DAY_OF_WEEK,
        REGISTER_DAY_OF_MONTH,
        REGISTER_MONTH_AND_CENTURY,
        REGISTER_YEAR,
        REGISTER_ALARM_1_SECONDS,
        REGISTER_ALARM_1_MINUTES,
        REGISTER_ALARM_1_HOURS,
        REGISTER_ALARM_1_DAY_OF_WEEK_OR_MONTH,
        REGISTER_ALARM_2_SECONDS,
        REGISTER_ALARM_2_MINUTES,
        REGISTER_ALARM_2_HOURS,
        REGISTER_ALARM_2_DAY_OF_WEEK_OR_MONTH,
        REGISTER_CONTROL,
        REGISTER_STATUS,                        // Also has some control features
        REGISTER_AGING_OFFSET,
        REGISTER_TEMP_MSB,                      // Signed 8-bit int (Celsius)
        REGISTER_TEMP_LSB,                      // Bits 6 and 7 hold fractional temp in 0.25 increments
        REGISTERS
    };
    static const uint8_t ALARM_REGISTER_COUNT = REGISTER_ALARM_2_SECONDS - REGISTER_ALARM_1_SECONDS;

    enum EControlRegisterFlags {
        // If an alarm bit and _INTERRUPT_CONTROL are set, permits the
        // corresponding alarm flag to be set in the status register.
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE              = (1 << 0), // Power-on state: 0
        CONTROL_REGISTER_FLAG_ALARM_2_ENABLE              = (1 << 1), // Power-on state: 0
        // 0: Square wave output on INT/SQW pin.
        // 1: Square wave only output when time matches an enabled alarm.
        CONTROL_REGISTER_FLAG_INTERRUPT_CONTROL           = (1 << 2), // Power-on state: 1
        // Rate select 2 and 1 control the SQW output rate
        //          |  0  |  0  | ==> 1Hz
        //          |  0  |  1  | ==> 1.024kHz
        //          |  1  |  0  | ==> 4.096kHz
        //          |  1  |  1  | ==> 8.192kHz  (default)
        CONTROL_REGISTER_FLAG_RATE_SELECT_1               = (1 << 3), // Power-on state: 1
        CONTROL_REGISTER_FLAG_RATE_SELECT_2               = (1 << 4), // Power-on state: 1
        // Set to initiate temperature conversion.  Stays set until conversion completes.
        CONTROL_REGISTER_FLAG_CONVERT_TEMPERATURE         = (1 << 5), // Power-on state: 0
        // 0: INT/SQW pin goes high impedance when Vcc < Vpf.
        // 1: If _INTERRUPT_CONTROL set and Vcc < Vpf, enable square wave.
        CONTROL_REGISTER_FLAG_BATTERY_BACKED_SQUARE_WAVE  = (1 << 6), // Power-on state: 0
        // 1: Oscillator is stopped when ChronoDot switches to Vbat.
        // Starts when set to 0.  Oscillator is always on when on Vcc.
        CONTROL_REGISTER_FLAG_DISABLE_OSCILLATOR_ON_VBATT = (1 << 7), // Power-on state: 0
    };

    enum EStatusRegisterFlags {
        // 0: Write to zero to clear the flag.  Cannot manually write to 1.
        // 1: Time matched alarm's value.
        STATUS_REGISTER_FLAG_ALARM_1_FLAG        = (1 << 0), // Power-on state: X
        STATUS_REGISTER_FLAG_ALARM_2_FLAG        = (1 << 1), // Power-on state: X
        // 1: Device busy executing TCXO functions (temperature conversion).
        STATUS_REGISTER_FLAG_BUSY                = (1 << 2), // Power-on state: X
        // 0: 32KhZ pin goes to a high-impedance state.
        // 1: 32kHz pin outputs a 32.768kHz square-wave (if oscillator is running).
        STATUS_REGISTER_FLAG_ENABLE_32kHz_OUTPUT = (1 << 3), // Power-on state: 1
        //STATUS_REGISTER_FLAG_UNUSED            = (1 << 4), // Power-on state: 0
        //STATUS_REGISTER_FLAG_UNUSED            = (1 << 5), // Power-on state: 0
        //STATUS_REGISTER_FLAG_UNUSED            = (1 << 6), // Power-on state: 0
        // Goes to 1 when the oscillator stops running.
        // Remains at 1 until written to 0.
        STATUS_REGISTER_FLAG_OSCILLATOR_STOPPED  = (1 << 7), // Power-on state: 1
    };

    enum EAlarm {
        ALARM_1 = 0,
        ALARM_2,
    };

//=============================================================================
// Data

    uint8_t registerCache[REGISTERS];

//=============================================================================
// Methods

    void Init ();

    // Blocks.  Returns false when not all bytes could be read.
    bool UpdateCache (ERegister reg, unsigned bytes);
    bool UpdateCacheRange (ERegister reg, ERegister endInclusive);

    void SetRegister (ERegister reg, uint8_t flags);
    void SetRegisterFlagsTo (ERegister reg, uint8_t flags, bool value);
    void GetRegister (
        ERegister reg,
        uint8_t * out,
        uint8_t outBytes,
        bool updateCache
    );

    // Simple alarm functionality
    void AlarmEnable (EAlarm alarm);
    void AlarmDisable (EAlarm alarm);
    void AlarmToggle (EAlarm alarm);
    bool AlarmEnabledStatus (EAlarm alarm, bool updateCache);
    bool AlarmFlagStatus (EAlarm alarm, bool updateCache);
    void AlarmClearStatusFlag (EAlarm alarm);

    int8_t GetTempC (bool updateCache);

};