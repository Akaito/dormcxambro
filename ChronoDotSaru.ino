#include "ChronoDotSaru.h"
#include <Wire.h>


//=============================================================================
void ChronoDotSaru::AlarmDisable (EAlarm alarm) {
    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        false
    );
}

//=============================================================================
void ChronoDotSaru::AlarmEnable (EAlarm alarm) {
    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        true
    );
}

//=============================================================================
void ChronoDotSaru::AlarmToggle (EAlarm alarm) {
    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        true
    );
}

//=============================================================================
bool ChronoDotSaru::AlarmEnabledStatus (EAlarm alarm, bool updateCache) {

    if (updateCache)
        UpdateCache(REGISTER_STATUS, 1);

    return registerCache[REGISTER_STATUS] & (STATUS_REGISTER_FLAG_ALARM_1_FLAG << alarm);

}

//=============================================================================
void ChronoDotSaru::AlarmClearStatusFlag (EAlarm alarm) {
    SetRegisterFlagsTo(
        REGISTER_STATUS,
        STATUS_REGISTER_FLAG_ALARM_1_FLAG << alarm,
        false
    );
}

//=============================================================================
void ChronoDotSaru::GetRegister (
    ERegister reg,
    uint8_t * out,
    uint8_t outBytes,
    bool updateCache
) {

    if (updateCache)
        UpdateCache(reg, outBytes);

    for (uint8_t i = 0;  i < outBytes; ++i)
        out[i] = registerCache[reg + i];

}

//=============================================================================
int8_t ChronoDotSaru::GetTempC (bool updateCache) {

    if (updateCache) {
        SetRegisterFlagsTo(
            REGISTER_CONTROL,
            CONTROL_REGISTER_FLAG_CONVERT_TEMPERATURE,
            true
        );
        UpdateCacheRange(REGISTER_TEMP_MSB, REGISTER_TEMP_LSB);
    }

    return *reinterpret_cast<int8_t*>(&registerCache[REGISTER_TEMP_MSB]);

}

//=============================================================================
uint8_t ChronoDotSaru::Hours () {

    // 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // 0 | M | P | H |     hour      |
    //       __
    // M: 12/24 hour mode
    //    __
    // P: AM/PM  OR  20 hour (BCD)
    // H: 10 hour (BCD)

    uint8_t reg  = registerCache[REGISTER_HOURS];
    uint8_t hours = ((reg >> 4) & 0x01) * 10 + (reg & 0x0F);

    if (!(reg & 0x40)) // 24 hour mode?
        hours += ((reg >> 5) & 0x01) * 20;

    return hours;

}

//=============================================================================
void ChronoDotSaru::Init () {
    
    // clear /EOSC bit
    // Sometimes necessary to ensure that the clock keeps running on just
    // battery power. Once set, it shouldn't need to be reset but it's a good
    // idea to make sure.
    // bit 7 is /EOSC // 00011100
    SetRegister(
        REGISTER_CONTROL,
        (
            CONTROL_REGISTER_FLAG_INTERRUPT_CONTROL |
            CONTROL_REGISTER_FLAG_RATE_SELECT_1     |
            CONTROL_REGISTER_FLAG_RATE_SELECT_2
        )
    );

}

//=============================================================================
uint8_t ChronoDotSaru::Minutes () {

    uint8_t minutes = registerCache[REGISTER_MINUTES];
    // convert BCD to decimal
    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 minutes |    minutes    |
    return (((minutes & 0xF0)>>4)*10 + (minutes & 0x0F));

}

//=============================================================================
uint8_t ChronoDotSaru::Seconds () {

    uint8_t seconds = registerCache[REGISTER_SECONDS];
    // convert BCD to decimal
    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 seconds |    seconds    |
    return (((seconds & 0xF0)>>4)*10 + (seconds & 0x0F));

}

//=============================================================================
void ChronoDotSaru::SetRegister (ERegister reg, uint8_t flags) {

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg); // select register
    Wire.write(flags);            // write bitmask
    if (Wire.endTransmission() == 0)
        registerCache[reg] = flags;

}

//=============================================================================
void ChronoDotSaru::SetRegisterFlagsTo (ERegister reg, uint8_t flags, bool value) {

    uint8_t desired = registerCache[reg];
    if (value)
        desired = registerCache[reg] | flags;
    else
        desired = registerCache[reg] & ~flags;

    SetRegister(reg, desired);

}

//=============================================================================
bool ChronoDotSaru::UpdateCache (ERegister reg, unsigned bytes) {

    // Giving REGISTERS indicates desire to read every register
    if (reg >= REGISTERS) {
        reg   = ERegister(0);
        bytes = REGISTERS;
    }

    // Protect against asking for too many bytes from the ChronoDot
    if (reg + bytes > REGISTERS)
        bytes = REGISTERS - reg;

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);                     // Specify start address to read from
    if (Wire.endTransmission())          // Zero indicates success
        return false;

    if (Wire.requestFrom(I2C_ADDRESS, bytes) < bytes)
        return false;

    unsigned outIndex = 0;
    while (outIndex < bytes && Wire.available())
        registerCache[reg + outIndex++] = Wire.read();

    // Only successful if all requested bytes were read
    return bytes == outIndex;

}

//=============================================================================
bool ChronoDotSaru::UpdateCacheRange (ERegister reg, ERegister endInclusive) {

    return UpdateCache(reg, (endInclusive - reg) + 1);

}
