#include <Wire.h>
#include <Adafruit_NeoPixel.h>
//#include "../../RTClib/RTClib.h"
//#include "E:/_work/arduino/led-ceiling/dormcxambro/RTClib/RTClib.h"
#include "ChronoDotSaru.h"

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60*4, PIN, NEO_GRB + NEO_KHZ800);

ChronoDotSaru s_chronoDot;
unsigned secondLastTempUpdate;
unsigned secondsSinceTempUpdate;
int      mode = 0;
 
void setup () {

    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    Wire.begin();
    Serial.begin(9600);

    s_chronoDot.Init();
    //s_chronoDot.SetTwelveHourMode(false);
    /*
    s_chronoDot.SetYear(2014);
    s_chronoDot.SetMonth(2);
    s_chronoDot.SetDayOfMonth(9);
    s_chronoDot.SetDayOfWeek(ChronoDotSaru::DAY_OF_WEEK_SUNDAY);
    //*/
    //*
    //s_chronoDot.SetHour24(18);
    //s_chronoDot.SetHour12(9, true);
    //s_chronoDot.SetMinute(23);
    //s_chronoDot.SetSecond(00);
    //*/

    s_chronoDot.SetHour12(7, false, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetMinute(5, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetSecond(0, ChronoDotSaru::CLOCK_ALARM_1);
    //s_chronoDot.SetHour12(10, true, ChronoDotSaru::CLOCK_ALARM_1);
    //s_chronoDot.SetMinute(38, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.AlarmEnable(ChronoDotSaru::CLOCK_ALARM_1);

    secondLastTempUpdate = 0;
    secondsSinceTempUpdate = 0;

}
 
void loop () {

    if (!mode) {
        s_chronoDot.UpdateCacheRange(ChronoDotSaru::REGISTER_SECONDS, ChronoDotSaru::REGISTER_YEAR);

        bool pmNotAm        = s_chronoDot.PmNotAm();
        bool twelveHourMode = s_chronoDot.TwelveHourMode();
        int  hours          = s_chronoDot.Hour();
        int  minutes        = s_chronoDot.Minute();
        int  seconds        = s_chronoDot.Second();

        Serial.print("  "); Serial.print(s_chronoDot.DayOfWeekName()); Serial.print(", "); Serial.print(s_chronoDot.MonthName());
            Serial.print(" "); Serial.print(s_chronoDot.DayOfMonth()); Serial.print(", "); Serial.println(s_chronoDot.Year());
        Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":");
        if (twelveHourMode) {
            Serial.print(seconds); Serial.println(pmNotAm ? " PM" : " AM");
        }
        else {
            Serial.println(seconds);
        }

        if (secondLastTempUpdate != seconds) {
            secondLastTempUpdate = seconds;
            if (++secondsSinceTempUpdate >= 10) {
                secondsSinceTempUpdate = 0;
                int8_t temp = s_chronoDot.GetTempC(true);
                Serial.print("Temp (C): "); Serial.println(temp);
            }
        }

        /*
        uint8_t test;
        s_chronoDot.GetRegister(ChronoDotSaru::REGISTER_STATUS, &test, 1, true);
        Serial.print("Test register: "); Serial.println(test);
        //*/

        if (s_chronoDot.AlarmFlagStatus(ChronoDotSaru::CLOCK_ALARM_1, true)) {
            Serial.println("Alarm!!");
            mode = 1;
        }
 
        delay(1000);
    }
    else {
        // Some example procedures showing how to display to the pixels:
        colorWipe(strip.Color(255, 0, 0), 50); // Red
        colorWipe(strip.Color(0, 255, 0), 50); // Green
        colorWipe(strip.Color(0, 0, 255), 50); // Blue
        // Send a theater pixel chase in...
        theaterChase(strip.Color(127, 127, 127), 50); // White
        theaterChase(strip.Color(127,   0,   0), 50); // Red
        theaterChase(strip.Color(  0,   0, 127), 50); // Blue

        rainbow(20);
        rainbowCycle(20);
        theaterChaseRainbow(50);
    }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
