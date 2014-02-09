#include <Wire.h>
//#include "../../RTClib/RTClib.h"
//#include "E:/_work/arduino/led-ceiling/dormcxambro/RTClib/RTClib.h"
#include "ChronoDotSaru.h"

ChronoDotSaru s_chronoDot;
unsigned secondLastTempUpdate;
unsigned secondsSinceTempUpdate;
 
void setup () {
    Wire.begin();
    Serial.begin(9600);
 
    s_chronoDot.Init();
    secondLastTempUpdate = 0;
    secondsSinceTempUpdate = 0;
}
 
void loop () {
    // send request to receive data starting at register 0
    /*
    Wire.beginTransmission(ChronoDotSaru::I2C_ADDRESS); // 0x68 is DS3231 device address
    Wire.write((byte)0); // start at register 0
    Wire.endTransmission();
    Wire.requestFrom(ChronoDotSaru::I2C_ADDRESS, uint8_t(3)); // request three bytes (seconds, minutes, hours)
    */
    uint8_t buff[3] = {0};
    s_chronoDot.GetRegister(ChronoDotSaru::REGISTER_SECONDS, buff, 3, true);

    //while (Wire.available()) {
        //int seconds = Wire.read(); // get seconds
        //int minutes = Wire.read(); // get minutes
        //int hours = Wire.read();     // get hours
        int seconds = buff[0];
        int minutes = buff[1];
        int hours = buff[2];
 
        seconds = (((seconds & 0b11110000)>>4)*10 + (seconds & 0b00001111)); // convert BCD to decimal
        minutes = (((minutes & 0b11110000)>>4)*10 + (minutes & 0b00001111)); // convert BCD to decimal
        hours = (((hours & 0b00100000)>>5)*20 + ((hours & 0b00010000)>>4)*10 + (hours & 0b00001111)); // convert BCD to decimal (assume 24 hour mode)
 
        Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.println(seconds);

        if (secondLastTempUpdate != seconds) {
            secondLastTempUpdate = seconds;
            if (++secondsSinceTempUpdate >= 10) {
                secondsSinceTempUpdate = 0;
                int8_t temp = s_chronoDot.GetTempC(true);
                Serial.print("Temp (C): "); Serial.println(temp);
            }
        }
    //}

    uint8_t test;
    s_chronoDot.GetRegister(ChronoDotSaru::REGISTER_STATUS, &test, 1, true);
    Serial.print("Status register: "); Serial.println(test);

 
    delay(1000);
}
