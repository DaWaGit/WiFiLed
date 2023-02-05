#include "Buttons.h"
#include "Utils.h"
#include "DebugLevel.h"

#define CLASS_NAME "Buttons"

#define PIN_IrDistanceSensor  A0  // IR distance sensor, ESP8266 Analog Pin ADC0 = A0
#define PIN_MotionSensor1     D2  // Motion sensor AM312, ESP8266 D2 Pin see: https://www.alldatasheet.com/datasheet-pdf/pdf/1179499/ETC2/AM312.html
#define PIN_MotionSensor2     D0  // Motion sensor AM312, ESP8266 D2 Pin see: https://www.alldatasheet.com/datasheet-pdf/pdf/1179499/ETC2/AM312.html
#define ADC_INTERVAL 10           // start ADC every x ms (WIFI and Webserver are unstable when ADC converts very often!?)
#define Button_ShortPress     30  // Button pressed longer than x ms but shorter than Button_LongPress
#define Button_LongPress      500 // Button pressed longer than x ms

//=============================================================================
Buttons::Buttons(uint8_t u8NewDebugLevel) {
    u8DebugLevel = u8NewDebugLevel; // store debug level
    // configure calibration button
    pinMode(PIN_MotionSensor1, INPUT); // see: file:///C:/Program%20Files%20(x86)/Arduino/reference/www.arduino.cc/en/Reference/PinMode.html
    pinMode(PIN_MotionSensor2, INPUT); // see: file:///C:/Program%20Files%20(x86)/Arduino/reference/www.arduino.cc/en/Reference/PinMode.html
}

//=============================================================================
void Buttons::vInit(class LedStripe *pNewLedStripe, class Eep *pNewEep, class NtpTime *pNewNtpTime) {
    pLedStripe = pNewLedStripe; // store LedStripe class
    pEep       = pNewEep;       // store eep class
    pNtpTime   = pNewNtpTime;   // store ntp class
}

//=============================================================================
void Buttons::vSetWebServer(class WebServer *pNewWebServer) {
    pWebServer = pNewWebServer;
}

//=============================================================================
void Buttons::vSet(tButtonStatus enNewButtonStatus) {
    char buffer[50];

    enButtonStatus = enNewButtonStatus;
    sprintf(buffer, "enButtonStatus: %d", enButtonStatus); vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
}

//=============================================================================
void Buttons::vReadIrSensorValue() {
    static ulong ulDownTimestamp = 0;
    static ulong ulDownTime      = 0;
    uint16_t u16AdcValue         = 0;

    //.........................................................................
    u16AdcValue = analogRead(PIN_IrDistanceSensor); // get value from ID distance sensor

    if (enButtonStatus == nCalibrationButton_Pressed) {
        // get the max value from IR distance sensor
        if ((u16AdcValue + 50) > pEep->u16CalibrationValue) {
            pEep->u16CalibrationValue = (u16AdcValue + 50);
        }
    } else {
        if (u16AdcValue >= pEep->u16CalibrationValue) {
            // button is pressed
            if (!ulDownTimestamp) {
                // first time down detected
                ulDownTimestamp = millis();
            }
            ulDownTime = millis() - ulDownTimestamp;
            if (   (ulDownTime > Button_LongPress )
                || (enButtonStatus == nIrButton_LongPress)) {
                    // button was pressed for more than 500ms
                    enButtonStatus = nIrButton_LongPress;
                    u16IrDistance  = u16AdcValue;
                }
        } else {
            // button is not pressed
            if ( ulDownTime ) {
                // button was pressed
                if (   (ulDownTime > Button_ShortPress) && (ulDownTime < Button_LongPress)    // pressed for 40ms-300ms
                    && (enButtonStatus != nIrButton_LongPress) ) { // long press was not active
                    // button was pressed for 30ms-500ms
                    enButtonStatus = nIrButton_ShortPressed;
                } else {
                    // button was long pressed
                    enButtonStatus = nNone;
                }
                ulIrButtonDownTime = ulDownTime;
                ulDownTime         = 0;
                ulDownTimestamp    = 0;
            }
        }
    }
}

//=============================================================================
void Buttons::vReadMotionSensor() {
    static ulong ulMotionOffTime = 0;
    static bool boMotionDetected  = false;
    static bool boMotionOffDelay  = false;
    static bool boStripeOn        = false;
    char buffer[50];

    if (enButtonStatus == nNone) {
        if (   (digitalRead(PIN_MotionSensor1) == HIGH)
            || (digitalRead(PIN_MotionSensor2) == HIGH)) {
            // motion detected
            boMotionDetected = true;
            boMotionOffDelay = false;
            enButtonStatus   = nMotion_Detected;
        } else if (   boMotionDetected
                   && !boMotionOffDelay) {
            // no motion anymore
            boMotionDetected = false;
            boMotionOffDelay = true;
            ulMotionOffTime = millis();
        } else  if (   ((millis() - ulMotionOffTime) >= ((ulong)pEep->u8MotionOffDelay)*1000)
                    && boMotionOffDelay) {
            boMotionOffDelay = false;
            enButtonStatus = nMotion_Finished;
        }
    }
}

