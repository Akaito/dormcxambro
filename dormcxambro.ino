#include <Wire.h>
#include <Adafruit_NeoPixel.h>
//#include "../../RTClib/RTClib.h"
//#include "E:/_work/arduino/led-ceiling/dormcxambro/RTClib/RTClib.h"
#include "ChronoDotSaru.h"

// Arduino
//#define STRIP0_PIN 6
//#define STRIP1_PIN 7
// Teensy
#define DEBUG_LED_PIN 13
#define STRIP0_PIN 11
#define STRIP1_PIN 12
#define STRIP0_PIXEL_COUNT 465
#define STRIP1_PIXEL_COUNT 496
#define BUTTON0_PIN 2
#define BUTTON1_PIN 3

#define WAIT_CHUNK_MS 20
#define EVENT_CAPACITY 6


// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip0 = Adafruit_NeoPixel(STRIP0_PIXEL_COUNT, STRIP0_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(STRIP1_PIXEL_COUNT, STRIP1_PIN, NEO_GRB + NEO_KHZ800);

// Note: We have 4 4m lengths, with 60 LEDs per meter (240 per 4m length, 960 total)

enum EMode {
    MODE_WAIT_FOR_ALARM = 0,
    MODE_ALARM,
    MODE_LED_MEASURE,
    MODES
};

enum EButtonState {
    BUTTON_STATE_UP = 0,
    BUTTON_STATE_DOWN,
};

enum EEvent {
    EVENT_NONE = 0,
    EVENT_BUTTON_0_PRESSED,
    EVENT_BUTTON_0_RELEASED,
    EVENT_BUTTON_1_PRESSED,
    EVENT_BUTTON_1_RELEASED,
};

struct PushButton {
    unsigned     pin;
    EButtonState state;
    EEvent       pressedEvent;
    EEvent       releasedEvent;
};

ChronoDotSaru s_chronoDot;
unsigned secondLastTempUpdate;
unsigned secondsSinceTempUpdate;
EMode    mode;
unsigned temp;
unsigned temp2;

PushButton pushButtons[] = {
    { BUTTON0_PIN, BUTTON_STATE_UP, EVENT_BUTTON_0_PRESSED },
    { BUTTON1_PIN, BUTTON_STATE_UP, EVENT_BUTTON_1_PRESSED },
};
#define PUSH_BUTTON_COUNT (sizeof(pushButtons) / sizeof(pushButtons[0]))

EEvent       events[EVENT_CAPACITY];
unsigned     eventIndex = 0;

void handleEvent(EEvent event);
bool inputLoop(uint8_t waitMs);
 
