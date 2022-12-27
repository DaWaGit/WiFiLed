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
void LedStripe::vSetDistanceCalibrationActive(bool boNewMode) {
    boDistanceSensCalibActive = boNewMode;
}

//=============================================================================
bool LedStripe::boGetSwitchStatus() {
    return boNewSwitchMode;
}

//=============================================================================
void LedStripe::vSetColorMode(tColorMode enNewColorMode, uint8_t clientNumber){
    pEep->vSetColorMode((uint8_t)enNewColorMode, true); // store the new mode
    vSetColor(clientNumber); // switch to the current mode
}

//=============================================================================
void LedStripe::vSetColor(uint8_t clientNumber){
    if (boGetSwitchStatus()) {
        // when stripe is on
        switch ((tColorMode)pEep->u8ColorMode) {
            case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                break;
            case nRainbow: // draw a rainbow and shift/move the colors
                vSetRainbow(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                break;
            case nRandom: // change for each pixel color individually, but smooth
                vSetRandom(pEep->u8Saturation, pEep->u8Brightness, true, pEep->u8Speed);
                break;
            case nMovingPoint: // change for each pixel color individually, but smooth
                vSetMovingPoint(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, false);
                break;
            default:
                break;
        }
    }
    if (pWebServer && !boDistanceSensCalibActive)
        pWebServer->vSendStripeStatus(clientNumber, true); // update values for every web client
}

//=============================================================================
void LedStripe::vTurn( bool boNewMode, bool boFast) {

    if (!boNewMode) {
        // turn off, store the current values
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
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "fast ON" );
            switch ((tColorMode)pEep->u8ColorMode) {
                case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                    vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                    break;
                case nRainbow: // draw a rainbow and shift/move the colors
                    vSetRainbow(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                    break;
                case nRandom: // change for each pixel color individually, but smooth
                    vSetRandom(pEep->u8Saturation, pEep->u8Brightness, boInitRandomHue, pEep->u8Speed);
                    boInitRandomHue = false;
                    break;
                case nMovingPoint: // change for each pixel color individually, but smooth
                    vSetMovingPoint(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, false);
                    break;
                default:
                    break;
            }
        } else {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "fast OFF" );
            strip->Begin();
            strip->Show();
        }
    } else {
        // switch smooth
        boNewSwitchMode     = boNewMode;
        boCurrentSwitchMode = !boNewMode;
        if (boNewSwitchMode) {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "smooth ON" );
            cOnOffDamp->vInitDampVal(0);
            u8NewSwitchBrightness = pEep->u8Brightness;
        } else {
            vConsole( u8DebugLevel, DEBUG_LED_EVENTS, CLASS_NAME, __FUNCTION__, "smooth OFF" );
            cOnOffDamp->vInitDampVal(pEep->u8Brightness);
            u8NewSwitchBrightness = 0;
        }
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
    uint8_t u8NewBrightness,
    uint8_t u8Speed)
{
    // limit the brightness
    if (u8NewBrightness <= pEep->u8BrightnessMin) { u8NewBrightness = pEep->u8BrightnessMin; }
    if (u8NewBrightness >= pEep->u8BrightnessMax) { u8NewBrightness = pEep->u8BrightnessMax; }

    if (pEep->u8Speed) { // change the color also during on/off dimming
        u16NewHue += (uint16_t)pEep->u8Speed;
        pEep->u16Hue = u16NewHue;
    }

    RgbColor rgbGammaColor = colorGamma.Correct( // see: https://github.com/Makuna/NeoPixelBus/wiki/NeoGamma-object#rgbcolor-correctrgbcolor-original
        RgbColor(                                // see: https://github.com/Makuna/NeoPixelBus/wiki/RgbColor-object-API
            HsbColor(                            // see: https://github.com/Makuna/NeoPixelBus/wiki/HsbColor-object-API
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
    uint8_t u8Brightness,
    uint8_t u8Speed)
{
    // calculate saturation and brightness
    float fSaturation = (float)u8Saturation / (float)0xff;
    float fBrightness = (float)u8Brightness / (float)0xff;

    if (pEep->u8Speed) { // change the color also during on/off dimming
        u16StartHue += (uint16_t)pEep->u8Speed;
        pEep->u16Hue = u16StartHue;
    }

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
    bool boUpdateWebClients = false;

    if (!boDistanceSensCalibActive) {
        // when distance sensor calibration is not active
        if (boNewSwitchMode != boCurrentSwitchMode) {
            // strip will be turned on/off
            uint8_t u8DampedBrightness = (uint8_t)cOnOffDamp->fGetDampedVal(u8NewSwitchBrightness);
            //...................................................................
            // manage color mode
            switch ((tColorMode)pEep->u8ColorMode) {
                case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                    vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, u8DampedBrightness, pEep->u8Speed);
                    break;
                case nRainbow: // draw a rainbow and shift/move the colors
                    vSetRainbow(pEep->u16Hue, pEep->u8Saturation, u8DampedBrightness, pEep->u8Speed);
                    break;
                case nRandom: // change for each pixel color individually, but smooth
                    vSetRandom(pEep->u8Saturation, u8DampedBrightness, boInitRandomHue, pEep->u8Speed);
                    boInitRandomHue = false;
                    break;
                case nMovingPoint: // change for each pixel color individually, but smooth
                    vSetMovingPoint(pEep->u16Hue, pEep->u8Saturation, u8DampedBrightness, false);
                    break;
                default:
                    break;
            }
            if (boNewSwitchMode) {
                if ((u8NewSwitchBrightness - 1) <= u8DampedBrightness) {
                    boCurrentSwitchMode = boNewSwitchMode;
                }
            } else {
                if ((u8DampedBrightness + 1) <= pEep->u8BrightnessMin) {
                    boCurrentSwitchMode = boNewSwitchMode;
                    strip->Begin();
                    strip->Show();
                }
            }
            if (pEep->u8Speed) boUpdateWebClients = true;
        }
        else if (   (boCurrentSwitchMode || boNewSwitchMode)
                && pEep->u8Speed) {
            // strip is on and an animation speed is active
            //...................................................................
            // manage color mode
            switch ((tColorMode)pEep->u8ColorMode) {
                case nMonochrome: // use the same color of all pixels, but shift the color smoothly
                    vSetMonochrome(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                    boUpdateWebClients = true;
                    break;
                case nRainbow: // draw a rainbow and shift/move the colors
                    vSetRainbow(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, pEep->u8Speed);
                    boUpdateWebClients = true;
                    break;
                case nRandom: // change for each pixel color individually, but smooth
                    vSetRandom(pEep->u8Saturation, pEep->u8Brightness, false, pEep->u8Speed);
                    break;
                case nMovingPoint: // change for each pixel color individually, but smooth
                    delay((255 - pEep->u8Speed)<<1); // wait max 500ms
                    vSetMovingPoint(pEep->u16Hue, pEep->u8Saturation, pEep->u8Brightness, true);
                    break;
                default:
                    break;
            }
        }
        if (boUpdateWebClients && pWebServer) {
            pWebServer->vSendStripeStatus(-1, true); // update values for every web client
        }
    }
}
