#ifndef Buttons_h
#define Buttons_h

#include "LedStripe.h"
#include "WebServer.h"
#include "Eep.h"

enum tButtonStatus {
    nNone = 0,                  // no button pressed
    nIrButton_ShortPressed,     // IR button short pressed (IrDistanceSensor)
    nIrButton_LongPress,        // IR button long pressed (IrDistanceSensor)
    nMotion_Detected,           // motion detected
    nMotion_Finished,           // motion finised
    nCalibrationButton_Pressed, // Calibration button pressed (CalibrationButton)
    nCalibrationButton_Released // Calibration button released (CalibrationButton)
};
enum tDimMode {
    nIncremental = 0, // brightness ++ or --
    nByDistance       // brightness via distance
};

class Buttons {
    public:
        Buttons(uint8_t);
        void vInit(class LedStripe *, class Eep *);
        void vLoop();
        void vSet(tButtonStatus);
        void vSetWebServer(class WebServer *);

    private:
        void vReadIrSensorValue();
        void vReadMotionSensor();
        class Eep *pEep;
        class WebServer *pWebServer;
        class LedStripe *pLedStripe;
        uint8_t u8DebugLevel         = 0;
        tButtonStatus enButtonStatus = nNone;
        uint16_t u16IrDistance       = 0;     // adc value from GP2Y0A21YK0F IR distance sensor
        ulong ulIrButtonDownTime     = 0;     // IrButton down time in ms
};
#endif
