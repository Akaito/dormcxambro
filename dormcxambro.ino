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

    s_chronoDot.UpdateCacheRange(ChronoDotSaru::REGISTER_SECONDS, ChronoDotSaru::REGISTER_YEAR);

    int hours   = s_chronoDot.Hours();
    int minutes = s_chronoDot.Minutes();
    int seconds = s_chronoDot.Seconds();

    Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.println(seconds);

    if (secondLastTempUpdate != seconds) {
        secondLastTempUpdate = seconds;
        if (++secondsSinceTempUpdate >= 10) {
            secondsSinceTempUpdate = 0;
            int8_t temp = s_chronoDot.GetTempC(true);
            Serial.print("Temp (C): "); Serial.println(temp);
        }
    }

    uint8_t test;
    s_chronoDot.GetRegister(ChronoDotSaru::REGISTER_STATUS, &test, 1, true);
    Serial.print("Status register: "); Serial.println(test);

 
    delay(1000);
}
