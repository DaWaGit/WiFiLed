#define ENABLE_WEBSERVER 1

#include "Wlan.h"
#include "Utils.h"
#include "DebugLevel.h"
#if ENABLE_WEBSERVER
    #include "WebServer.h"
#endif

#define CLASS_NAME              "Wlan"
#define LED_BLINK_INTERVAL_SSID 100   // blink interval for WiFi SSI Mode [ms]
#define LED_BLINK_INTERVAL_AP   1000  // blink interval for WiFi AP Mode [ms]

#define CONNECTION_TIMEOUT_SSID 60000*3 // timeout for initially SSID connection [ms]

class Eep *pEep;
char acWifiSsid[EepSizeWifiSsid];
char acWifiPwd[EepSizeWifiPwd];

volatile ulong ulWiFiLastBlinkInterval = 0;
volatile ulong ulSSIDinitLinkTimeout   = 0;
volatile uint8_t u8WiFiDebugLevel      = 0;
volatile bool boWiFiLedStatus          = false;
volatile bool boSsidConnected          = false;
volatile bool boSSIDeverConnected      = false;
WiFiEventHandler Wlan_GotIpEvent, Wlan_DisconnectedEvent;

#if ENABLE_WEBSERVER
class LedStripe *pLedStripe;
class Buttons *pButtons;
WebServer oWebServer(80, 1337); // create WebServer object
#endif

//=============================================================================
Wlan::Wlan(uint8_t u8NewDebugLevel) {
    u8WiFiDebugLevel = u8NewDebugLevel;
}

//=============================================================================
IPAddress Wlan::localIP() {
    return WiFi.localIP();
}

//=============================================================================
void Wlan::vInit(class Buttons *pNewButtons, class LedStripe *pNewLedStripe, class Eep *pNewEep, class NtpTime *pNewNtpTime) {

    pEep = pNewEep;
    pEep->vGetWifiSsid(acWifiSsid);
    pEep->vGetWifiPwd(acWifiPwd);

    pButtons = pNewButtons;

#if ENABLE_WEBSERVER
        pLedStripe = pNewLedStripe;
        oWebServer.vInit(pButtons, pLedStripe, pEep, pNewNtpTime, u8WiFiDebugLevel); // init WebServer
        //pNewNtpTime->vSetWebServer(&oWebServer);
#endif

    pinMode(LED_BUILTIN, OUTPUT); // use build in LED to show the WiFi status
    ulWiFiLastBlinkInterval = millis();
    digitalWrite(LED_BUILTIN, !boWiFiLedStatus);

    // get length of defined SSID in EEP
    int iSSIDlength = 0;
    for (int i = 0; i < EepSizeWifiSsid-1; i++) {
        if (acWifiSsid[i] == 0) {
            iSSIDlength = i;
            i = EepSizeWifiSsid;
        }
    }

    // start WiFi
    if ((iSSIDlength > 0) && (!pEep->u8WiFiMode)) {
        // start WiFi SSID mode
        ulSSIDinitLinkTimeout = millis();
        vInitWiFiMode(false);
    } else {
        // start WiFi AP mode
        vInitWiFiMode(true);
    }
}

