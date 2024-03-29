#ifndef Eep_h
#define Eep_h
#include "NtpTime.h"
#include <Arduino.h>

#define EepMotionOffDelayMin 4

#define EepStringSize 50

class Eep {
    public:
        Eep(uint8_t);
        void vInit(class NtpTime *);
        void vFactoryReset();
        void vGetWifiSsid(char *);                     // read SSID
        void vGetWifiPwd(char *);                      // read PWD
        void vSetWifiSsidPwd(char *, char *, bool);    // update SSID and PWD
        void vSetLedCount(uint16_t, bool);             // store a new ledCount value
        void vSetCalibrationValue(uint16_t, bool);     // store distance sensor calibration value
        void vSetHue(uint16_t, bool);                  // store hue value
        void vSetSaturation(uint8_t, bool);            // store saturation value
        void vSetBrightnessDay(uint8_t, bool);         // store brightness value
        void vSetBrightnessNight(uint8_t, bool);       // store brightness value
        void vSetDimMode(uint8_t, bool);               // store the new dim mode
        void vSetBrightnessMin(uint8_t, bool);         // store a new brightness min value
        void vSetBrightnessMax(uint8_t, bool);         // store a new brightness max value
        void vSetWiFiMode(uint8_t, bool);              // store WiFi AP mode for the next start  (0:SSID / 1:AP)
        void vSetColorMode(uint8_t, bool);             // store color mode (0..2 default:0)
        void vSetSpeed(uint8_t, bool);                 // store speed (1..255 default:128)
        void vSetMotionOffDelay(uint8_t, bool);        // store turn off delay [s] (1..255 default:60)
        void vSetDistanceSensorEnabled(uint8_t, bool); // store enable/disable distance sensor (0..255 default:0)
        void vSetMotionSensorEnabled(uint8_t, bool);   // store enable/disable motion sensor (0..255 default:1)
        void vSetLongitude(double , bool);             // store longitude
        void vSetLatitude(double, bool);               // store latitude
        void vSetNtp(char *, char *, char *, char *, bool); // store NTP TimeZone, NTP Server1, NTP Server2
        void vSetSwitchStatus(uint8_t, bool);          // store switch status
        void vSetPowerOnRestoreSwitch(uint8_t, bool);  // store mode for "restore switch status after PowerOnReset"

        uint16_t u16LedCount;          // number of current configured LEDs (0..65535 default:300)
        uint16_t u16CalibrationValue;      // distance sensor calibration value (0..65535 default:200)
        uint16_t u16Hue;                   // color hue (0..65535 default:0)
        uint8_t u8Saturation;              // color saturation value (0..65535 default:0)
        uint8_t u8BrightnessDay;           // color brightness (0..255 default_128)
        uint8_t u8BrightnessNight;         // color brightness (0..255 default_128)
        uint8_t u8BrightnessMin;           // LED min brightness (0..255 default:24)
        uint8_t u8BrightnessMax;           // LED max brightness (0..255 default:255)
        uint8_t u8DimMode;                 // distancesensor dim mode (0:brightness ++/-- 1:brightness via distance default:1)
        uint8_t u8WiFiApMode;              // wifi mode (0:SSID 1:AP default:1)
        uint8_t u8ColorMode;               // color mode (0..2 default:0)
        uint8_t u8Speed;                   // speed (0..255 default:128)
        uint8_t u8MotionOffDelay;          // turn off delay [s] (1..255 default:60)
        uint8_t u8DistanceSensorEnabled;   // enable/disable distance sensor (0..255 default:0)
        uint8_t u8MotionSensorEnabled;     // enable/disable motion sensor (0..255 default:1)
        uint8_t u8SwitchStatus;            // switch status (0..1 default:0)
        uint8_t u8PowerOnRestoreSwitch;    // restore switch status after PowerOn (0..1 default:0)
        char acTimeZone[EepStringSize];    // NTP Time Zone String
        char acTimeZoneName[EepStringSize];// NTP Time Zone Name string
        char acNtpServer1[EepStringSize];  // NTP server1
        char acNtpServer2[EepStringSize];  // NTP server2
        double dLongitude;                 // position Longitude
        double dLatitude;                  // position Latitude

    private:
        uint8_t u8DebugLevel = 0;
        class NtpTime *pNtpTime;
        };
#endif
