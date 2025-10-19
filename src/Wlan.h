#ifndef Wlan_h
#define Wlan_h
#include <Arduino.h>
#include <ESP8266WiFi.h> // see: https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#quick-start
#include <ESP8266HTTPClient.h>

#include "Eep.h"
#include "LedStripe.h"
#include "Buttons.h"
#include "NtpTime.h"
#include "WebServer.h"

class Wlan {
    public:
        Wlan(uint8_t);
        class WebServer* vInit(class Buttons *, class LedStripe *, class Eep *, class NtpTime *);
        void vLoop();
        IPAddress localIP();
        bool boSSIDconnected;

    private:
        void vInitWiFiMode(bool);
        bool boApMode;
};

#endif
