#include "ChronoDotSaru.h"
#include <Wire.h>

//=============================================================================
// Local helpers
//=============================================================================

//=============================================================================
uint8_t BcdFromDecimal (uint8_t decimal) {
    return ((decimal / 10) << 4) | (decimal % 10);
}

//=============================================================================
uint8_t DecimalFromBcd (uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}


//=============================================================================
// ChronoDotSaru
//=============================================================================

//=============================================================================
void ChronoDotSaru::AlarmClearStatusFlag (EClock alarm) {
    SetRegisterFlagsTo(
        REGISTER_STATUS,
        STATUS_REGISTER_FLAG_ALARM_1_FLAG << alarm,
        false
    );
}

//=============================================================================
void ChronoDotSaru::AlarmDisable (EClock alarm) {
    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        false
    );
}

//=============================================================================
void ChronoDotSaru::AlarmEnable (EClock alarm) {

    if (alarm == CLOCK_TIME)
        return;

    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        true
    );

    // TODO : Don't just assume hour-minute matching mode.
    if (alarm == CLOCK_ALARM_1) {
        SetRegisterFlagsTo(REGISTER_ALARM_1_SECONDS, 0x80, false);             // A1M1
        SetRegisterFlagsTo(REGISTER_ALARM_1_MINUTES, 0x80, false);             // A1M2
        SetRegisterFlagsTo(REGISTER_ALARM_1_HOURS, 0x80, false);               // A1M3
        SetRegisterFlagsTo(REGISTER_ALARM_1_DAY_OF_WEEK_OR_MONTH, 0x80, true); // A1M4
    }
    else {
        SetRegisterFlagsTo(REGISTER_ALARM_2_MINUTES, 0x80, false);             // A1M2
        SetRegisterFlagsTo(REGISTER_ALARM_2_HOURS, 0x80, false);               // A1M3
        SetRegisterFlagsTo(REGISTER_ALARM_2_DAY_OF_WEEK_OR_MONTH, 0x80, true); // A1M4
    }

}

//=============================================================================
bool ChronoDotSaru::AlarmEnabledStatus (EClock alarm, bool updateCache) {

    if (updateCache)
        UpdateCache(REGISTER_CONTROL, 1);

    return registerCache[REGISTER_CONTROL] & (CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm);

}

//=============================================================================
bool ChronoDotSaru::AlarmFlagStatus (EClock alarm, bool updateCache) {

    if (updateCache)
        UpdateCache(REGISTER_STATUS, 1);

    return registerCache[REGISTER_STATUS] & (STATUS_REGISTER_FLAG_ALARM_1_FLAG << alarm);

}

//=============================================================================
void ChronoDotSaru::AlarmToggle (EClock alarm) {
    SetRegisterFlagsTo(
        REGISTER_CONTROL,
        CONTROL_REGISTER_FLAG_ALARM_1_ENABLE << alarm,
        true
    );
}

//=============================================================================
uint8_t ChronoDotSaru::DecimalHourFromRegister (ERegister reg) {

    // 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // 0 | M | P | H |     hour      |
    //       __
    // M: 12/24 hour mode
    //    __
    // P: AM/PM  OR  20 hour (BCD)
    // H: 10 hour (BCD)

    uint8_t regVal = registerCache[reg];

    // 12 hour mode?
    if (regVal & 0x40)
        return DecimalFromBcd(regVal & 0x1F);

    return DecimalFromBcd(regVal & 0x3F);

}

//=============================================================================
uint8_t ChronoDotSaru::DayOfMonth () {
    return DecimalFromBcd(registerCache[REGISTER_DAY_OF_MONTH]);
}

