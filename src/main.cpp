// #############################################################################
//  Project  : WiFiLed (control RGB LED stripe with WS2812 LED's via ESP32)
//  Author   : Daniel Warnicki 22.12.2022
//  GitHubUrl: https://github.com/DaWaGit/WiFiLed.git
//  Doc      : https://github.com/DaWaGit/WiFiLed/blob/main/README.md
// #############################################################################
#define CLASS_NAME       "WiFiLed"

#include <Arduino.h>
#include <string.h>
#include <ArduinoJson.h>  // see: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
#include <PubSubClient.h> // see: https://pubsubclient.knolleary.net/

#include "Eep.h"        // EEP interface
#include "LedStripe.h"  // LED controll
#include "Buttons.h"    // button handling
#include "PT1.h"        // PT1 damping
#include "Wlan.h"       // WiFi Interface
#include "Version.h"    // version definition
#include "Utils.h"      // useful utils
#include "DebugLevel.h" // debug level definiton
#include "NtpTime.h"    // NTP time

#define mqttSendCyclicInterval 60000 * 5 // send values cyclic every 5min
#define mqttReconnectInterval   1000 * 5 // mqtt reconnect interval 5sec
#define mqttSendEventInterval   1000     // send changed values earliest 1sec

//=======================================================================
//                               Globals
//=======================================================================
Eep oEep(DEBUG_LEVEL);             // create the Eep object
LedStripe oLedStripe(DEBUG_LEVEL); // create the LedStrip object
Buttons oButtons(DEBUG_LEVEL);     // create the Button object
Wlan oWlan(DEBUG_LEVEL);           // create Wlan object
NtpTime oNtpTime(DEBUG_LEVEL);     // create an NTP time object
WebServer *pWebServer = NULL;

const char *mqttServerIp = "192.168.1.18";
const long mqttPort      = 1883;

char acMqttTxTopic[30];
char acMqttRxTopic[30];
WiFiClient espWiFiClient;
PubSubClient mqttClient(espWiFiClient);
long mqttCyclicTxTimer  = 0;
long mqttEventTxTimer   = 0;
long mqttReconnectTimer = 0;

//=======================================================================
void vPrintChipInfo() {
    // use the following link to generate BigTxt: https://patorjk.com/software/taag/#p=display&f=Big&t=LedColorStripe
    // TODO: use StackTRace Decode to find the root cause see: https://github.com/me-no-dev/EspExceptionDecoder
    Serial.println("============================================================================");
    Serial.println(" __          ___ ______ _ _              _ ");
    Serial.println(" \\ \\        / (_)  ____(_) |            | |");
    Serial.println("  \\ \\  /\\  / / _| |__   _| |     ___  __| |");
    Serial.println("   \\ \\/  \\/ / | |  __| | | |    / _ \\/ _` |");
    Serial.println("    \\  /\\  /  | | |    | | |___|  __/ (_| |");
    Serial.println("     \\/  \\/   |_|_|    |_|______\\___|\\__,_|");
    Serial.println("----------------------------------------------------------------------------");
    Serial.print("Board  : "); Serial.println(ARDUINO_BOARD);
    Serial.print("Version: V"); Serial.println(VERSION);
    Serial.print("Author : DaWa (Daniel Warnicki)\n");
    Serial.print("Date   : "); Serial.print(__DATE__); Serial.print(" "); Serial.println(__TIME__);
    Serial.println("----------------------------------------------------------------------------");
    Serial.print("ResetReason      : "); Serial.println(ESP.getResetReason()); // returns a String containing the last reset reason in human readable format.
    Serial.print("FreeHeap         : "); Serial.println(ESP.getFreeHeap()); // returns the free heap size.
    Serial.print("HeapFragmentation: "); Serial.println(ESP.getHeapFragmentation()); // returns the fragmentation metric (0% is clean, more than ~50% is not harmless)
    Serial.print("MaxFreeBlockSize : "); Serial.println(ESP.getMaxFreeBlockSize()); // returns the maximum allocatable ram block regarding heap fragmentation
    Serial.print("ChipId           : 0x"); Serial.println(ESP.getChipId(), HEX); // returns the ESP8266 chip ID as a 32-bit integer.
    Serial.print("CoreVersion      : "); Serial.println(ESP.getCoreVersion()); // returns a String containing the core version.
    Serial.print("SdkVersion       : "); Serial.println(ESP.getSdkVersion()); // returns the SDK version as a char.
    Serial.print("CpuFreqMHz       : "); Serial.println(ESP.getCpuFreqMHz()); // returns the CPU frequency in MHz as an unsigned 8-bit integer.
    Serial.print("SketchSize       : "); Serial.println(ESP.getSketchSize()); // returns the size of the current sketch as an unsigned 32-bit integer.
    Serial.print("FreeSketchSpace  : "); Serial.println(ESP.getFreeSketchSpace()); // returns the free sketch space as an unsigned 32-bit integer.
    Serial.print("SketchMD5        : "); Serial.println(ESP.getSketchMD5()); // returns a lowercase String containing the MD5 of the current sketch.
    Serial.print("FlashChipId      : 0x"); Serial.println(ESP.getFlashChipId(), HEX); // returns the flash chip ID as a 32-bit integer.
    Serial.print("FlashChipSize    : "); Serial.println(ESP.getFlashChipSize()); // returns the flash chip size, in bytes, as seen by the SDK (may be less than actual size).
    Serial.print("FlashChipRealSize: "); Serial.println(ESP.getFlashChipRealSize()); // returns the real chip size, in bytes, based on the flash chip ID.
    Serial.print("FlashChipSpeed   : "); Serial.println(ESP.getFlashChipSpeed()); // returns the flash chip frequency, in Hz.
    Serial.print("CycleCount       : "); Serial.println(ESP.getCycleCount()); // returns the cpu instruction cycle count since start as an unsigned 32-bit. This is useful for accurate timing of very short actions like bit banging.
    Serial.print("Vcc              : "); Serial.println(ESP.getVcc()); //
    Serial.println("============================================================================");
}

