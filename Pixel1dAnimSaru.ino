#include "Pixel1dAnimSaru.h"

/**********************************************************************************
*
*  Adafruit
*
**********************************************************************************/

//=============================================================================
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


/**********************************************************************************
*
*  CPatternRainbow
*
**********************************************************************************/

//=============================================================================
CPatternRainbow::CPatternRainbow () :
    CPattern()
{}

//=============================================================================
void CPatternRainbow::Update (uint16_t dtMs) {

    CPattern::Update(dtMs);
    /*
    timeMs += dtMs;
    timeThisFrameMs += dtMs;

    const uint16_t frameLast = frame;
    //frame = timeMs / frameDurationMs;
    //frame = timeMs / uint16_t(500);
    if (timeThisFrameMs >= s_frameDurationMs) {
        timeThisFrameMs -= s_frameDurationMs;
        ++frame;
    }

    if (frame != frameLast)
        colorsDirty = true;
    //*/
    if (!colorsDirty) // Could check frames pre-post parent Update instead.
        return;

    //frame %= 256; // Pattern only has 256 frames; repeats cleanly.
    if (frame > 255)
        frame = 0;

    for (uint16_t i = 0; i < longestStripCount; ++i) {
        uint32_t color = Wheel((i+frame) & 0xFF);
        for (uint8_t s = 0; s < stripCount; ++s) {
            if (i > strips[s].numPixels())
                continue;

            strips[s].setPixelColor(i, color);
        } // For each strip
    } // For each pixel

}


/**********************************************************************************
*
*  CPatternRainbowHued
*
**********************************************************************************/

//=============================================================================
CPatternRainbowHued::CPatternRainbowHued () :
    CPattern()
{}

//=============================================================================
void CPatternRainbowHued::Update (uint16_t dtMs) {

    CPattern::Update(dtMs);
    /*
    timeMs += dtMs;
    timeThisFrameMs += dtMs;

    const uint16_t frameLast = frame;
    //frame = timeMs / frameDurationMs;
    //frame = timeMs / uint16_t(500);
    if (timeThisFrameMs >= s_frameDurationMs) {
        timeThisFrameMs -= s_frameDurationMs;
        ++frame;
    }

    if (frame != frameLast)
        colorsDirty = true;
    //*/
    if (!colorsDirty) // Could check frames pre-post parent Update instead.
        return;

    //frame %= 256; // Pattern only has 256 frames; repeats cleanly.
    if (frame > 255)
        frame = 0;

    for (uint16_t i = 0; i < longestStripCount; ++i) {
        uint32_t color = Wheel((i+frame) & 0xFF);
        for (uint8_t s = 0; s < stripCount; ++s) {
            if (i > strips[s].numPixels())
                continue;

            strips[s].setPixelColor(i, color);
        } // For each strip
    } // For each pixel

}
