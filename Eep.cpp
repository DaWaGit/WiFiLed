#include <EEPROM.h>
#include "Eep.h"
#include "Utils.h"
#include "LedStripe.h"
#include "DebugLevel.h"

#define CLASS_NAME       "Eep"

#define EepAdr_ChipId                0
#define EepAdr_u16LedCount           (EepAdr_ChipId + sizeof(ESP.getChipId()))
#define EepAdr_u16CalibrationValue   (EepAdr_u16LedCount + sizeof(uint16_t))
#define EepAdr_u16Hue                (EepAdr_u16CalibrationValue + sizeof(uint16_t))
#define EepAdr_u8Saturation          (EepAdr_u16Hue + sizeof(uint16_t))
#define EepAdr_u8Brightness          (EepAdr_u8Saturation + sizeof(uint8_t))
#define EepAdr_u8DimMode             (EepAdr_u8Brightness + sizeof(uint8_t))
#define EepAdr_acWifiSsid            (EepAdr_u8DimMode + sizeof(uint8_t))
#define EepAdr_acWifiPwd             (EepAdr_acWifiSsid + EepSizeWifiSsid)
#define EepAdr_u8WiFiMode            (EepAdr_acWifiPwd + EepSizeWifiPwd)
#define EepAdr_u8BrightnessMin       (EepAdr_u8WiFiMode + sizeof(uint8_t))
#define EepAdr_u8BrightnessMax       (EepAdr_u8BrightnessMin + sizeof(uint8_t))
#define EepAdr_u8ColorMode           (EepAdr_u8BrightnessMax + sizeof(uint8_t))
#define EepAdr_u8Speed               (EepAdr_u8ColorMode + sizeof(uint8_t))
#define EepAdr_u8MotionOffDelay      (EepAdr_u8Speed + sizeof(uint8_t))
#define EepAdr_u8DistanceSensorEnabled (EepAdr_u8MotionOffDelay + sizeof(uint8_t))
#define EepAdr_u8MotionSensorEnabled   (EepAdr_u8DistanceSensorEnabled + sizeof(uint8_t))
#define EepAdr_Last                  (EepAdr_u8MotionSensorEnabled + sizeof(uint8_t))

//=======================================================================
Eep::Eep(uint8_t u8NewDebugLevel) {
    u8DebugLevel = u8NewDebugLevel;
}

//=======================================================================
void Eep::vInit() {
    uint32_t u32ChipId = 0;

    EEPROM.begin(512);
    EEPROM.get(EepAdr_ChipId, u32ChipId); // try to read ChipId from Eep

    if (u32ChipId != ESP.getChipId()) {
        // eep not initialized, write default values
        vFactoryReset();
    }

    // get all values
    EEPROM.get(EepAdr_ChipId,                u32ChipId);
    EEPROM.get(EepAdr_u16CalibrationValue,   u16CalibrationValue);
    EEPROM.get(EepAdr_u16LedCount,           u16LedCount);
    EEPROM.get(EepAdr_u16Hue,                u16Hue);
    EEPROM.get(EepAdr_u8Saturation,          u8Saturation);
    EEPROM.get(EepAdr_u8Brightness,          u8Brightness);
    EEPROM.get(EepAdr_u8BrightnessMin,       u8BrightnessMin);
    EEPROM.get(EepAdr_u8BrightnessMax,       u8BrightnessMax);
    EEPROM.get(EepAdr_u8DimMode,             u8DimMode);
    EEPROM.get(EepAdr_u8WiFiMode,            u8WiFiMode);
    EEPROM.get(EepAdr_u8ColorMode,           u8ColorMode); u8ColorMode = (u8ColorMode >= nNoMode) ? nNoMode-1 : u8ColorMode;
    EEPROM.get(EepAdr_u8Speed,               u8Speed);
    EEPROM.get(EepAdr_u8MotionOffDelay,      u8MotionOffDelay);
    EEPROM.get(EepAdr_u8DistanceSensorEnabled, u8DistanceSensorEnabled);
    EEPROM.get(EepAdr_u8MotionSensorEnabled,   u8MotionSensorEnabled);

    if (u8DebugLevel & DEBUG_EEP_EVENTS) {
        char buffer[100];
        sprintf(buffer, "Eep.Read Adr:0x%04X u32ChipId             = 0x%08X ",EepAdr_ChipId, u32ChipId); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u16CalibrationValue   = %d ", EepAdr_u16CalibrationValue, u16CalibrationValue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u16LedCount           = %d ", EepAdr_u16LedCount, u16LedCount); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u16Hue                = 0x%04X ", EepAdr_u16Hue, u16Hue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8Saturation          = 0x%02X ", EepAdr_u8Saturation, u8Saturation); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8Brightness          = 0x%02X ", EepAdr_u8Brightness, u8Brightness); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8BrightnessMin       = %d ", EepAdr_u8BrightnessMin, u8BrightnessMin); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8BrightnessMax       = %d ", EepAdr_u8BrightnessMax, u8BrightnessMax); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8DimMode             = 0x%02X ", EepAdr_u8DimMode, u8DimMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8WiFiMode            = 0x%02X ", EepAdr_u8WiFiMode, u8WiFiMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8ColorMode           = %d ", EepAdr_u8ColorMode, u8ColorMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8Speed               = %d ", EepAdr_u8Speed, u8Speed); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8MotionOffDelay      = %d [s]", EepAdr_u8MotionOffDelay, u8MotionOffDelay); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8DistanceSensorEnabled = %d", EepAdr_u8DistanceSensorEnabled, u8DistanceSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Read Adr:0x%04X u8MotionSensorEnabled   = %d", EepAdr_u8MotionSensorEnabled, u8MotionSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