//=======================================================================
void vMqttRx(char *topic, byte *message, unsigned int length) {
    String sPayload = String((char *)message).substring(0, length);
    sPayload[length] = '\0';

    // convert sPayload String in a JSON object, see: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
    DynamicJsonDocument doc(255);
    DeserializationError err = deserializeJson(doc, sPayload.c_str());
    if (err) {
        Serial.printf("[%s::%s] JSON Error: %s \n", CLASS_NAME, __FUNCTION__, err.f_str());
        Serial.printf("                   Topic:%s\n", topic);
        Serial.printf("                   Length:%d\n", length);
        Serial.printf("                   Message:%s\n", sPayload.c_str());
    } else {
        int8_t jsonSwitch    = (doc["switch"] | -1)    >= 0 ? doc["switch"].as<int8_t>()    : -1;
        long jsonHue         = (doc["hue"] | -1)       >= 0 ? doc["hue"].as<long>()         : -1;
        int16_t jsonSat      = (doc["sat"] | -1)       >= 0 ? doc["sat"].as<int16_t>()      : -1;
        int16_t jsonBri      = (doc["bri"] | -1)       >= 0 ? doc["bri"].as<int16_t>()      : -1;
        int8_t jsonColorMode = (doc["colorMode"] | -1) >= 0 ? doc["colorMode"].as<int8_t>() : -1;
        int16_t jsonSpeed    = (doc["speed"] | -1)     >= 0 ? doc["speed"].as<int16_t>()    : -1;
        if (DEBUG_LEVEL & DEBUG_WEBSERVER_EVENTS) {
            Serial.printf("[%s::%s] ", CLASS_NAME, __FUNCTION__);
            if (jsonSwitch    >= 0) Serial.printf("switch:%d\n                   ",    jsonSwitch);
            if (jsonHue       >= 0) Serial.printf("hue:%d\n                   ",       jsonHue);
            if (jsonSat       >= 0) Serial.printf("sat:%d\n                   ",       jsonSat);
            if (jsonBri       >= 0) Serial.printf("bri:%d\n                   ",       jsonBri);
            if (jsonColorMode >= 0) Serial.printf("colorMode:%d\n                   ", jsonColorMode);
            if (jsonSpeed     >= 0) Serial.printf("Speed:%d\n                   ",     jsonSpeed);
            Serial.printf("\n");
        }
        if (jsonHue >= 0 || jsonSat >= 0 || jsonBri >= 0 || jsonColorMode >= 0 || jsonSpeed >=0 ) {
            oEep.vSetSpeed(jsonSpeed >= 0 ? (int8_t)jsonSpeed : oEep.u8Speed, true );
            oLedStripe.vSetValues(
                jsonHue >= 0 ? (uint16_t)jsonHue : oEep.u16Hue,
                jsonSat >= 0 ? (uint8_t)jsonSat  : oEep.u8Saturation,
                jsonBri >= 0 ? (uint8_t)jsonBri  : (oNtpTime.stLocal.boSunHasRisen ? oEep.u8BrightnessDay : oEep.u8BrightnessNight)
            );
            oLedStripe.vSetColorMode(jsonColorMode >= 0 ? (tColorMode)jsonColorMode : (tColorMode)oEep.u8ColorMode, true);
            pWebServer->vSendColorMode(-1, true);
        }
        if (jsonSwitch >= 0) oLedStripe.vTurn((bool)jsonSwitch, false); // turn smooth on/off
    }
}

