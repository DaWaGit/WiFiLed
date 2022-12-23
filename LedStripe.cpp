#include "LedStripe.h"
#include "Utils.h"
#include "DebugLevel.h"

#define FLUSH_DELAY       5 // delay in ms after led tripe update
#define DATA_PIN          D1
#define CLASS_NAME        "LedStripe"

//=============================================================================
LedStripe::LedStripe(uint8_t u8NewDebugLevel) {
    u8DebugLevel = u8NewDebugLevel;
}

//=============================================================================
void LedStripe::vInit(class Eep *pNewEep) {
    pEep = pNewEep;
    // Declare our NeoPixel strip object:
    stripe = new Adafruit_NeoPixel(pEep->u16LedCount, DATA_PIN, NEO_GRB + NEO_KHZ800);
    // Argument 1 = Number of pixels in NeoPixel strip
    // Argument 2 = Arduino pin number (most are valid)
    // Argument 3 = Pixel type flags, add together as needed:
    //              NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LED's)
    //              NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //              NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //              NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    //              NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
    stripe->begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    stripe->clear();
    stripe->show(); // flush
    delay(FLUSH_DELAY);

    if (u8DebugLevel & DEBUG_LED_EVENTS) {
        char buffer[100];
        sprintf(buffer, " LedCount:%d", pEep->u16LedCount);
        sprintf(buffer, "%s Hue:0x%04x", buffer, pEep->u16Hue);
        sprintf(buffer, "%s Sat:0x%02x", buffer, pEep->u8Saturation);
        sprintf(buffer, "%s Bri:0x%02x", buffer, pEep->u8Brightness);
        vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }

}

//=============================================================================
void LedStripe::vSetWebServer(class WebServer *pNewWebServer) {
    pWebServer = pNewWebServer;
}

//=============================================================================
bool LedStripe::boGetSwitchStatus() {
    return boNewSwitchMode;
}

//=============================================================================
uint16_t LedStripe::u16GetHue(){
    return pEep->u16Hue;
}

//=============================================================================
uint8_t LedStripe::u8GetSaturation(){
    return pEep->u8Saturation;
}

//=============================================================================
uint8_t LedStripe::u8GetBrightness(){
    return pEep->u8Brightness;
}

//=============================================================================
void LedStripe::vSetColorMode(tColorMode enNewColorMode){
    if ((uint8_t)enNewColorMode != pEep->u8ColorMode) {
        pEep->vSetColorMode((uint8_t)enNewColorMode, true);
        if (boGetSwitchStatus()) {
            // when stripe is on and animation speed us zero
            // switch the current mode
            if ((tColorMode)pEep->u8ColorMode == nMonochrome) {
                vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness);
            } else if ((tColorMode)pEep->u8ColorMode == nRainbow) {
                vSetRainbow(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness);
            } else if ((tColorMode)pEep->u8ColorMode == nRandom) {
                vSetRandom(pEep->u8Saturation, pEep->u8Brightness, true, pEep->u8Speed);
            } else {
                vSetMovingPoint(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, false);
            }
        }
        if (pWebServer)
            pWebServer->vSendStripeStatus(-1, true); // update values for every client
    }
}

//=============================================================================
void LedStripe::vTurn( bool boNewMode, bool boFast) {

    if (!boNewMode) {
        // turn off
        pEep->vSetHue(pEep->u16Hue, true);
        pEep->vSetSaturation(pEep->u8Saturation, true);
        pEep->vSetBrightness(pEep->u8Brightness, true);
    } else {
        boInitRandomHue = true;
    }

    if (boFast) {
        // switch fast
        boNewSwitchMode     = boNewMode;
        boCurrentSwitchMode = boNewMode;
        if (boNewMode) {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "LedStripe : fast ON" );
            vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness);
        } else {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "LedStripe : fast OFF" );
            stripe->clear(); // turn all off
            stripe->show();  // flush
            delay(FLUSH_DELAY);
        }
    } else {
        // switch smooth
        boNewSwitchMode     = boNewMode;
        boCurrentSwitchMode = !boNewMode;
        if (boNewSwitchMode) {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "LedStripe : smooth ON" );
            cOnOffDamp->vInitDampVal(0);
            u8NewSwitchBrightness = pEep->u8Brightness;
        } else {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "LedStripe : smooth OFF" );
            cOnOffDamp->vInitDampVal(pEep->u8Brightness);
            u8NewSwitchBrightness = 0;
        }
    }
    if (pWebServer && !boNewMode) {
        // webserver defined and off event
        pWebServer->vSendStripeStatus(-1, true); // update values for every client
    }
}

