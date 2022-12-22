#ifndef LedStripe_h
#define LedStripe_h
#include "PT1.h"
#include "Eep.h"
#include "WebServer.h"
#include <Arduino.h> // see: https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

#include <Adafruit_NeoPixel.h> // see: https://github.com/adafruit/Adafruit_NeoPixel
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
        void vInit(class Eep *);
        void vTurn(bool, bool);
        void vSetMonochrome(uint16_t, uint8_t, uint8_t);
        void vSetRainbow(long, uint8_t, uint8_t);
        void vSetRandom(uint8_t, uint8_t, bool, uint8_t);
        void vSetMovingPoint(uint16_t, uint8_t, uint8_t, bool);
        void vSetValues(uint16_t, uint8_t, uint8_t);
        void vSetWebServer(class WebServer *);
        void vLoop();
        bool boGetSwitchStatus();
        uint16_t u16GetHue();
        uint8_t u8GetSaturation();
        uint8_t u8GetBrightness();
        void vSetColorMode(tColorMode);

    private:
        class Eep         *pEep;
        class WebServer   *pWebServer;
        Adafruit_NeoPixel *stripe;
        PT1      *cOnOffDamp           = new PT1(10, 150);
        uint8_t  u8DebugLevel          = 0;
        bool     boCurrentSwitchMode   = false;
        bool     boNewSwitchMode       = false;
        bool     boInitRandomHue       = false;
        uint8_t  u8NewSwitchBrightness = 0;
};
#endif
