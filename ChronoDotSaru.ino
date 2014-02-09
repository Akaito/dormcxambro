#include "ChronoDotSaru.h"
#include <Wire.h>

//=============================================================================
// Local helpers
//=============================================================================

//=============================================================================
uint8_t BcdFromDecimal (uint8_t decimal) {
    return ((decimal / 10) << 4) + (decimal % 10);
}

//=============================================================================
uint8_t DecimalFromBcd (uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}


//=============================================================================
// ChronoDotSaru
//=============================================================================

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

    uint8_t reg = registerCache[REGISTER_HOURS];

    if (reg & 0x40) // 12 hour mode?
        return DecimalFromBcd(reg & 0x1F);

    return DecimalFromBcd(reg & 0x3F);

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

    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 minutes |    minutes    |
    return DecimalFromBcd(registerCache[REGISTER_MINUTES]);

}

//=============================================================================
uint8_t ChronoDotSaru::Seconds () {

    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 seconds |    seconds    |
    return DecimalFromBcd(registerCache[REGISTER_SECONDS]);

}

//=============================================================================
void ChronoDotSaru::SetHours (uint8_t hours, bool twelveHourMode) {

    uint8_t val = BcdFromDecimal(hours);
    if (twelveHourMode)
        val |= 0x40;

    SetRegister(REGISTER_HOURS, hours);

}

//=============================================================================
void ChronoDotSaru::SetMinutes (uint8_t minutes) {
    SetRegister(REGISTER_MINUTES, BcdFromDecimal(minutes));
}

//=============================================================================
void ChronoDotSaru::SetRegister (ERegister reg, uint8_t value) {

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg); // select register
    Wire.write(value);            // write bitmask
    if (Wire.endTransmission() == 0)
        registerCache[reg] = value;

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
void ChronoDotSaru::SetSeconds (uint8_t seconds) {
    SetRegister(REGISTER_SECONDS, BcdFromDecimal(seconds));
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
