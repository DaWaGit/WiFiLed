// #############################################################################
//  Project  : WiFiLed (control RGB LED stripe with WS2812 LED's via ESP32)
//  Author   : Daniel Warnicki 22.12.2022
//  GitHubUrl: https://github.com/DaWaGit/WiFiLed.git
//  Doc      : https://github.com/DaWaGit/WiFiLed/blob/main/README.md
// #############################################################################
#define CLASS_NAME       "WiFiLed"
#define ENABLE_WLAN      1

#include "Eep.h"        // EEP interface
#include "LedStripe.h"  // LED controll
#include "Buttons.h"    // button handling
#include "PT1.h"        // PT1 damping
#if ENABLE_WLAN
    #include "Wlan.h"   // WiFi Interface
#endif
#include "Version.h"    // version definition
#include "Utils.h"      // useful utils
#include "DebugLevel.h" // debug level definiton
#include "NtpTime.h"    // NTP time

//=======================================================================
//                               Globals
//=======================================================================
Eep oEep(DEBUG_LEVEL);             // create the Eep object
LedStripe oLedStripe(DEBUG_LEVEL); // create the LedStrip object
Buttons oButtons(DEBUG_LEVEL);     // create the Button object
#if ENABLE_WLAN
    Wlan oWlan(DEBUG_LEVEL);       // create Wlan object
#endif
NtpTime oNtpTime(DEBUG_LEVEL);     // create an NTP time object

//=======================================================================
//                               Setup
//=======================================================================
void setup() {
    // init serial monitor
    Serial.begin(115200);
    Serial.println("");
    vPrintChipInfo();
    Serial.setDebugOutput(DEBUG_LEVEL & DEBUG_GLOBAL_OUTPUT ? true : false);

    oEep.vInit(&oNtpTime);              // download all EEP values
    oLedStripe.vInit(&oEep, &oNtpTime); // init LED strip
    oButtons.vInit(&oLedStripe, &oEep, &oNtpTime); // init Buttons
#if ENABLE_WLAN
    oWlan.vInit(&oButtons, &oLedStripe, &oEep, &oNtpTime); // init Wlan+Webserver
#endif
    oNtpTime.vSetLedStripe(&oLedStripe);

    oNtpTime.vInit(
        oEep.acTimeZone,   // TimeZone see: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
        oEep.acNtpServer1, // NTP server 1 e.g. "ptbtime1.ptb.de"
        oEep.acNtpServer2, // NTP server 2 e.g. "ptbtime2.ptb.de"
        "ptbtime3.ptb.de", // NTP server 3 e.g. "ptbtime3.ptb.de"
        oEep.dLatitude,    // latitude
        oEep.dLongitude    // longitude
    );
}
//=======================================================================
//                               MAIN LOOP
//=======================================================================
void loop() {
    oButtons.vLoop();   // detect and handle Button events
    oLedStripe.vLoop(); // damp stripe changes
#if ENABLE_WLAN
    oWlan.vLoop();      // check Wlan status, reconnect
#endif
    oNtpTime.vLoop();
}

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
    Serial.print  ("Board  : "); Serial.println(ARDUINO_BOARD);
    Serial.print  ("Version: V"); Serial.println(VERSION);
    Serial.println("Author : DaWa (Daniel Warnicki)");
    Serial.print(  "Date   : "); Serial.print( __DATE__); Serial.print(" "); Serial.println(__TIME__ );
    Serial.println("----------------------------------------------------------------------------");
    Serial.print("ResetReason      : "); Serial.println(ESP.getResetReason()); // returns a String containing the last reset reason in human readable format.
    Serial.print("FreeHeap         : "); Serial.println(ESP.getFreeHeap()); // returns the free heap size.
    Serial.print("HeapFragmentation: "); Serial.println(ESP.getHeapFragmentation()); // returns the fragmentation metric (0% is clean, more than ~50% is not harmless)
    Serial.print("MaxFreeBlockSize : "); Serial.println(ESP.getMaxFreeBlockSize()); // returns the maximum allocatable ram block regarding heap fragmentation
    Serial.print("ChipId           : 0x"); Serial.println(ESP.getChipId(),HEX); // returns the ESP8266 chip ID as a 32-bit integer.
    Serial.print("CoreVersion      : "); Serial.println(ESP.getCoreVersion()); // returns a String containing the core version.
    Serial.print("SdkVersion       : "); Serial.println(ESP.getSdkVersion()); // returns the SDK version as a char.
    Serial.print("CpuFreqMHz       : "); Serial.println(ESP.getCpuFreqMHz()); // returns the CPU frequency in MHz as an unsigned 8-bit integer.
    Serial.print("SketchSize       : "); Serial.println(ESP.getSketchSize()); // returns the size of the current sketch as an unsigned 32-bit integer.
    Serial.print("FreeSketchSpace  : "); Serial.println(ESP.getFreeSketchSpace()); // returns the free sketch space as an unsigned 32-bit integer.
    Serial.print("SketchMD5        : "); Serial.println(ESP.getSketchMD5()); // returns a lowercase String containing the MD5 of the current sketch.
    Serial.print("FlashChipId      : 0x"); Serial.println(ESP.getFlashChipId(),HEX); // returns the flash chip ID as a 32-bit integer.
    Serial.print("FlashChipSize    : "); Serial.println(ESP.getFlashChipSize()); // returns the flash chip size, in bytes, as seen by the SDK (may be less than actual size).
    Serial.print("FlashChipRealSize: "); Serial.println(ESP.getFlashChipRealSize()); // returns the real chip size, in bytes, based on the flash chip ID.
    Serial.print("FlashChipSpeed   : "); Serial.println(ESP.getFlashChipSpeed()); // returns the flash chip frequency, in Hz.
    Serial.print("CycleCount       : "); Serial.println(ESP.getCycleCount()); // returns the cpu instruction cycle count since start as an unsigned 32-bit. This is useful for accurate timing of very short actions like bit banging.
    Serial.print("Vcc              : "); Serial.println(ESP.getVcc()); //
    Serial.println("============================================================================");
}
