#include "LedStripe.h"
#include "Utils.h"
#include "DebugLevel.h"

#define CLASS_NAME "LedStripe"

//=============================================================================
LedStripe::LedStripe(uint8_t u8NewDebugLevel) {
    u8DebugLevel = u8NewDebugLevel;
}

//=============================================================================
void LedStripe::vInit(class Eep *pNewEep) {
    pEep = pNewEep;

    // For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
    strip = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>(pEep->u16LedCount);
    // this resets all the neopixels to an off state
    strip->Begin(); // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-begin
    strip->Show();  // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-showbool-maintainbufferconsistency--true

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
void LedStripe::vSetColorMode(tColorMode enNewColorMode, uint8_t clientNumber){
    //if ((uint8_t)enNewColorMode != pEep->u8ColorMode) {
        pEep->vSetColorMode((uint8_t)enNewColorMode, true);
        // switch the current mode
        vSetColor(clientNumber);
        //}
}

//=============================================================================
void LedStripe::vSetColor(uint8_t clientNumber){
    if (boGetSwitchStatus()) {
        // when stripe is on and animation speed is zero
        // switch the current mode
        // dependent from current mode, enable the LEDs
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
        pWebServer->vSendStripeStatus(clientNumber, true); // update values for every client
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
            strip->Begin();
            strip->Show();
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

    RgbColor rgbGammaColor = colorGamma.Correct( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoGamma-object#rgbcolor-correctrgbcolor-original
        RgbColor(                           // see: https://github.com/Makuna/NeoPixelBus/wiki/RgbColor-object-API
            HsbColor(                       // see: https://github.com/Makuna/NeoPixelBus/wiki/HsbColor-object-API
                (float)u16NewHue / (float)0xffff,
                (float)u8NewSaturation / (float)0xff,
                (float)u8NewBrightness / (float)0xff)));

    strip->ClearTo(rgbGammaColor); // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-cleartocolorobject-color
    strip->Show();

    if (u8DebugLevel & DEBUG_LED_DETAILS) {
        char buffer[100];
        sprintf(buffer, " Hue:0x%04x", u16NewHue);
        sprintf(buffer, "%s Sat:0x%02x", buffer, u8NewSaturation);
        sprintf(buffer, "%s Bri:0x%02x", buffer, u8NewBrightness);
        sprintf(buffer, "%s RGB:0x%02x%02x%02x", buffer, rgbGammaColor.R, rgbGammaColor.G, rgbGammaColor.B);
        vConsole(u8DebugLevel, DEBUG_LED_DETAILS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=============================================================================
void LedStripe::vSetRainbow(
    uint16_t u16StartHue,
    uint8_t u8Saturation,
    uint8_t u8Brightness)
{
    float fSaturation = (float)u8Saturation / (float)0xff;
    float fBrightness = (float)u8Brightness / (float)0xff;
    for (uint16_t u16LedIdx = 0; u16LedIdx < pEep->u16LedCount; u16LedIdx++) {
        // loop over all pixels
        uint16_t u16PixelHue = u16StartHue + (u16LedIdx * 65536L / pEep->u16LedCount);
        strip->SetPixelColor( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-setpixelcoloruint16_t-indexpixel-colorobject-color
            u16LedIdx,
            colorGamma.Correct( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoGamma-object#rgbcolor-correctrgbcolor-original
                RgbColor(       // see: https://github.com/Makuna/NeoPixelBus/wiki/RgbColor-object-API
                    HsbColor(   // see: https://github.com/Makuna/NeoPixelBus/wiki/HsbColor-object-API
                        (float)u16PixelHue / (float)0xffff,
                        fSaturation,
                        fBrightness
                    )
                )
            )
        );
    }
    strip->Show();
}

//=============================================================================
void LedStripe::vSetRandom(
    uint8_t u8Saturation,
    uint8_t u8Brightness,
    bool boInitNewColors,
    uint8_t u8Speed)
{
    static uint16_t aHue[300];
    float fSaturation = (float)u8Saturation / (float)0xff;
    float fBrightness = (float)u8Brightness / (float)0xff;

    for(uint16_t u16LedIdx=0; u16LedIdx < pEep->u16LedCount; u16LedIdx++) {
        if (boInitNewColors) {
            aHue[u16LedIdx] = random(0x0000, 0xffff);
        } else if (u8Speed) {
            if (u16LedIdx & 0x0001) {
                aHue[u16LedIdx] += u8Speed;
            } else {
                aHue[u16LedIdx] -= u8Speed;
            }
        }
        strip->SetPixelColor( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-setpixelcoloruint16_t-indexpixel-colorobject-color
            u16LedIdx,
            colorGamma.Correct( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoGamma-object#rgbcolor-correctrgbcolor-original
                RgbColor(       // see: https://github.com/Makuna/NeoPixelBus/wiki/RgbColor-object-API
                    HsbColor(   // see: https://github.com/Makuna/NeoPixelBus/wiki/HsbColor-object-API
                        (float)aHue[u16LedIdx] / (float)0xffff,
                        fSaturation,
                        fBrightness
                    )
                )
            )
        );
    }
    strip->Show();
}

//=============================================================================
void LedStripe::vSetMovingPoint(
    uint16_t u16NewHue,
    uint8_t u8Saturation,
    uint8_t u8Brightness,
    bool boMove)
{
    static uint16_t u16Pos    = 0;
    static bool boDirection = false;
    float fHue              = (float)u16NewHue / (float)0xffff;
    float fSaturation       = (float)u8Saturation / (float)0xff;
    float fBrightness       = (float)u8Brightness / (float)0xff;

    if (boMove) {
        if (boDirection) {
            if (++u16Pos >= pEep->u16LedCount) {
                u16Pos -= 2;
                boDirection = !boDirection;
            }
        } else {
            if (u16Pos > 0) {
                --u16Pos;
            } else {
                ++u16Pos;
                boDirection = !boDirection;
            }
        }
    }
    for (uint16_t u16LedIdx = 0; u16LedIdx < pEep->u16LedCount; u16LedIdx++) {
        strip->SetPixelColor( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API#void-setpixelcoloruint16_t-indexpixel-colorobject-color
            u16LedIdx,
            colorGamma.Correct( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoGamma-object#rgbcolor-correctrgbcolor-original
                RgbColor(       // see: https://github.com/Makuna/NeoPixelBus/wiki/RgbColor-object-API
                    HsbColor(   // see: https://github.com/Makuna/NeoPixelBus/wiki/HsbColor-object-API
                        fHue,
                        fSaturation,
                        u16Pos == u16LedIdx ? fBrightness : 0
                    )
                )
            )
        );
    }
    strip->Show();
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
                strip->Begin();
                strip->Show();
            }
        }
    } else if (   (boCurrentSwitchMode || boNewSwitchMode)
               && pEep->u8Speed) {
        // is on and an animation speed is active
        //...................................................................
        // manage color mode
        switch ((tColorMode)pEep->u8ColorMode) {
            case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                if (pEep->u8Speed) {
                    pEep->u16Hue += (uint16_t)pEep->u8Speed;
                    vSetMonochrome(pEep->u16Hue, u8Saturation, u8Brightness);
                    if (pWebServer)
                        pWebServer->vSendStripeStatus(-1, true); // update values for every client
                }
                break;
            case nRainbow: // draw a rainbow and shift/move the colors
                if (pEep->u8Speed) {
                    delay((255 - pEep->u8Speed) << 1); // wait max 500ms
                    strip->RotateRight(1);
                    strip->Show();
                    HsbColor hsbColor = HsbColor(strip->GetPixelColor(0)); // get color from the first pixel
                    pEep->u16Hue = (uint16_t)((float)hsbColor.H * (float)0xffff); // get hue from the first pixel
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
