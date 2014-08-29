#pragma once
#include <Adafruit_NeoPixel.h>

/**********************************************************************************
*
*  Adafruit
*
**********************************************************************************/

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos);



/**********************************************************************************
*
*  Patterns
*
**********************************************************************************/

//=============================================================================
enum EPattern {
    PATTERN_WIPE_RED = 0,
    PATTERN_WIPE_GREEN,
    PATTERN_WIPE_BLUE,
    PATTERN_THEATER_WHITE,
    PATTERN_THEATER_RED,
    PATTERN_THEATER_BLUE_ORANGE,
    PATTERN_RAINBOW_WHITE,
    PATTERN_RAINBOW_HUE_CYCLE,
    PATTERNS
};

//=============================================================================
class IPattern {

public:
    // == Accessors =================================================
    virtual uint16_t   FrameDurationMs () = 0;
    virtual uint16_t * Frame () = 0;
    virtual EPattern   GetPattern () const = 0;
    virtual bool       IsColorsDirty () const = 0;
    virtual bool       IsDone () const = 0;

    // == Methods ===================================================
    virtual void SetStrips (Adafruit_NeoPixel * strips, uint8_t count) = 0;

    virtual void Prepare () = 0;
    virtual void Update (uint16_t dtMs) = 0;
    virtual void Present () = 0;

};

//=============================================================================
template <EPattern T_Pattern, uint16_t T_FrameDurationMs = 16>
class CPattern : public IPattern {

public:
    // == Data ======================================================
    static const EPattern s_pattern         = T_Pattern;
    static const uint16_t s_frameDurationMs = T_FrameDurationMs;

protected:
    // == Data ======================================================
    Adafruit_NeoPixel * strips;
    uint8_t             stripCount;
    uint16_t            longestStripCount;

    uint16_t frame;
    uint16_t timeMs;
    uint16_t timeThisFrameMs;

    bool colorsDirty;


public:
    CPattern () :
        strips(0),
        stripCount(0),
        longestStripCount(0),
        frame(0),
        timeMs(0),
        timeThisFrameMs(0),
        colorsDirty(true)
    {}

    // == Accessors =================================================
    virtual uint16_t   FrameDurationMs ()     { return T_FrameDurationMs; }
    virtual uint16_t * Frame ()               { return &frame; }
    virtual EPattern   GetPattern () const    { return T_Pattern; }
    virtual bool       IsColorsDirty () const { return colorsDirty; }

    // == Methods ===================================================
    virtual void SetStrips (Adafruit_NeoPixel * strips, uint8_t count) {
        this->strips = strips;
        stripCount   = count;

        for (uint8_t i = 0; i < count; ++i) {
            if (strips[i].numPixels() > longestStripCount)
                longestStripCount = strips[i].numPixels();
        }
    }

    virtual void Prepare () { timeMs = frame = 0; colorsDirty = true; }

    virtual void Update (uint16_t dtMs) {
        timeMs += dtMs;
        timeThisFrameMs += dtMs;

        uint16_t frameLast = frame;
        while (timeThisFrameMs >= T_FrameDurationMs) {
        //if (timeThisFrameMs >= T_FrameDurationMs) {
            timeThisFrameMs -= T_FrameDurationMs;
            ++frame;
        }

        if (frame != frameLast)
            colorsDirty = true;
    }

    virtual void Present () {
        colorsDirty = false;
        for (uint8_t i = 0; i < stripCount; ++i)
            strips[i].show();
    }

};

//=============================================================================
class CPatternRainbow : public CPattern<PATTERN_RAINBOW_WHITE, 20> {

public:
    // == Accessors =================================================
    virtual bool IsDone () const { return timeMs > 15000; }

    // == Methods ===================================================
    CPatternRainbow ();

    virtual void Update (uint16_t dtMs);

};

//=============================================================================
class CPatternRainbowHued : public CPattern<PATTERN_RAINBOW_HUE_CYCLE, 20> {

public:
    // == Accessors =================================================
    virtual bool IsDone () const { return timeMs > 15000; }

    // == Methods ===================================================
    CPatternRainbowHued ();