//=============================================================================
void Wlan::vInitWiFiMode(bool boNewApMode) {
    // create the device name (DEVICE_NAME+ChipId)
    char acDeviceName[25];
    snprintf(acDeviceName, 25, "%s-%08X", DEVICE_NAME, (uint32_t)ESP.getChipId());

    if (boNewApMode) {
        // create an AccessPoint without password, see: https://techtutorialsx.com/2021/01/04/esp32-soft-ap-and-station-modes/
        if (u8WiFiDebugLevel & DEBUG_WLAN_EVENTS) {
            Serial.printf("[%s::%s] Status : WiFi started in AP mode\n", CLASS_NAME, __FUNCTION__);
        }
        WiFi.softAP(acDeviceName); // see: https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html#wifiap
        Serial.printf("[%s::%s] SSID       : ", CLASS_NAME, "AccessPoint");
        Serial.println(WiFi.softAPSSID());                                                                              // AP Name
        Serial.printf("[%s::%s] IpAddress  : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPIP());            // get the AP IPv4 address.
        //Serial.printf("[%s::%s] Hostname   : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPgetHostname());   // hostname
        Serial.printf("[%s::%s] MacAdr     : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPmacAddress());    // mac adr
        Serial.printf("[%s::%s] StationNum : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPgetStationNum()); // number of clients connected to the AP.
        //Serial.printf("[%s::%s] BroadcastIP: ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPBroadcastIP());   // get the AP IPv4 broadcast address.
        //Serial.printf("[%s::%s] NetworkID  : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPNetworkID());     // network ID.
        //Serial.printf("[%s::%s] SubnetCIDR : ", CLASS_NAME, "AccessPoint"); Serial.println(WiFi.softAPSubnetCIDR());    // subnet CIDR.
#if ENABLE_WEBSERVER
        oWebServer.vSetIp(WiFi.softAPIP()); // send IP to WebServer
        pLedStripe->vSetWebServer(&oWebServer);
        pButtons->vSetWebServer(&oWebServer);
#endif
    } else {
        // connect to SSID
        WiFi.persistent(false);      // do not store network config in flash
        WiFi.setAutoConnect(false);  // do not connect on power on to the last used access point
        WiFi.mode(WIFI_STA);         // WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(false);
        WiFi.hostname(acDeviceName);

        //.........................................................................
        Wlan_GotIpEvent = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
            boSsidConnected     = true;
            boSSIDeverConnected = true;
            digitalWrite(LED_BUILTIN, !boSsidConnected);
            pEep->vSetWiFiMode(0, true); // after Reset start SSID mode
            if (u8WiFiDebugLevel & DEBUG_WLAN_EVENTS) {
                Serial.printf("[%s::%s] Status    : ", CLASS_NAME, "onStationModeGotIP"); Serial.println("connected");
                Serial.printf("[%s::%s] HostName  : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.hostname()); //  DHCP hostname
                Serial.printf("[%s::%s] LocalUrl  : ", CLASS_NAME, "onStationModeGotIP"); Serial.print("http://"); Serial.print(WiFi.hostname()); Serial.println(".fritz.box");
                Serial.printf("[%s::%s] SSID      : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.SSID());     // name of Wi-Fi network
                Serial.printf("[%s::%s] BSSID     : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.BSSIDstr()); // mac address of the access point
                Serial.printf("[%s::%s] SignalStr : ", CLASS_NAME, "onStationModeGotIP"); Serial.print(WiFi.RSSI()); Serial.println(" dBm"); // signal strength of Wi-Fi network
                Serial.printf("[%s::%s] Mode      : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.getMode());   // current Wi-Fi mode
                Serial.printf("[%s::%s] IpAddress : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.localIP());   // IP address of ESP station’s interface
                Serial.printf("[%s::%s] MacAddress: ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.macAddress());// MAC address of the ESP station’s interface
                Serial.printf("[%s::%s] SubnetMask: ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.subnetMask());// subnet mask of the station’s interface
                Serial.printf("[%s::%s] GatewayIP : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.gatewayIP()); // IP address of the gateway
                Serial.printf("[%s::%s] DnsIP 1/2 : ", CLASS_NAME, "onStationModeGotIP"); Serial.print(WiFi.dnsIP());Serial.print(" / "); Serial.println(WiFi.dnsIP(1)); // IP addresses of Domain Name Servers
                Serial.printf("[%s::%s] AutoCon.  : ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.getAutoConnect()       ? "enabled" : "disabled"); // automatically connect to last used access point on power on
                Serial.printf("[%s::%s] AutoRecon.: ", CLASS_NAME, "onStationModeGotIP"); Serial.println(WiFi.setAutoReconnect(true) ? "enabled" : "disabled"); // reconnect to an access point in case it is disconnected
            }
#if ENABLE_WEBSERVER
            oWebServer.vSetIp(WiFi.localIP()); // send IP to WebServer
            pLedStripe->vSetWebServer(&oWebServer);
            pButtons->vSetWebServer(&oWebServer);
#endif
        });

        //.........................................................................
        Wlan_DisconnectedEvent = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
            boWiFiLedStatus         = false;
            ulWiFiLastBlinkInterval = millis();
            digitalWrite(LED_BUILTIN, !boWiFiLedStatus);
            if (!boSsidConnected) {
                // connection was not estableshed
                if (u8WiFiDebugLevel & DEBUG_WLAN_EVENTS) {
                    Serial.printf("[%s::%s] Status : try to connect to ", CLASS_NAME, "disconnectedEvent"); Serial.println(WiFi.SSID());
                }
                WiFi.begin(acWifiSsid, acWifiPwd);
            } else {
                // connection was estableshed
                boSsidConnected = false;
                if (u8WiFiDebugLevel & DEBUG_WLAN_EVENTS) {
                    Serial.printf("[%s::%s] Status : try to reconnect to ", CLASS_NAME, "disconnectedEvent"); Serial.println(WiFi.SSID());
                }
            }
        });
        // start WiFi connection
        if (u8WiFiDebugLevel & DEBUG_WLAN_EVENTS) {
            Serial.printf("[%s::%s] Status : WiFi started in SSID mode\n", CLASS_NAME, __FUNCTION__);
        }
        WiFi.begin(acWifiSsid, acWifiPwd);
    }
    boApMode = boNewApMode;
}

//=============================================================================
void Wlan::vLoop(){

    if (boApMode) {
        // AP mode active
#if ENABLE_WEBSERVER
        oWebServer.vLoop(); // loop to handle WebSocket data
#endif
        // SSID not connected
        if ((millis() - ulWiFiLastBlinkInterval) > LED_BLINK_INTERVAL_AP) {
            // status LED blink every 200ms
            ulWiFiLastBlinkInterval = millis();
            boWiFiLedStatus         = !boWiFiLedStatus;
            digitalWrite(LED_BUILTIN, !boWiFiLedStatus);
        }
    } else {
        // SSID mode active
        if (boSsidConnected) {
            // WLAN connected
#if ENABLE_WEBSERVER
            oWebServer.vLoop(); // loop to handle WebSocket data
#endif
        } else {
            // SSID not connected
            if ((millis() - ulWiFiLastBlinkInterval) > LED_BLINK_INTERVAL_SSID) {
                // status LED blink every 200ms
                ulWiFiLastBlinkInterval = millis();
                boWiFiLedStatus         = !boWiFiLedStatus;
                digitalWrite(LED_BUILTIN, !boWiFiLedStatus);
            }
            if (!boSSIDeverConnected) {
                // SSID never connected
                if ((millis() - ulSSIDinitLinkTimeout) > CONNECTION_TIMEOUT_SSID) {
                    // start WiFi AP mode
                    pEep->vSetWiFiMode(1, true); // after Reset start AP mode
                    ESP.restart();         // reset
                }
            }
        }
    }
}



