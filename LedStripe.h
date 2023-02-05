#ifndef LedStripe_h
#define LedStripe_h
#include "PT1.h"
#include "Eep.h"
#include "WebServer.h"
#include "NtpTime.h"
#include <Arduino.h>     // see: https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
#include <NeoPixelBus.h> // see: https://github.com/Makuna/NeoPixelBus
                         //      https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

enum tColorMode {
    nMonochrome = 0,
    nRainbow,
    nRandom,
    nMovingPoint,
    nNoMode
};

class LedStripe {
    public:
        LedStripe(uint8_t);
        void vInit(class Eep *, class NtpTime *);
        void vTurn(bool, bool);
        void vSetMonochrome(uint16_t, uint8_t, uint8_t, uint8_t);
        void vSetRainbow(uint16_t, uint8_t, uint8_t, uint8_t);
        void vSetRandom(uint8_t, uint8_t, bool, uint8_t);
        void vSetMovingPoint(uint16_t, uint8_t, uint8_t, bool);
        void vSetValues(uint16_t, uint8_t, uint8_t);
        void vSetWebServer(class WebServer *);
        bool boGetSwitchStatus();
        void vSetColorMode(tColorMode, uint8_t);
        void vSetColor(uint8_t);
        void vSetDistanceCalibrationActive(bool);
        void vLoop();
        void vUpdateDayLight();

    private:
        class Eep       *pEep;
        class NtpTime   *pNtpTime;
        class WebServer *pWebServer;
        NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> *strip;
        NeoGamma<NeoGammaTableMethod> colorGamma;
        uint8_t u8GetBrightness();
        PT1 *cOnOffDamp = new PT1(10, 150);
        uint8_t  u8DebugLevel              = 0;
        bool     boCurrentSwitchMode       = false;
        bool     boNewSwitchMode           = false;
        bool     boInitRandomHue           = false;
        bool     boDistanceSensCalibActive = false;
        uint8_t u8NewSwitchBrightness      = 0;
};
#endif
