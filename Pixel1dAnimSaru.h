#pragma once
#if 0
#include <Adafruit_NeoPixel.h>

// Tweener from/inspired-by https://code.google.com/p/cpptweener/


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