//=======================================================================
void Eep::vFactoryReset() {
    char acWifiSsid[EepSizeWifiSsid];
    char acWifiPwd[EepSizeWifiPwd];

    EEPROM.put(EepAdr_ChipId, ESP.getChipId()); // ChipId
    vSetCalibrationValue(200, false);           // distance sensor calibration value (0..65535 default:200)
    vSetLedCount(300, false);                   // number of current configured LEDs (0..65535 default:300)
    vSetHue(0, false);                          // color hue (0..65535 default:0)
    vSetSaturation(0, false);                   // color saturation value (0..65535 default:0)
    vSetBrightness(128, false);                 // color brightness (0..255 default_128)
    vSetBrightnessMin(18, false);               // LED min brightness (0..255 default:24)
    vSetBrightnessMax(0xff, false);             // LED max brightness (0..255 default:255)
    vSetDimMode(1, false);                      // 0:brightness ++ or -- 1:brightness via distance
    vSetWiFiMode(1, false);                     // wifi mode (0:SSID 1:AP default:1)
    vSetColorMode(0, false);                    // color mode (0..2 default:0)
    vSetSpeed(128, false);                      // speed (1..255 default:128)
    vSetMotionOffDelay(60 - EepMotionOffDelayMin, false); // off delay (1..255 default:60)
    vSetDistanceSensorEnabled(0, false);          // enable/disable distance sensor (0..255 default:0)
    vSetMotionSensorEnabled(1, false);            // enable/disable motion sensor (0..255 default:1)

    for (int i = 0; i < EepSizeWifiSsid-1; i++) { acWifiSsid[i] = 0; }
    for (int i = 0; i < EepSizeWifiPwd-1; i++)  { acWifiPwd[i]  = 0; }
    vSetWifiSsidPwd(acWifiSsid, acWifiPwd, false);// set wifi SSID und Pwd

    if (u8DebugLevel & DEBUG_EEP_EVENTS) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X u32ChipId             = 0x%08X ",EepAdr_ChipId, ESP.getChipId()); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u16CalibrationValue   = %d ", EepAdr_u16CalibrationValue, u16CalibrationValue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u16LedCount           = %d ", EepAdr_u16LedCount, u16LedCount); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u16Hue                = 0x%04X ", EepAdr_u16Hue, u16Hue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8Saturation          = 0x%02X ", EepAdr_u8Saturation, u8Saturation); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8Brightness          = 0x%02X ", EepAdr_u8Brightness, u8Brightness); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8BrightnessMin       = %d ", EepAdr_u8BrightnessMin, u8BrightnessMin); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8BrightnessMax       = %d ", EepAdr_u8BrightnessMax, u8BrightnessMax); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8DimMode             = 0x%02X ", EepAdr_u8DimMode, u8DimMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8WiFiMode            = 0x%02X ", EepAdr_u8WiFiMode, u8WiFiMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8ColorMode           = %d ", EepAdr_u8ColorMode, u8ColorMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8Speed               = %d ", EepAdr_u8Speed, u8Speed); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8MotionOffDelay      = %d ", EepAdr_u8MotionOffDelay, u8MotionOffDelay); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8DistanceSensorEnabled = %d ", EepAdr_u8DistanceSensorEnabled, u8DistanceSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X u8MotionSensorEnabled   = %d ", EepAdr_u8MotionSensorEnabled, u8MotionSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X acWifiSsid            = %s ", EepAdr_acWifiSsid, acWifiSsid); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X acWifiPwd             = %s ", EepAdr_acWifiPwd, acWifiPwd); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
    ESP.restart(); // reset
}

