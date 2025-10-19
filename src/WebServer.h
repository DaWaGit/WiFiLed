#ifndef WebServer_h
#define WebServer_h

#ifdef ESP32
    #include <ESPAsyncWebServer.h>
    #include <WebSocketsServer.h>
    #include <SPIFFS.h>
#else
    #include <Arduino.h>
    #include <Hash.h>
    #include <ESPAsyncTCP.h>
    #include <ESPAsyncWebServer.h>
    #include <WebSocketsServer.h> // see: https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSocketsServer.h
    #include <FS.h>
#endif

#include "Eep.h"
#include "LedStripe.h"
#include "Buttons.h"
#include "NtpTime.h"

class WebServer {
    public:
        WebServer(int, int);
        void vInit(class Buttons *, class LedStripe *, class Eep *, class NtpTime *, uint8_t);
        void vSetIp(IPAddress);
        void vLoop();
        void vSendStripeStatus(int, bool);
        void vSendSunData(int, bool);
        void vSendColorMode(int, bool);

    private:
        void vWebSocketEvent(uint8_t, WStype_t, uint8_t *, size_t);
        void vSendDistanceSensorEnabled(int, bool);
        void vSendMotionSensorEnabled(int, bool);
        void vSendTimeSetup(int, bool);
        void vSendBufferToAllClients(char *, int);
        void vSendBufferToOneClient(char *, int);
        void vSendInitValues(int , bool);
        void vSendPowerOnRestoreSwitch(int, bool);

        class Eep *pEep;
        class LedStripe *pLedStripe;
        class Buttons *pButtons;
        class NtpTime *pNtpTime;
        String sTemplateProcessor(const String &);
        AsyncWebServer *pWebServer;
        WebSocketsServer *pWebSocket;
        IPAddress localIP;
        int iWebSocketPort;
};

#endif