//=============================================================================
void Buttons::vLoop() {
    static ulong ulLastAdcInterval   = 0;
    static bool boButtonStatusShowed = false;
    static bool boTmpStripeOn        = false;
    static bool boStripeOn           = false;
    static bool boDarken             = true; // true:u8BrightnessDay/Night-- / false: u8BrightnessDay/Night++
    static PT1 *cDimDamp             = new PT1(10, 300);

    if (pEep->u8MotionSensorEnabled) {
        vReadMotionSensor();
    }
    if (pEep->u8DistanceSensorEnabled) {
        if ((millis() - ulLastAdcInterval) >= ADC_INTERVAL) {
            // patch: ADC read only every 5ms, otherwise WiFi connection will lost
            // see  : https://github.com/esp8266/Arduino/issues/1634
            ulLastAdcInterval = millis();
            vReadIrSensorValue();
            //...................................................................
            // show IR button downtime
            if (ulIrButtonDownTime) {
                char buffer[50];
                sprintf(buffer, "Button.IrButton : DownTime %dms", ulIrButtonDownTime); vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
                ulIrButtonDownTime = 0;
            }
        }
    }
    //...................................................................
    // handle button events
    switch (enButtonStatus)  {
        //..............................
        case nCalibrationButton_Pressed:
            pLedStripe->vSetDistanceCalibrationActive(true);
            if (!boButtonStatusShowed) {
                vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.CalibrationButton : Pressend");
                // store the current LED status
                pEep->u16CalibrationValue = 0; // reset calibration value
                boTmpStripeOn             = pLedStripe->boGetSwitchStatus();
                if (boTmpStripeOn) pLedStripe->vTurn(false, true); // turn fast off
                pLedStripe->vSetMonochrome(0, 0, pEep->u8BrightnessMax,0);
                boButtonStatusShowed = true;
            }
            break;
        //..............................
        case nCalibrationButton_Released:
            // calibration button was pressed
            vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.CalibrationButton : Released");
            pEep->vSetCalibrationValue(pEep->u16CalibrationValue, true);
            pLedStripe->vSetDistanceCalibrationActive(false);
            pLedStripe->vSetColor(-1);
            pLedStripe->vTurn(boTmpStripeOn, true); // turn fast on/off
            enButtonStatus = nNone; // consume the status
            break;
        //..............................
        case nIrButton_ShortPressed:
            if (!boButtonStatusShowed) {
                vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.IrButton : ShortPressed");
                boButtonStatusShowed = true;
            }
            enButtonStatus = nNone; // consume the status
            boStripeOn     = !pLedStripe->boGetSwitchStatus(); // toggle turn mode
            pLedStripe->vTurn(boStripeOn, false); // turn smooth on/off
            if (pWebServer)
                pWebServer->vSendStripeStatus(-1, true); // update values for every web client
            break;
        //..............................
        case nIrButton_LongPress:
            if (!boButtonStatusShowed) {
                vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.IrButton : LongPress");
                boButtonStatusShowed = true;
            }
            if (!boStripeOn) {
                // tripe is off, turn it now on
                boStripeOn = true;
                pLedStripe->vTurn(boStripeOn, true);   // turn fast on
                cDimDamp->vInitDampVal(u8GetBrightness()); // init dim damper
            }
            switch ((tDimMode)pEep->u8DimMode) {
                case nByDistance:
                    // change brightness via distance
                    if (pNtpTime->stLocal.boSunHasRisen) {
                        pEep->u8BrightnessDay = (uint8_t)cDimDamp->fGetDampedVal(
                            map(
                                u16IrDistance,
                                pEep->u16CalibrationValue,
                                1024,
                                pEep->u8BrightnessMin,
                                pEep->u8BrightnessMax));
                    } else {
                        pEep->u8BrightnessNight = (uint8_t)cDimDamp->fGetDampedVal(
                            map(
                                u16IrDistance,
                                pEep->u16CalibrationValue,
                                1024,
                                pEep->u8BrightnessMin,
                                pEep->u8BrightnessMax));
                    }
                    pLedStripe->vSetColor(-1);
                    break;
                case nIncremental:
                default:
                    // change brightness via ++/--
                    if (boDarken) {
                        // reduce the brightness
                        if (u8GetBrightness() >= pEep->u8BrightnessMin)
                            if (pNtpTime->stLocal.boSunHasRisen) {
                                pEep->u8BrightnessDay--;
                            } else {
                                pEep->u8BrightnessNight--;
                            }
                        else
                            boDarken = false; // change mode
                    } else {
                        // increase the brightness
                        if (u8GetBrightness() < pEep->u8BrightnessMax)
                            if (pNtpTime->stLocal.boSunHasRisen) {
                                pEep->u8BrightnessDay++;
                            } else {
                                pEep->u8BrightnessNight--;
                            }
                        else
                            boDarken = true; // change mode
                    }
                    pLedStripe->vSetColor(-1);
                    break;
            }
            break;
        //..............................
        case nMotion_Detected:
            enButtonStatus = nNone; // consume the status
            if (!pLedStripe->boGetSwitchStatus()) {
                // stripe is off
                vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.MotionDetected");
                if (!boButtonStatusShowed) {
                    boButtonStatusShowed = true;
                }
                boStripeOn = true;
                pLedStripe->vTurn(boStripeOn, false);    // turn smooth on
                if (pWebServer)
                    pWebServer->vSendStripeStatus(-1, true); // update values for every web client
            }
            break;
        //..............................
        case nMotion_Finished:
            enButtonStatus = nNone; // consume the status
            if (pLedStripe->boGetSwitchStatus()) {
                // stripe is on
                vConsole(u8DebugLevel, DEBUG_BUTTON_EVENTS, CLASS_NAME, __FUNCTION__, "Button.MotionFinished");
                if (!boButtonStatusShowed) {
                    boButtonStatusShowed = true;
                }
                boStripeOn = false;
                pLedStripe->vTurn(boStripeOn, false); // turn smooth off
                if (pWebServer)
                    pWebServer->vSendStripeStatus(-1, true); // update values for every web client
            }
            break;
        //..............................
        default:
            boButtonStatusShowed = false;
            break;
    }
}

//=============================================================================
uint8_t Buttons::u8GetBrightness() {
    return pNtpTime->stLocal.boSunHasRisen ? pEep->u8BrightnessDay : pEep->u8BrightnessNight;
}