    virtual void Update (uint16_t dtMs);

};













#if 0

// Tweening code from/inspired-by https://code.google.com/p/cpptweener/


namespace Pixel1dAnimSaru {

/**********************************************************************************
*
*  Math Functions
*
**********************************************************************************/

template <typename T>
T LinearEase (T t, T b, T c, T d) {
    return c * t / d + b;
}


/**********************************************************************************
*
*  Types
*
**********************************************************************************/

//=============================================================================
enum ELimitBehavior {
    LIMIT_BEHAVIOR_CAP = 0,
    LIMIT_BEHAVIOR_WRAP,
    LIMIT_BEHAVIOR_BOUNCE
};

//=============================================================================
enum EEase {
    EASE_IN = 0,
    EASE_OUT,
    EASE_IN_OUT,
};

//=============================================================================
enum ETweenType {
    TWEEN_TYPE_LINEAR = 0,
    TWEEN_TYPE_CUBIC,
    TWEEN_TYPE_BOUNCE,
    TWEEN_TYPE_ELASTIC,
};

//=============================================================================
struct PixelIndex {
    uint16_t index;
    uint16_t minIndex;
    uint16_t maxIndex;
};

//=============================================================================
struct Segment {
    unsigned indexStart;
    unsigned indexEnd;
    uint32_t color;
};

//=============================================================================
struct Particle {
    unsigned index;
    uint32_t color;
    uint8_t  velocity;
};

//=============================================================================
template <typename T_TargetType>
struct TweenTarget {
    T_TargetType *              target;
    T_TargetType                initialValue;
    T_TargetType                finalValue;

    TweenTarget<T_TargetType> * next;

    TweenTarget (T_TargetType * target, T_TargetType initialValue, T_TargetType finalValue
    ) :
        target(target), initialValue(initialValue), finalValue(finalValue)
    {}
};

//=============================================================================
template <typename T>
struct TweenParam {
    TweenTarget<T_TargetType> * targets;
    uint8_t                     targetCount;
    unsigned long               millisElapsed;
    unsigned long               millisDelay;
    EEase                       ease;
    ETweenType                  tween;
    bool                        repeat;

    TweenParam (EEase ease, ETweenType tween, bool repeat
    ) :
        targets(0), targetCount(0),
        millisElapsed(0), millisDelay(0),
        ease(ease), tween(tween), repeat(repeat)
    {}
};

//=============================================================================
template <typename T_TargetType>
struct Tweener {
    typedef TweenParam<T_TargetType> Param;
    Param *        params;
    uint8_t        paramCount;
    unsigned long  millisLastStep;
    T_TargetType   minValue;
    T_TargetType   maxValue;
    ELimitBehavior limitBehavior;
    ETweenType     tweenType;

    Tweener (
        T_TargetType minValue, T_TargetType maxValue, ELimitBehavior limitBehavior, ETweenType tweenType
    ) : 
        targets(0),
        targetCount(0),
        minValue(minValue),
        maxValue(maxValue),
        limitBehavior(limitBehavior),
        tweenType(tweenType)
    {}

    bool IsComplete () const;

    void Step (unsigned millisElapsed) {

        switch (tweenType) {
            case TWEEN_TYPE_LINEAR: {
                Param * paramIt = params;
                while (paramIt) {
                    paramIt = paramIt->
                }
            } break;
        }

    }
};


/**********************************************************************************
*
*  Data
*
**********************************************************************************/


/**********************************************************************************
*
*  Implementations
*
**********************************************************************************/

//=============================================================================
struct PatternParticleTest;

//=============================================================================
struct LedAnimSaru {

    //=============================================================================
    // Constants and types
    static unsigned const s_maxNeoPixelStrips = 8;

    //=============================================================================
    // Data
    Adafruit_NeoPixel * neoPixelStrips[s_maxNeoPixelStrips];

    //=============================================================================
    // Construction

    LedAnimSaru ();
    ~LedAnimSaru ();

    //=============================================================================
    // Methods

    void AddNpStrip (Adafruit_NeoPixel * strip);
    void  ();

};

} // namespace Pixel1dAnimSaru 
#endif