//=======================================================================
void vMqttTx() {
    char payload[100];
    sprintf(
        payload,
        "{\"switch\":%d,\"hue\":%d,\"sat\":%d,\"bri\":%d,\"colorMode\":%d,\"speed\":%d}", //,\"sunHasRisen\":1,\"time\":2}",
        oLedStripe.boGetSwitchStatus(),
        oEep.u16Hue,
        oEep.u8Saturation,
        oLedStripe.u8GetBrightness(),
        oEep.u8ColorMode,
        oEep.u8Speed);
    mqttClient.publish(acMqttTxTopic, payload);
    if (DEBUG_LEVEL & DEBUG_WEBSERVER_EVENTS) {
        Serial.printf("[%s::%s] %s = %s\n", CLASS_NAME, __FUNCTION__, acMqttTxTopic, payload);
    }
}
//=======================================================================
void vMqttReconnect() {
    // Loop until we're reconnected
    if (!mqttClient.connected()) {
        long now = millis();
        if (now - mqttReconnectTimer > mqttReconnectInterval) {
            mqttReconnectTimer = now;
            if (DEBUG_LEVEL & DEBUG_WEBSERVER_EVENTS) {
                Serial.printf("[%s::%s] MQTT connection to Server:%s Port:%d Status:", CLASS_NAME, __FUNCTION__, mqttServerIp, mqttPort);
            }
            // Attempt to connect
            if (mqttClient.connect("ESP8266Client")) {
                if (DEBUG_LEVEL & DEBUG_WEBSERVER_EVENTS) {
                    Serial.println("connected");
                }
                // Subscribe
                mqttClient.subscribe(acMqttRxTopic);
            } else {
                if (DEBUG_LEVEL & DEBUG_WEBSERVER_EVENTS) {
                    Serial.print("failed, rc=");
                    Serial.print(mqttClient.state());
                    Serial.println(", try again in 5sec");
                }
            }
        }
    }
}

//=======================================================================
//                               Setup
//=======================================================================
void setup() {

    // init variables
    sprintf(acMqttTxTopic, "stat/wifiled_%08X/STATE", ESP.getChipId());
    sprintf(acMqttRxTopic, "cmnd/wifiled_%08X/VALUES", ESP.getChipId());

    // init serial monitor
    Serial.begin(115200);
    Serial.println("");
    vPrintChipInfo();
    Serial.setDebugOutput(DEBUG_LEVEL & DEBUG_GLOBAL_OUTPUT ? true : false);

    oEep.vInit(&oNtpTime);              // download all EEP values
    oLedStripe.vInit(&oEep, &oNtpTime); // init LED strip
    oButtons.vInit(&oLedStripe, &oEep, &oNtpTime); // init Buttons
    pWebServer = oWlan.vInit(&oButtons, &oLedStripe, &oEep, &oNtpTime); // init Wlan+Webserver
    oNtpTime.vSetLedStripe(&oLedStripe);
    oNtpTime.vSetWebServer(pWebServer);

    oNtpTime.vInit(
        oEep.acTimeZone,   // TimeZone see: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
        oEep.acNtpServer1, // NTP server 1 e.g. "ptbtime1.ptb.de"
        oEep.acNtpServer2, // NTP server 2 e.g. "ptbtime2.ptb.de"
        "ptbtime3.ptb.de", // NTP server 3 e.g. "ptbtime3.ptb.de"
        oEep.dLatitude,    // latitude
        oEep.dLongitude    // longitude
    );
    // setup MQTT
    mqttClient.setServer(mqttServerIp, mqttPort);
    mqttClient.setCallback(vMqttRx);
}
//=======================================================================
//                               MAIN LOOP
//=======================================================================
void loop() {
    static bool boSwitchStatus   = false;
    static uint16_t u16Hue       = 0;
    static uint8_t u8Saturation  = 0;
    static uint8_t u8Brightness  = 0;
    static uint8_t u8ColorMode   = 0;
    static uint8_t u8Speed       = 0;
    bool topicStat               = false;

    oButtons.vLoop();   // detect and handle Button events
    oLedStripe.vLoop(); // damp stripe changes
    oWlan.vLoop();      // check Wlan status, reconnect
    oNtpTime.vLoop();   // calculate sunrise and sun set dependent on the current time

    if (oWlan.boSSIDconnected) {

        // establish MQTT connection
        vMqttReconnect();
        mqttClient.loop();

        // send MQTT message
        if (mqttClient.connected()) {
            long now = millis();

            if (   (   (   boSwitchStatus          != oLedStripe.boGetSwitchStatus()
                        //|| u16Hue                  != oEep.u16Hue
                        //|| u8Saturation            != oEep.u8Saturation
                        //|| u8Brightness            != oLedStripe.u8GetBrightness()
                        //|| u8ColorMode             != oEep.u8ColorMode
                        //|| u8Speed                 != oEep.u8Speed
                    )
                    && (now - mqttEventTxTimer > mqttSendEventInterval))
                || (now - mqttCyclicTxTimer > mqttSendCyclicInterval)) {

                boSwitchStatus = oLedStripe.boGetSwitchStatus();
                u16Hue         = oEep.u16Hue;
                u8Saturation   = oEep.u8Saturation;
                u8Brightness   = oLedStripe.u8GetBrightness();
                u8ColorMode    = oEep.u8ColorMode;
                u8Speed        = oEep.u8Speed;

                mqttCyclicTxTimer = now; // send next cyclic message earliest 5min
                mqttEventTxTimer  = now; // send changes earliest 1sec
                vMqttTx(); // send MQTT message
            }
        }
    }
}