//=============================================================================
ChronoDotSaru::EDayOfWeek ChronoDotSaru::DayOfWeek () {
    return EDayOfWeek(registerCache[REGISTER_DAY_OF_WEEK]);
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
uint8_t ChronoDotSaru::Hour (EClock clock) {
    return DecimalHourFromRegister(HourRegisterFromClock(clock));
}

//=============================================================================
ChronoDotSaru::ERegister ChronoDotSaru::HourRegisterFromClock (EClock clock) {

    switch (clock) {
        case CLOCK_ALARM_1: return REGISTER_ALARM_1_HOURS;
        case CLOCK_ALARM_2: return REGISTER_ALARM_2_HOURS;
        case CLOCK_TIME:    return REGISTER_HOURS;
    }
    return REGISTERS;

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
    AlarmClearStatusFlag(ChronoDotSaru::CLOCK_ALARM_1);
    AlarmClearStatusFlag(ChronoDotSaru::CLOCK_ALARM_2);
    UpdateCache(REGISTERS, REGISTERS);

}

//=============================================================================
uint8_t ChronoDotSaru::Minute (EClock clock) {

    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 minutes |    minutes    |
    return DecimalFromBcd(registerCache[MinuteRegisterFromClock(clock)]);

}

//=============================================================================
ChronoDotSaru::ERegister ChronoDotSaru::MinuteRegisterFromClock (EClock clock) {

    switch (clock) {
        case CLOCK_ALARM_1: return REGISTER_ALARM_1_MINUTES;
        case CLOCK_ALARM_2: return REGISTER_ALARM_2_MINUTES;
        case CLOCK_TIME:    return REGISTER_MINUTES;
    }
    return REGISTERS;

}

//=============================================================================
uint8_t ChronoDotSaru::Month () {
    return DecimalFromBcd(registerCache[REGISTER_MONTH_AND_CENTURY] & 0x1F);
}

//=============================================================================
bool ChronoDotSaru::PmNotAm (EClock clock) {
    ERegister reg = HourRegisterFromClock(clock);
    if (TwelveHourMode())
        return registerCache[reg] & 0x40;
    return DecimalHourFromRegister(reg) >= 12;
}

//=============================================================================
uint8_t ChronoDotSaru::Second (EClock clock) {

    // 7 | 6 | 5 | 4  | 3 | 2 | 1 | 0 |
    // 0 | 10 seconds |    seconds    |
    switch (clock) {
        case CLOCK_ALARM_1:
            return DecimalFromBcd(registerCache[REGISTER_ALARM_1_SECONDS]);
        // (Alarm 2 doesn't have a seconds register.)
        case CLOCK_TIME:
            return DecimalFromBcd(registerCache[REGISTER_SECONDS]);
    }

    return 0;

}

//=============================================================================
ChronoDotSaru::ERegister ChronoDotSaru::SecondRegisterFromClock (EClock clock) {

    switch (clock) {
        case CLOCK_ALARM_1:
            return REGISTER_ALARM_1_SECONDS;
        case CLOCK_TIME:
            return REGISTER_SECONDS;
    }

    return REGISTERS;

}

//=============================================================================
void ChronoDotSaru::SetDayOfMonth (uint8_t day) {
    SetRegister(REGISTER_DAY_OF_MONTH, BcdFromDecimal(day));
}

//=============================================================================
void ChronoDotSaru::SetDayOfWeek (EDayOfWeek day) {
    SetRegister(REGISTER_DAY_OF_WEEK, BcdFromDecimal(day));
}

//=============================================================================
void ChronoDotSaru::SetHour12 (uint8_t hours, bool pmNotAm, EClock clock) {

    uint8_t hoursBcd = BcdFromDecimal(hours);
    if (pmNotAm)
        hoursBcd |= 0x20;

    ERegister reg = HourRegisterFromClock(clock);
    SetRegister(reg, hoursBcd | (registerCache[reg] & 0x80) | 0x40);

}

//=============================================================================
void ChronoDotSaru::SetHour24 (uint8_t hours, EClock clock) {

    ERegister reg = HourRegisterFromClock(clock);
    uint8_t bcd = BcdFromDecimal(hours);
    //Serial.print("SetHour24: BcdFromDecimal result: "); Serial.print(hours); Serial.print(" => "); Serial.println(bcd);
    SetRegister(reg, BcdFromDecimal(hours) | (registerCache[reg] & 0x80));

}

//=============================================================================
void ChronoDotSaru::SetMinute (uint8_t minutes, EClock clock) {

    ERegister reg = MinuteRegisterFromClock(clock);
    SetRegister(reg, BcdFromDecimal(minutes) | (registerCache[reg] & 0x80));

}

//=============================================================================
void ChronoDotSaru::SetMonth (uint8_t month) {
    //UpdateCache(REGISTER_MONTH_AND_CENTURY, 1);
    SetRegister(
        REGISTER_MONTH_AND_CENTURY,
        (
            BcdFromDecimal(month) |
            (registerCache[REGISTER_MONTH_AND_CENTURY] & 0x80)
        )
    );
}

//=============================================================================
void ChronoDotSaru::SetRegister (ERegister reg, uint8_t value) {

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);   // select register
    Wire.write(value); // write bitmask
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
void ChronoDotSaru::SetSecond (uint8_t seconds, EClock clock) {

    if (clock == CLOCK_ALARM_2)
        return;

    ERegister reg = SecondRegisterFromClock(clock);
    SetRegister(reg, BcdFromDecimal(seconds) | (registerCache[reg] & 0x80));

}

//=============================================================================
void ChronoDotSaru::SetTwelveHourMode (bool twelveHourMode, EClock clock) {
    ERegister reg           = HourRegisterFromClock(clock);
    uint8_t   regValue      = registerCache[reg];
    bool      wasTwelveHour = regValue & 0x40;

    if (wasTwelveHour == twelveHourMode)
        return;

    if (wasTwelveHour) {
        SetHour24((Hour(clock) % 12) + (PmNotAm(clock) ? 12 : 0), clock);
    }
    else {
        uint8_t hour    = Hour(clock);
        uint8_t hourMod = hour % 12;
        SetHour12(hourMod ? hourMod : 12, hour >= 12, clock);
        Serial.print("Set12HM: Was 24.  "); Serial.println(registerCache[reg]);
    }
}

//=============================================================================
void ChronoDotSaru::SetYear (int year) {

    SetRegister(REGISTER_YEAR, BcdFromDecimal(year % 100));
    //UpdateCache(REGISTER_MONTH_AND_CENTURY, 1);
    SetRegisterFlagsTo(REGISTER_MONTH_AND_CENTURY, 0x80, ((year - 1900) / 100) % 2);

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

//=============================================================================
int ChronoDotSaru::Year () {
    return
        1900 +
        (registerCache[REGISTER_MONTH_AND_CENTURY] ? 100 : 0) +
        DecimalFromBcd(registerCache[REGISTER_YEAR]);
}