//=============================================================================
void LedStripe::vSetValues(
    uint16_t u16NewHue,
    uint8_t u8NewSaturation,
    uint8_t u8NewBrightness)
{
    pEep->u16Hue = u16NewHue;
    pEep->u8Saturation = u8NewSaturation;
    pEep->u8Brightness = u8NewBrightness;
}

//=============================================================================
void LedStripe::vSetMonochrome(
    uint16_t u16NewHue,
    uint8_t u8NewSaturation,
    uint8_t u8NewBrightness)
{
    // limit the brightness
    if (u8NewBrightness <= pEep->u8BrightnessMin) { u8NewBrightness = pEep->u8BrightnessMin; }
    if (u8NewBrightness >= pEep->u8BrightnessMax) { u8NewBrightness = pEep->u8BrightnessMax; }

    uint32_t u32RgbColor = stripe->gamma32(stripe->ColorHSV(u16NewHue, u8NewSaturation, u8NewBrightness)); // get rgb color
    if (boNewSwitchMode || boCurrentSwitchMode) {
        // update stripe when it is turned on
        stripe->fill(u32RgbColor, 0, pEep->u16LedCount); // set all
        stripe->show();                                  // flush
        delay(FLUSH_DELAY);
    }

    if (u8DebugLevel & DEBUG_LED_DETAILS) {
        char buffer[100];
        sprintf(buffer, " Hue:0x%04x", u16NewHue);
        sprintf(buffer, "%s Sat:0x%02x", buffer, u8NewSaturation);
        sprintf(buffer, "%s Bri:0x%02x", buffer, u8NewBrightness);
        sprintf(buffer, "%s RGB:0x%06x", buffer, u32RgbColor);
        vConsole(u8DebugLevel, DEBUG_LED_DETAILS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=============================================================================
void LedStripe::vSetRainbow(
    long lStartHue,
    uint8_t u8Saturation,
    uint8_t u8Brightness )
{ // see: https://learn.adafruit.com/black-lives-matter-badge/arduino-neopixel-rainbow
    for(int iLedCnt=0; iLedCnt < pEep->u16LedCount; iLedCnt++) {
        int pixelHue = lStartHue + (iLedCnt * 65536L / pEep->u16LedCount);
        stripe->setPixelColor(iLedCnt, stripe->gamma32(stripe->ColorHSV(pixelHue, u8Saturation, u8Brightness)));
    }
    stripe->show();
    delay(FLUSH_DELAY);
}

//=============================================================================
void LedStripe::vSetRandom(
    uint8_t u8Saturation,
    uint8_t u8Brightness,
    bool boInitNewColors,
    uint8_t u8Speed)
{
    static uint16_t aHue[300];

    for(int iLedCnt=0; iLedCnt < pEep->u16LedCount; iLedCnt++) {
        if (boInitNewColors) {
            aHue[iLedCnt] = random(0x0000, 0xffff);
        } else if (u8Speed) {
            if (iLedCnt & 0x0001) {
                aHue[iLedCnt] += u8Speed;
            } else {
                aHue[iLedCnt] -= u8Speed;
            }
        }
        stripe->setPixelColor(iLedCnt, stripe->gamma32(stripe->ColorHSV(aHue[iLedCnt], u8Saturation, u8Brightness)));
    }
    stripe->show();
    delay(FLUSH_DELAY);
}

//=============================================================================
void LedStripe::vSetMovingPoint(
    uint16_t u16NewHue,
    uint8_t u8Saturation,
    uint8_t u8Brightness,
    bool boMove)
{
    static int iPos         = 0;
    static bool boDirection = false;

    if (boMove) {
        if (boDirection) {
            if (++iPos >= pEep->u16LedCount) {
                iPos -= 2;
                boDirection = !boDirection;
            }
        } else {
            if (iPos > 0) {
                --iPos;
            } else {
                ++iPos;
                boDirection = !boDirection;
            }
        }
    }
    for (int iLedCnt = 0; iLedCnt < pEep->u16LedCount; iLedCnt++) {
        stripe->setPixelColor(iLedCnt, stripe->gamma32(stripe->ColorHSV(u16NewHue, u8Saturation, iPos == iLedCnt ? u8Brightness : 0)));
    }
    stripe->show();
    delay(FLUSH_DELAY);
}

//=============================================================================
void LedStripe::vLoop() {
    static uint8_t u8DampedBrightness = 0;
    static uint16_t u16SendCnt        = 0;

    uint16_t u16Hue      = pEep->u16Hue;
    uint8_t u8Saturation = pEep->u8Saturation;
    uint8_t u8Brightness = pEep->u8Brightness;

    if (boNewSwitchMode != boCurrentSwitchMode) {
        u8DampedBrightness = (uint8_t)cOnOffDamp->fGetDampedVal(u8NewSwitchBrightness);
        u8Brightness       = u8DampedBrightness;

        if ((tColorMode)pEep->u8ColorMode == nMonochrome) {
            vSetMonochrome(u16Hue, u8Saturation, u8Brightness);
        } else if ((tColorMode)pEep->u8ColorMode == nRainbow) {
            vSetRainbow(pEep->u16Hue, u8Saturation, u8Brightness);
        } else if ((tColorMode)pEep->u8ColorMode == nRandom) {
            vSetRandom(u8Saturation, u8Brightness, boInitRandomHue, pEep->u8Speed);
            boInitRandomHue = false;
        } else {
            vSetMovingPoint(u16Hue, u8Saturation, u8Brightness, false);
        }
        if (boNewSwitchMode) {
            if ((u8NewSwitchBrightness - 1) <= u8DampedBrightness) {
                u8Brightness        = u8NewSwitchBrightness;
                boCurrentSwitchMode = boNewSwitchMode;
            }
        } else {
            if ((u8DampedBrightness - 1) <= pEep->u8BrightnessMin) {
                boCurrentSwitchMode = boNewSwitchMode;
                stripe->clear(); // turn all off
                stripe->show();  // flush
                delay(FLUSH_DELAY);
            }
        }
    } else if (   (boCurrentSwitchMode || boNewSwitchMode)
               && pEep->u8Speed) {
        // is on and an animation speed is active
        //...................................................................
        // manage color mode
        switch ((tColorMode)pEep->u8ColorMode) {
            case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                pEep->u16Hue   += (uint16_t)pEep->u8Speed;
                u16Hue         = pEep->u16Hue;
                vSetMonochrome(u16Hue, u8Saturation, u8Brightness);
                if (++u16SendCnt > 25) {
                    // PATCH: Do not send each change, because web socket need time to send all.
                    u16SendCnt = 0;
                    if (pWebServer)
                        pWebServer->vSendStripeStatus(-1, true); // update values for every client
                }
                break;
            case nRainbow: // draw a rainbow and shift/move the colors
                pEep->u16Hue += (uint16_t)pEep->u8Speed;
                vSetRainbow(pEep->u16Hue, u8Saturation, u8Brightness);
                if (++u16SendCnt > 25) {
                    // PATCH: Do not send each change, because web socket need time to send all.
                    u16SendCnt = 0;
                    if (pWebServer)
                        pWebServer->vSendStripeStatus(-1, true); // update values for every client
                }
                break;
            case nRandom: // change for each pixel color individually, but smooth
                vSetRandom(u8Saturation, u8Brightness, false, pEep->u8Speed);
                break;
            case nMovingPoint: // change for each pixel color individually, but smooth
                if (pEep->u8Speed) {
                    delay((255 - pEep->u8Speed)<<1); // wait max 500ms
                    vSetMovingPoint(u16Hue, u8Saturation, u8Brightness, true);
                }
                break;
            default:
                break;
        }
    }
}
