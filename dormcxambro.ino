#include <Wire.h>
#include <Adafruit_NeoPixel.h>
//#include "../../RTClib/RTClib.h"
#include "ChronoDotSaru.h"
#include "Pixel1dAnimSaru.h"

// Arduino
//#define STRIP0_PIN 6
//#define STRIP1_PIN 7
// Teensy
#define DEBUG_LED_PIN 13
#define STRIP0_PIN 11
#define STRIP1_PIN 12
#define BUTTON0_PIN 2
#define BUTTON1_PIN 3

#define STRIP0_PIXEL_COUNT 496
#define STRIP1_PIXEL_COUNT 465
#define MAX_STRIP_PIXELS (STRIP0_PIXEL_COUNT > STRIP1_PIXEL_COUNT ? STRIP0_PIXEL_COUNT : STRIP1_PIXEL_COUNT)

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
Adafruit_NeoPixel * strips[2]  = {0};
uint8_t             stripCount = sizeof(strips) / sizeof(strips[0]);

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
unsigned      secondLastTempUpdate   = 0;
unsigned      secondsSinceTempUpdate = 0;
EMode         mode                   = MODE_ALARM;
EPattern      s_pattern;//              = PATTERN_RAINBOW_WHITE;
unsigned      temp                   = 0;
unsigned      temp2                  = 0;

static unsigned long s_progLifetimeMs         =    0; // Prog lifetime (milliseconds)
static unsigned long s_progLifetimeScaledMs   =    0; // Offset from prog lifetime due to scaling (milliseconds)
static uint32_t      s_patternLifetimeMs      =    0; // Pattern lifetime (milliseconds)
static unsigned long s_lifetimeOffsetPerLoop  =    0; // 0 = no effect.  Adder to lifetimes each loop().
static uint16_t      s_patternFrameDurationMs;// = 1000;
static uint16_t      s_patternFrame           =    0;
static unsigned long s_patternTotalDurationMs = 5000; // Auto-switch patterns after this time.  Some patterns don't obey this (colorWipe).

static uint16_t      s_patternFrameDurationsMs[PATTERNS];
static unsigned long s_patternDurationsMs[PATTERNS];

static IPattern * s_patternNew = 0;

static IPattern *     s_patterns[2];
static const uint16_t s_patternsCount = sizeof(s_patterns) / sizeof(s_patterns[0]);

PushButton pushButtons[] = {
    { BUTTON0_PIN, BUTTON_STATE_UP, EVENT_BUTTON_0_PRESSED },
    { BUTTON1_PIN, BUTTON_STATE_UP, EVENT_BUTTON_1_PRESSED },
};
#define PUSH_BUTTON_COUNT (sizeof(pushButtons) / sizeof(pushButtons[0]))

EEvent        events[EVENT_CAPACITY];
unsigned      eventIndex = 0;

void handleEvent(EEvent event);
bool inputLoop(uint8_t waitMs);

// Pattern funcs
bool colorWipe (
    uint32_t color,
    uint16_t firstPixelIndex = 0,
    uint16_t lastPixelIndex  = MAX_STRIP_PIXELS
);
void theaterChase(
    uint32_t onColor,
    uint32_t offColor,
    uint16_t everyNthIsOn,
    uint16_t firstPixelIndex = 0,
    uint16_t lastPixelIndex  = MAX_STRIP_PIXELS
);
void rainbow (
    uint16_t firstPixelIndex = 0,
    uint16_t lastPixelIndex  = MAX_STRIP_PIXELS
);