void setup () {

    temp2  = 1;
    memset(events, 0, sizeof(events));

    pinMode(DEBUG_LED_PIN, OUTPUT);
    pinMode(BUTTON0_PIN, INPUT_PULLUP);
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    digitalWrite(DEBUG_LED_PIN, HIGH); // test

    strip0.begin();
    strip0.show(); // Initialize all pixels to 'off'
    strip1.begin();
    strip1.show();

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
    //s_chronoDot.SetHour24(23);
    //s_chronoDot.SetHour12(9, true);
    //s_chronoDot.SetMinute(48);
    //s_chronoDot.SetSecond(30);
    //*/

    srandom((unsigned long)(s_chronoDot.Second()) << 8 | (unsigned long)(s_chronoDot.Second()));
    //mode = (random() % 2) ? MODE_WAIT_FOR_ALARM : MODE_ALARM;
    //mode = MODE_LED_MEASURE;
    mode = MODE_ALARM;
    temp = random(3);

    // Sunrise
    s_chronoDot.SetHour24(7, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetMinute(28, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetSecond(0, ChronoDotSaru::CLOCK_ALARM_1);
    //s_chronoDot.SetHour12(10, true, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.AlarmEnable(ChronoDotSaru::CLOCK_ALARM_1);

    // Sunset
    s_chronoDot.SetHour24(20, ChronoDotSaru::CLOCK_ALARM_2);
    s_chronoDot.SetMinute(50, ChronoDotSaru::CLOCK_ALARM_2);
    s_chronoDot.AlarmEnable(ChronoDotSaru::CLOCK_ALARM_2);

    secondLastTempUpdate = 0;
    secondsSinceTempUpdate = 0;

}
 
void loop () {

    if (mode == MODE_WAIT_FOR_ALARM) {
        SolidColor(strip0.Color(0x00, 0x00, 0x00), 0);
        s_chronoDot.UpdateCacheRange(ChronoDotSaru::REGISTER_SECONDS, ChronoDotSaru::REGISTER_YEAR);

        bool pmNotAm        = s_chronoDot.PmNotAm();
        bool twelveHourMode = s_chronoDot.TwelveHourMode();
        int  hours          = s_chronoDot.Hour();
        int  minutes        = s_chronoDot.Minute();
        int  seconds        = s_chronoDot.Second();

        /*
        Serial.print("  "); Serial.print(s_chronoDot.DayOfWeekName()); Serial.print(", "); Serial.print(s_chronoDot.MonthName());
            Serial.print(" "); Serial.print(s_chronoDot.DayOfMonth()); Serial.print(", "); Serial.println(s_chronoDot.Year());
        */
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

            // Blink debug LED to show that we're reading from the clock
            if (temp2 = !temp2)
                digitalWrite(DEBUG_LED_PIN, HIGH);
            else
                digitalWrite(DEBUG_LED_PIN, LOW);
        }

        /*
        uint8_t test;
        s_chronoDot.GetRegister(ChronoDotSaru::REGISTER_STATUS, &test, 1, true);
        Serial.print("Test register: "); Serial.println(test);
        //*/

        if (s_chronoDot.AlarmFlagStatus(ChronoDotSaru::CLOCK_ALARM_1, true)) {
            Serial.println("Alarm_1 !!");
            mode = MODE_ALARM;
        }
        else if (s_chronoDot.AlarmFlagStatus(ChronoDotSaru::CLOCK_ALARM_2, true)) {
            Serial.println("Alarm_2 !!");
            mode = MODE_ALARM;
        }
 
        inputLoop(1000);
    }
    else if (mode == MODE_ALARM) {
        Serial.println("Alarm mode");

        // Some example procedures showing how to display to the pixels:
        colorWipe(strip0.Color(255, 0, 0), 50); // Red
        if (mode != MODE_ALARM) return;
        colorWipe(strip0.Color(0, 255, 0), 50); // Green
        if (mode != MODE_ALARM) return;
        colorWipe(strip0.Color(0, 0, 255), 50); // Blue
        if (mode != MODE_ALARM) return;
        // Send a theater pixel chase in...
        theaterChase(strip0.Color(127, 127, 127), 50); // White
        if (mode != MODE_ALARM) return;
        theaterChase(strip0.Color(127,   0,   0), 50); // Red
        if (mode != MODE_ALARM) return;
        theaterChase(strip0.Color(  0,   0, 127), 50); // Blue
        if (mode != MODE_ALARM) return;

        rainbow(20);
        if (mode != MODE_ALARM) return;
        rainbowCycle(20);
        if (mode != MODE_ALARM) return;
        theaterChaseRainbow(50);
    }
    else {
        switch (temp) {
            case 0:
                LedCounter(1000);
                break;
            case 1:
                SolidColor(strip0.Color(255, 0, 0), 1000);
                break;
            default:
                rainbowCycle(60);
                break;
        }
    }
}

void handleEvent(EEvent event) {
    switch (event) {
        case EVENT_BUTTON_0_RELEASED: break; // Nothing cares

        // Default case, some other code handles the event.
        default:
            if (++eventIndex > EVENT_CAPACITY) {
                memset(events, EVENT_NONE, sizeof(events));
                eventIndex = 0;
            }
            events[eventIndex] = event;
            break;
    }
}

bool inputLoop(uint8_t waitMs) {
    unsigned waitMsTemp = waitMs;
    while (waitMsTemp) {
        delay(min(WAIT_CHUNK_MS, waitMsTemp));
        waitMsTemp -= min(WAIT_CHUNK_MS, waitMsTemp);

        EButtonState b0State = digitalRead(BUTTON0_PIN) == LOW ? BUTTON_STATE_DOWN : BUTTON_STATE_UP;
        if (b0State == pushButtons[0].state)
            continue;
        pushButtons[0].state = b0State;

        if (b0State == BUTTON_STATE_DOWN) {
            if (temp2 = !temp2)
                digitalWrite(DEBUG_LED_PIN, HIGH);
            else
                digitalWrite(DEBUG_LED_PIN, LOW);
            //handleEvent(pushButtons[0].pressedEvent);
            mode = EMode((mode + 1) % MODES);
            return true;
        }
        else {
            //handleEvent(pushButtons[0].releasedEvent);
        }

        /*
        if (events[eventIndex] == pushButtons[0].pressedEvent) {
            --eventIndex;
            mode = EMode((mode + 1) % MODES);
            return true;
        }
        */
    }

    return false;
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t waitMs) {
  for(uint16_t i=0; i<strip0.numPixels(); i++) {
      strip0.setPixelColor(i, c);
      strip1.setPixelColor(i, c);
      strip0.show();
      strip1.show();
      if (inputLoop(waitMs))
        return;
  }
}

void rainbow(uint8_t waitMs) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip0.numPixels(); i++) {
      strip0.setPixelColor(i, Wheel((i+j) & 255));
      strip1.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip0.show();
    strip1.show();
    if (inputLoop(waitMs))
        return;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t waitMs) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip0.numPixels(); i++) {
      strip0.setPixelColor(i, Wheel(((i * 256 / strip0.numPixels()) + j) & 255));
      strip1.setPixelColor(i, Wheel(((i * 256 / strip1.numPixels()) + j) & 255));
    }
    strip0.show();
    strip1.show();
    if (inputLoop(waitMs))
        return;
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t waitMs) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip0.numPixels(); i=i+3) {
        strip0.setPixelColor(i+q, c);    //turn every third pixel on
        strip1.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip0.show();
      strip1.show();
     
      if (inputLoop(waitMs))
          return;
     
      for (int i=0; i < strip0.numPixels(); i=i+3) {
        strip0.setPixelColor(i+q, 0);        //turn every third pixel off
        strip1.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t waitMs) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip0.numPixels(); i=i+3) {
          strip0.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
          strip1.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip0.show();
        strip1.show();
       
        if (inputLoop(waitMs))
            return;
       
        for (int i=0; i < strip0.numPixels(); i=i+3) {
          strip0.setPixelColor(i+q, 0);        //turn every third pixel off
          strip1.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip0.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip0.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip0.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// LED count measuring pattern
void LedCounter (uint8_t waitMs) {

    Serial.println("LED counting mode.");

    static uint32_t colorTable[] = {
        strip0.Color(0xFF, 0x00, 0x00),
        strip0.Color(0x00, 0xFF, 0x00),
        strip0.Color(0x00, 0x00, 0xFF),
        strip0.Color(0x7F, 0x7F, 0x00),
        strip0.Color(0x7F, 0x00, 0x7F),
        strip0.Color(0x00, 0x7F, 0x7F),
        strip0.Color(0xFF, 0x7F, 0x7F),
        strip0.Color(0x7F, 0xFF, 0x7F),
        strip0.Color(0x7F, 0x7F, 0xFF),
        strip0.Color(0x55, 0x55, 0x55),
    };
    static uint16_t colorTableCount = sizeof(colorTable)/sizeof(colorTable[0]);

    const uint16_t pixelCount = strip0.numPixels();
    for (uint16_t i = 0; i < pixelCount; ++i) {
        strip0.setPixelColor(i, colorTable[(i/10) % colorTableCount]);
        strip1.setPixelColor(i, colorTable[(i/10) % colorTableCount]);
    }

    strip0.show();
    strip1.show();
    inputLoop(waitMs);

}

// Solid color
void SolidColor (uint32_t color, uint8_t waitMs) {

    const uint16_t pixelCount = strip0.numPixels();
    for (uint16_t i = 0; i < pixelCount; ++i) {
        strip0.setPixelColor(i, color);
        strip1.setPixelColor(i, color);
    }

    strip0.show();
    strip1.show();
    inputLoop(waitMs);

}