//=======================================================================
void Eep::vSetHue(uint16_t u16NewHue, bool boPrintConsole) {
    uint16_t u16Hue_Tmp = 0;
    bool     boUpdated  = false;
    u16Hue = u16NewHue;
    EEPROM.get(EepAdr_u16Hue, u16Hue_Tmp);
    if (u16Hue_Tmp != u16Hue) {
        // at least one value changed
        EEPROM.put(EepAdr_u16Hue, u16Hue);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u16Hue = 0x%04X ", EepAdr_u16Hue, boUpdated ? "updated" : "unchanged", u16Hue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
//=======================================================================
void Eep::vSetSaturation(uint8_t u8NewSaturation, bool boPrintConsole) {
    uint8_t u8Saturation_Tmp = 0;
    bool    boUpdated        = false;
    u8Saturation = u8NewSaturation;
    EEPROM.get(EepAdr_u8Saturation, u8Saturation_Tmp);
    if (u8Saturation_Tmp != u8Saturation) {
        // at least one value changed
        EEPROM.put(EepAdr_u8Saturation,        u8Saturation);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8Saturation = 0x%02X ", EepAdr_u8Saturation, boUpdated ? "updated" : "unchanged", u8Saturation); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
//=======================================================================
void Eep::vSetBrightness(uint8_t u8NewBrightness, bool boPrintConsole) {
    uint8_t  u8Brightness_Tmp = 0;
    bool     boUpdated        = false;
    u8Brightness = u8NewBrightness;
    EEPROM.get(EepAdr_u8Brightness, u8Brightness_Tmp);
    if (u8Brightness_Tmp != u8Brightness) {
        // at least one value changed
        EEPROM.put(EepAdr_u8Brightness, u8Brightness);
        EEPROM.commit();
        boUpdated = true;
    }

    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8Brightness = 0x%02X ", EepAdr_u8Brightness, boUpdated ? "updated" : "unchanged", u8Brightness); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
//=======================================================================
void Eep::vSetDimMode(uint8_t u8NewDimMode, bool boPrintConsole) {
    uint8_t  u8DimMode_Tmp = 0;
    bool boUpdated          = false;
    u8DimMode = u8NewDimMode;
    EEPROM.get(EepAdr_u8DimMode, u8DimMode_Tmp);
    if (u8DimMode_Tmp != u8DimMode) {
        // at least one value changed
        EEPROM.put(EepAdr_u8DimMode, u8DimMode);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8DimMode = 0x%02X ", EepAdr_u8DimMode, boUpdated ? "updated" : "unchanged", u8DimMode); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
//=======================================================================
void Eep::vSetCalibrationValue(volatile uint16_t u16NewCalibrationValue, bool boPrintConsole) {
    uint16_t u16CalibrationValue_Tmp = 0;
    bool     boUpdated               = false;
    u16CalibrationValue = u16NewCalibrationValue;
    EEPROM.get(EepAdr_u16CalibrationValue, u16CalibrationValue_Tmp);
    if (u16CalibrationValue_Tmp != u16CalibrationValue) {
        // at least one value changed
        EEPROM.put(EepAdr_u16CalibrationValue, u16CalibrationValue);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u16CalibrationValue = %d ", EepAdr_u16CalibrationValue, boUpdated ? "updated" : "unchanged", u16CalibrationValue); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vGetWifiSsid(char *pWifiSsid) {
    // reat SSID byte-by-byte to EEPROM
    for (int i = 0; i < EepSizeWifiSsid; i++) {
        pWifiSsid[i] = EEPROM.read(EepAdr_acWifiSsid + i);
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS) {
        char buffer[100];
        sprintf(buffer, "Eep.Read Adr:0x%04X acWifiSsid = %s ", EepAdr_acWifiSsid, pWifiSsid); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
void Eep::vGetWifiPwd(char *pWifiPwd) {
    // read PWD byte-by-byte to EEPROM
    for (int i = 0; i < EepSizeWifiPwd; i++) {
        pWifiPwd[i] = EEPROM.read(EepAdr_acWifiPwd + i);
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS) {
        char buffer[100];
        sprintf(buffer, "Eep.Read Adr:0x%04X acWifiPwd = %s ", EepAdr_acWifiPwd, pWifiPwd); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
void Eep::vSetWifiSsidPwd(char *pNewWifiSsid, char *pNewWifiPwd, bool boPrintConsole) {
    // writing SSID byte-by-byte to EEPROM
    for (int i = 0; i < EepSizeWifiSsid; i++) {
        EEPROM.write(EepAdr_acWifiSsid + i, pNewWifiSsid[i]);
    }
    // writing PWD byte-by-byte to EEPROM
    for (int i = 0; i < EepSizeWifiPwd; i++) {
        EEPROM.write(EepAdr_acWifiPwd + i, pNewWifiPwd[i]);
    }
    EEPROM.commit();

    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X acWifiSsid = %s ", EepAdr_acWifiSsid, pNewWifiSsid); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        sprintf(buffer, "Eep.Write Adr:0x%04X acWifiPwd = %s ", EepAdr_acWifiPwd, pNewWifiPwd); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetWiFiMode(uint8_t u8NewWiFiMode, bool boPrintConsole) {
    uint8_t u8WiFiMode_Tmp = 0;
    bool boUpdated         = false;
    u8WiFiMode = u8NewWiFiMode;
    EEPROM.get(EepAdr_u8WiFiMode, u8WiFiMode_Tmp); // read the current LedCount
    if (u8WiFiMode_Tmp != u8WiFiMode) {
        // at least one value changed
        EEPROM.put(EepAdr_u8WiFiMode, u8WiFiMode);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8WiFiMode = %d ", EepAdr_u8WiFiMode, boUpdated ? "updated" : "unchanged", u8WiFiMode);
        vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetColorMode(uint8_t u8NewColorMode, bool boPrintConsole) {
    uint8_t u8ColorMode_Tmp = 0;
    bool boUpdated          = false;
    u8ColorMode = u8NewColorMode;
    EEPROM.get(EepAdr_u8ColorMode, u8ColorMode_Tmp); // read the current LedCount
    if (u8ColorMode_Tmp != u8ColorMode) {
        // at least one value changed
        EEPROM.put(EepAdr_u8ColorMode, u8ColorMode);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8ColorMode = %d ", EepAdr_u8ColorMode, boUpdated ? "updated" : "unchanged", u8ColorMode);
        vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetSpeed(uint8_t u8NewSpeed, bool boPrintConsole) {
    uint8_t u8Speed_Tmp = 0;
    bool boUpdated      = false;
    u8Speed = u8NewSpeed;
    EEPROM.get(EepAdr_u8Speed, u8Speed_Tmp); // read the current LedCount
    if (u8Speed_Tmp != u8Speed) {
        // at least one value changed
        EEPROM.put(EepAdr_u8Speed, u8Speed);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8Speed = %d ", EepAdr_u8Speed, boUpdated ? "updated" : "unchanged", u8Speed); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetDistanceSensorEnabled(uint8_t u8NewDistanceSensorEnabled, bool boPrintConsole) {
    uint8_t u8DistanceSensorEnabled_Tmp = 0;
    bool boUpdated                    = false;
    u8DistanceSensorEnabled = u8NewDistanceSensorEnabled;
    EEPROM.get(EepAdr_u8DistanceSensorEnabled, u8DistanceSensorEnabled_Tmp); // read the current LedCount
    if (u8DistanceSensorEnabled_Tmp != u8DistanceSensorEnabled) {
        // at least one value changed
        EEPROM.put(EepAdr_u8DistanceSensorEnabled, u8DistanceSensorEnabled);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8DistanceSensorEnabled = %d ", EepAdr_u8DistanceSensorEnabled, boUpdated ? "updated" : "unchanged", u8DistanceSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetMotionSensorEnabled(uint8_t u8NewMotionSensorEnabled, bool boPrintConsole) {
    uint8_t u8MotionSensorEnabled_Tmp = 0;
    bool boUpdated                  = false;
    u8MotionSensorEnabled = u8NewMotionSensorEnabled;
    EEPROM.get(EepAdr_u8MotionSensorEnabled, u8MotionSensorEnabled_Tmp); // read the current LedCount
    if (u8MotionSensorEnabled_Tmp != u8MotionSensorEnabled) {
        // at least one value changed
        EEPROM.put(EepAdr_u8MotionSensorEnabled, u8MotionSensorEnabled);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8MotionSensorEnabled = %d ", EepAdr_u8MotionSensorEnabled, boUpdated ? "updated" : "unchanged", u8MotionSensorEnabled); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetMotionOffDelay(uint8_t u8NewMotionOffDelay, bool boPrintConsole) {
    uint8_t u8MotionOffDelay_Tmp = 0;
    bool boUpdated               = false;
    u8MotionOffDelay = u8NewMotionOffDelay;
    EEPROM.get(EepAdr_u8MotionOffDelay, u8MotionOffDelay_Tmp); // read the current LedCount
    if (u8MotionOffDelay_Tmp != u8MotionOffDelay) {
        // at least one value changed
        EEPROM.put(EepAdr_u8MotionOffDelay, u8MotionOffDelay);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8MotionOffDelay = %d ", EepAdr_u8MotionOffDelay, boUpdated ? "updated" : "unchanged", u8MotionOffDelay); vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}

//=======================================================================
void Eep::vSetLedCount( uint16_t u16NewLedCount, bool boPrintConsole) {
    uint16_t u16LedCount_Tmp = 0;
    bool boUpdated           = false;
    u16LedCount = u16NewLedCount; // store new vale in RAM
    EEPROM.get(EepAdr_u16LedCount, u16LedCount_Tmp); // read the current LedCount
    if (u16LedCount_Tmp != u16LedCount) {
        // at least one value changed
        EEPROM.put(EepAdr_u16LedCount, u16LedCount);
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u16LedCount = %d ", EepAdr_u16LedCount, boUpdated ? "updated" : "unchanged", u16LedCount);
        vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
void Eep::vSetBrightnessMin( uint8_t u8NewBrightnessMin, bool boPrintConsole) {
    uint16_t u8BrightnessMin_Tmp = 0;
    bool boUpdated               = false;
    u8BrightnessMin = u8NewBrightnessMin; // store new vale in RAM
    EEPROM.get(EepAdr_u8BrightnessMin, u8BrightnessMin_Tmp); // read the current value from EEP
    if (u8BrightnessMin_Tmp != u8BrightnessMin) {
        // at least one value changed
        EEPROM.put(EepAdr_u8BrightnessMin, u8BrightnessMin); // store new value in EEP
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8BrightnessMin = %d ", EepAdr_u8BrightnessMin, boUpdated ? "updated" : "unchanged", u8BrightnessMin);
        vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
void Eep::vSetBrightnessMax( uint8_t u8NewBrightnessMax, bool boPrintConsole) {
    uint16_t u8BrightnessMax_Tmp = 0;
    bool boUpdated               = false;
    u8BrightnessMax = u8NewBrightnessMax; // store new vale in RAM
    EEPROM.get(EepAdr_u8BrightnessMax, u8BrightnessMax_Tmp); // read the current value from EEP
    if (u8BrightnessMax_Tmp != u8BrightnessMax) {
        // at least one value changed
        EEPROM.put(EepAdr_u8BrightnessMax, u8BrightnessMax); // store new value in EEP
        EEPROM.commit();
        boUpdated = true;
    }
    if (u8DebugLevel & DEBUG_EEP_EVENTS && boPrintConsole) {
        char buffer[100];
        sprintf(buffer, "Eep.Write Adr:0x%04X %s u8BrightnessMax = %d ", EepAdr_u8BrightnessMax, boUpdated ? "updated" : "unchanged", u8BrightnessMax);
        vConsole(u8DebugLevel, DEBUG_EEP_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
    }
}