//=============================================================================
void setup () {

    temp2 = 1;
    memset(events, 0, sizeof(events));

    pinMode(DEBUG_LED_PIN, OUTPUT);
    pinMode(BUTTON0_PIN, INPUT_PULLUP);
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    digitalWrite(DEBUG_LED_PIN, HIGH); // test

    strip0.begin();
    strip0.show(); // Initialize all pixels to 'off'
    strips[0] = &strip0;
    strip1.begin();
    strip1.show();
    strips[1] = &strip1;

    s_patterns[0] = new CPatternRainbow();
    s_patterns[0]->SetStrips(strips[0], stripCount);
    s_patterns[1] = new CPatternRainbowHued();
    s_patterns[1]->SetStrips(strips[0], stripCount);

    s_patternNew = s_patterns[0];
    s_patternNew->Prepare();

    s_patternFrameDurationsMs[PATTERN_WIPE_RED]            = 40;
    s_patternFrameDurationsMs[PATTERN_WIPE_GREEN]          = 16;
    s_patternFrameDurationsMs[PATTERN_WIPE_BLUE]           =  8;
    s_patternFrameDurationsMs[PATTERN_THEATER_WHITE]       = 16;
    s_patternFrameDurationsMs[PATTERN_THEATER_RED]         = 16;
    s_patternFrameDurationsMs[PATTERN_THEATER_BLUE_ORANGE] = 16;
    s_patternFrameDurationsMs[PATTERN_RAINBOW_WHITE]       = 20;
    s_patternFrameDurationsMs[PATTERN_RAINBOW_HUE_CYCLE]   = 20;

    s_patternDurationsMs[PATTERN_WIPE_RED]            =    0; // Ignored
    s_patternDurationsMs[PATTERN_WIPE_GREEN]          =    0; // Ignored
    s_patternDurationsMs[PATTERN_WIPE_BLUE]           =    0; // Ignored
    s_patternDurationsMs[PATTERN_THEATER_WHITE]       = 5000;
    s_patternDurationsMs[PATTERN_THEATER_RED]         = 5000;
    s_patternDurationsMs[PATTERN_THEATER_BLUE_ORANGE] = 5000;
    s_patternDurationsMs[PATTERN_RAINBOW_WHITE]       = 5000;
    s_patternDurationsMs[PATTERN_RAINBOW_HUE_CYCLE]   = 5000;

    s_pattern                = PATTERN_RAINBOW_WHITE;
    s_patternFrameDurationMs = s_patternFrameDurationsMs[s_pattern];

    Wire.begin();
    //Serial.begin(9600);

    s_chronoDot.Init();
    //s_chronoDot.SetTwelveHourMode(false);
    /*
    s_chronoDot.SetYear(2014);
    s_chronoDot.SetMonth(2);
    s_chronoDot.SetDayOfMonth(9);
    s_chronoDot.SetDayOfWeek(ChronoDotSaru::DAY_OF_WEEK_SUNDAY);
    //*/
    /*
    s_chronoDot.SetHour24(21);
    //s_chronoDot.SetHour12(9, true);
    s_chronoDot.SetMinute(11);
    s_chronoDot.SetSecond(30);
    //*/

    srandom((unsigned long)(s_chronoDot.Second()) << 8 | (unsigned long)(s_chronoDot.Second()));
    //mode = (random() % 2) ? MODE_WAIT_FOR_ALARM : MODE_ALARM;
    temp = random(3);

    // Sunrise
    s_chronoDot.SetHour24(7, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetMinute(28, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.SetSecond(0, ChronoDotSaru::CLOCK_ALARM_1);
    s_chronoDot.AlarmEnable(ChronoDotSaru::CLOCK_ALARM_1);

    // Sunset
    s_chronoDot.SetHour24(20, ChronoDotSaru::CLOCK_ALARM_2);
    s_chronoDot.SetMinute(50, ChronoDotSaru::CLOCK_ALARM_2);
    s_chronoDot.AlarmEnable(ChronoDotSaru::CLOCK_ALARM_2);

}

//=============================================================================
void loop () {

    // milliseconds delta from last loop
    const unsigned dtMs = unsigned(millis() - s_progLifetimeMs);
    // Wraps at about 50 days running time.
    s_progLifetimeMs       += dtMs;
    s_progLifetimeScaledMs += dtMs + s_lifetimeOffsetPerLoop;
    s_patternLifetimeMs    += dtMs + s_lifetimeOffsetPerLoop;

    if (s_patternFrameDurationMs > 1000)
        s_patternFrameDurationMs = 1000;
    else if (s_patternFrameDurationMs == 0)
        s_patternFrameDurationMs = 500;

    //const uint16_t lastFrame = s_patternFrame;
    //s_patternFrame           = uint16_t(s_patternLifetimeMs / s_patternFrameDurationMs);
    s_patternFrame           = uint16_t(s_patternLifetimeMs / 1000);
    //s_patternFrame           = uint16_t(s_patternLifetimeMs / foo->GetTime());

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

        // Hacky wait-for-human when pressing the button.
        if (mode != MODE_WAIT_FOR_ALARM)
            delay(150);
    }
    else if (mode == MODE_ALARM) {
        //Serial.println("Alarm mode");

        s_patternNew->Update(dtMs);

        //*s_patternNew->Frame() = s_patternFrame;

        static bool b = false;
        //if ((s_progLifetimeMs / 500) % 2 == 0)
        if (*s_patternNew->Frame() % 2 == 0)
        //if (s_patternFrame % 2 == 0)
        //if (s_pattern % 2 == 0)
        //if (b)
            digitalWrite(DEBUG_LED_PIN, HIGH);
        else
            digitalWrite(DEBUG_LED_PIN, LOW);
        b = !b;
        //delay(1);

        if (s_patternNew->IsColorsDirty())
            s_patternNew->Present();

        const bool buttonPressed = digitalRead(BUTTON0_PIN) == LOW;
        const IPattern * patternBefore = s_patternNew;
        if (buttonPressed || s_patternNew->IsDone()) {
            //s_patternNew = s_patterns[0] + ((s_patternNew - s_patterns[0]) + 1) % s_patternsCount;
            s_patternNew->Prepare();
            if (buttonPressed && s_patternNew == s_patterns[0])
                mode = MODE_WAIT_FOR_ALARM;
            /*
            ++s_patternNew;
            if (s_patternNew - s_patterns[0] >= s_patternsCount) {
                s_patternNew = s_patterns[0];
                if (buttonPressed)
                    mode = MODE_WAIT_FOR_ALARM;
            }
            s_patternNew->Prepare();
            */
        }

        if (buttonPressed) // Wait for the human.
            delay(150);
        else
            delayMicroseconds(500);

#if 0

        if (s_patternFrame == lastFrame)
            return;

        bool nextPattern = false;
        // "Render"
        if (s_patternDurationsMs[s_pattern] && s_patternLifetimeMs >= s_patternDurationsMs[s_pattern]) {
            nextPattern = true;
        }
        else {
            switch (s_pattern) {
                case PATTERN_WIPE_RED:
                    nextPattern = colorWipe(strip0.Color(255, 0, 0));
                    break;
                case PATTERN_WIPE_GREEN:
                    nextPattern = colorWipe(strip0.Color(0, 255, 0));
                    break;
                case PATTERN_WIPE_BLUE:
                    nextPattern = colorWipe(strip0.Color(0, 0, 255));
                    break;
                case PATTERN_THEATER_WHITE:
                    theaterChase(
                        strip0.Color(0x8F, 0x8F, 0x8F),
                        0,
                        3
                    );
                    break;
                case PATTERN_THEATER_RED:
                    theaterChase(
                        strip0.Color(0x7F, 0x00, 0x00),
                        0,
                        3
                    );
                    break;
                case PATTERN_THEATER_BLUE_ORANGE:
                    theaterChase(
                        strip0.Color(0xFF, 0x7F, 0x00),
                        strip0.Color(0x00, 0x00, 0x7F),
                        5
                    );
                    break;
                case PATTERN_RAINBOW_WHITE:
                    rainbow();
                    break;
                case PATTERN_RAINBOW_HUE_CYCLE:
                    //break;
                default:
                    nextPattern = true;
            }
        }
        // "Update"
        if (nextPattern) {
            s_pattern = EPattern((unsigned(s_pattern) + 1));
            if (s_pattern >= PATTERNS)
                s_pattern = EPattern(0);
            s_patternLifetimeMs = 0;
            s_patternFrame      = uint16_t(-1); // Force redraw on zeroth frame.
            s_patternFrameDurationMs = s_patternFrameDurationsMs[s_pattern];
        }

        // "Present"
        strip0.show();
        strip1.show();

        /*
        rainbow(20);
        if (mode != MODE_ALARM) return;
        rainbowCycle(20);
        if (mode != MODE_ALARM) return;
        theaterChaseRainbow(50);
        */
#endif
    }
    else {
        temp = 0;
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

//=============================================================================
// Fill the dots one after the other with a color.
// Returns true when pattern is done.
bool colorWipe (
    uint32_t      color,
    uint16_t      firstPixelIndex,
    uint16_t      lastPixelIndex
) {

    const uint16_t framesTotal = lastPixelIndex - firstPixelIndex;
    if (s_patternFrame >= framesTotal)
        return true;

    const uint16_t lastIndexNow = firstPixelIndex + s_patternFrame;
    for (uint16_t i = firstPixelIndex; i <= lastIndexNow; ++i) {
        if (i <= STRIP0_PIXEL_COUNT)
            strip0.setPixelColor(i, color);
        if (i <= STRIP1_PIXEL_COUNT)
            strip1.setPixelColor(i, color);
    }

    return false;

}

//=============================================================================
void rainbow (
    uint16_t firstPixelIndex,
    uint16_t lastPixelIndex
) {

    // Pattern has 256 frames, and can be repeated.
    const uint16_t f = s_patternFrame % 256;

    for (uint16_t i = firstPixelIndex; i < lastPixelIndex; ++i) {
        if (i <= STRIP0_PIXEL_COUNT)
            strip0.setPixelColor(i, Wheel((i+f) & 0xFF));
        if (i <= STRIP1_PIXEL_COUNT)
            strip1.setPixelColor(i, Wheel((i+f) & 0xFF));
    }

}

// Slightly different, this makes the rainbow lean towards one color at a time
void rainbowCycle (uint8_t waitMs) {
    uint16_t i, j;

    for (j = 0; j < 256*5; ++j) { // 5 cycles of all colors on wheel
        for (i = 0; i < STRIP0_PIXEL_COUNT; ++i)
            strip0.setPixelColor(i, Wheel(((i * 256 / strip0.numPixels()) + j) & 255));
        for (i = 0; i < STRIP1_PIXEL_COUNT; ++i)
            strip1.setPixelColor(i, Wheel(((i * 256 / strip1.numPixels()) + j) & 255));
        strip0.show();
        strip1.show();
        if (inputLoop(waitMs))
            return;
    }
}

//=============================================================================
//Theatre-style crawling lights.
void theaterChase (
    uint32_t onColor,
    uint32_t offColor,
    uint16_t everyNthIsOn,
    uint16_t firstPixelIndex,
    uint16_t lastPixelIndex
) {

    for (uint16_t i = firstPixelIndex; i <= lastPixelIndex; ++i) {
        if ((i + s_patternFrame) % everyNthIsOn == 0) {
            if (i < STRIP0_PIXEL_COUNT)
                strip0.setPixelColor(i, onColor);
            if (i < STRIP1_PIXEL_COUNT)
                strip1.setPixelColor(i, onColor);
        }
        else {
            if (i < STRIP0_PIXEL_COUNT)
                strip0.setPixelColor(i, offColor);
            if (i < STRIP1_PIXEL_COUNT)
                strip1.setPixelColor(i, offColor);
        }
    }

}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t waitMs) {
  for (int j = 0; j < 256; ++j) {     // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; ++q) {
        for (int i = 0; i < STRIP0_PIXEL_COUNT; i += 3)
          strip0.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        for (int i = 0; i < STRIP1_PIXEL_COUNT; i += 3)
          strip1.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        strip0.show();
        strip1.show();
       
        if (inputLoop(waitMs))
            return;
       
        for (int i = 0; i < STRIP0_PIXEL_COUNT; i += 3)
          strip0.setPixelColor(i+q, 0);        //turn every third pixel off
        for (int i = 0; i < STRIP1_PIXEL_COUNT; i += 3)
          strip1.setPixelColor(i+q, 0);        //turn every third pixel off
    }
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

    const uint16_t pixelCount0 = strip0.numPixels();
    for (uint16_t i = 0; i < pixelCount0; ++i)
        strip0.setPixelColor(i, colorTable[(i/10) % colorTableCount]);
    const uint16_t pixelCount1 = strip1.numPixels();
    for (uint16_t i = 0; i < pixelCount1; ++i)
        strip1.setPixelColor(i, colorTable[(i/10) % colorTableCount]);

    strip0.show();
    strip1.show();
    inputLoop(waitMs);

}

// Solid color
void SolidColor (uint32_t color, uint8_t waitMs) {

    for (uint16_t i = 0; i < STRIP0_PIXEL_COUNT; ++i)
        strip0.setPixelColor(i, color);
    for (uint16_t i = 0; i < STRIP1_PIXEL_COUNT; ++i)
        strip1.setPixelColor(i, color);

    strip0.show();
    strip1.show();
    inputLoop(waitMs);

}
