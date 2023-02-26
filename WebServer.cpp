#include <string.h>
#include "WebServer.h"
#include "Version.h"
#include "DebugLevel.h"

#define CLASS_NAME "WebServer"

using namespace std::placeholders;
volatile uint8_t u8DebugLevel = 0;

//=======================================================================
WebServer::WebServer(int iWebUrlPort, int iNewWebSocketPort) {
    // Create AsyncWebServer object on port 80
    pWebServer     = new AsyncWebServer(iWebUrlPort);
    iWebSocketPort = iNewWebSocketPort;
    pWebSocket     = new WebSocketsServer(iWebSocketPort);
}

//=======================================================================
void WebServer::vInit(class Buttons *pNewButtons, class LedStripe *pNewLedStripe, class Eep *pNewEep, class NtpTime *pNewNtpTime, uint8_t u8NewDebugLevel)
{

    pEep         = pNewEep;         // eep class
    pLedStripe   = pNewLedStripe;   // store LedStripe
    pButtons     = pNewButtons;     // store Buttons
    pNtpTime     = pNewNtpTime;     // store NTP time
    u8DebugLevel = u8NewDebugLevel; // store debug level

    // Initialize SPIFFS
    if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
        Serial.printf("[%s::%s] SPIFFS-mount : ", CLASS_NAME, __FUNCTION__);
    }
    if (!SPIFFS.begin()) {
        if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
            Serial.println("ERROR detected");
        }
        return;
    } else {
        if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
            Serial.println("successfully");
        }
    }

    // Route for root / web page
    static AwsTemplateProcessor processorFunc = std::bind(&WebServer::sTemplateProcessor, this, _1);
    pWebServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, processorFunc);
    });
    pWebServer->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    });
    pWebServer->on("/sun.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/sun.svg", "image/svg+xml");
    });
    pWebServer->on("/moon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/moon.svg", "image/svg+xml");
    });
    pWebServer->on("/jquery-3.4.1.slim.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/jquery-3.4.1.slim.min.js", "text/javascript");
    });
    pWebServer->on("/jquery.wheelcolorpicker.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/jquery.wheelcolorpicker.min.js", "text/javascript");
    });
    pWebServer->on("/ToggleSwitch.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/ToggleSwitch.css", "text/css");
    });
    pWebServer->on("/wheelcolorpicker.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/wheelcolorpicker.css", "text/css");
    });

    // Start pWebServer
    pWebServer->begin();

    // Start WebSocket pWebServer and assign callback
    pWebSocket->begin();
    pWebSocket->onEvent(std::bind(&WebServer::vWebSocketEvent, this, _1, _2, _3, _4));

    //vSendInitValues(-1, true);            // update values for every client
}

//=======================================================================
void WebServer::vSetIp(IPAddress newLocalIP) {
    localIP = newLocalIP; // store the current IP
}

//=======================================================================
void WebServer::vLoop() {
    // loop to handle WebSocket data
    pWebSocket->loop();
}

    //=======================================================================
String WebServer::sTemplateProcessor(const String &var) {
    // Per default TEMPLATE_PLACEHOLDER is defined as '%'.
    // But this does not wor really good with HTML.
    // The definition of TEMPLATE_PLACEHOLDER should be replaces to '`'
    // you can find the definition in:
    // ..Arduino\libraries\ESPAsyncWebServer\WebResponseImpl.h
    char buffer[51]; buffer[0] = 0;
    if (var == "id") {
        snprintf(buffer, 50, "%08X", (uint32_t)ESP.getChipId());
    } else if (var == "ver") {
        snprintf(buffer, 50, "%s", VERSION);
    } else if (var == "ssid") {
        pEep->vGetWifiSsid(buffer);
    } else if (var == "pwd") {
        pEep->vGetWifiPwd(buffer);
    } else if (var == "ledCount") {
        snprintf(buffer, 50, "%d", pEep->u16LedCount);
    } else if (var == "bMin") {
        snprintf(buffer, 50, "%d", pEep->u8BrightnessMin);
    } else if (var == "bMax") {
        snprintf(buffer, 50, "%d", pEep->u8BrightnessMax);
    } else if (var == "offDelay") {
        snprintf(buffer, 50, "%d", pEep->u8MotionOffDelay + EepMotionOffDelayMin);
    } else if (var == "colorMode") {
        snprintf(buffer, 50, "%d", pEep->u8ColorMode);
    } else if (var == "speed") {
        snprintf(buffer, 50, "%d", pEep->u8Speed);
    } else if (var == "wsUrl") {
        snprintf(buffer, 50, "ws://%d.%d.%d.%d:%d/", localIP[0], localIP[1], localIP[2], localIP[3], iWebSocketPort);
    }
    if (buffer[0] != 0) {
        if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
            Serial.printf("[%s::%s] replace `%s` to %s\n", CLASS_NAME, __FUNCTION__, var, buffer);
        }
        return buffer; // return the new value for the website
    } else {
        return String();
    }
}

//=======================================================================
// Callback: receiving any WebSocket message
void WebServer::vWebSocketEvent(uint8_t clientNumber,
                                WStype_t type,
                                uint8_t * payload,
                                size_t length)
{
    IPAddress ip = pWebSocket->remoteIP(clientNumber);
    if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
        Serial.printf("[%s::%s] client[%u]->", CLASS_NAME, __FUNCTION__, clientNumber);
        Serial.print(ip.toString());
    }
    switch (type) {                         // Figure out the type of WebSocket event
        case WStype_DISCONNECTED: // Client has disconnected
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" DISCONNECTED");
            }
            break;
        case WStype_CONNECTED: // New client has connected
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" CONNECTED");
            }
            break;
        case WStype_TEXT: // Handle text messages from client
            // Print out raw message
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.printf(" TEXT: %s\n", payload);
            }
            if (strcmp((char *)payload, "on") == 0) {
                // turn stripe on via web page
                pLedStripe->vTurn(true, false);
                vSendStripeStatus(clientNumber, true); // update values for every client expect himself
            } else if (strcmp((char *)payload, "off") == 0) {
                // turn stripe off via web page
                pLedStripe->vTurn(false, false);
                vSendStripeStatus(clientNumber, true); // update values for every client expect himself
            } else if (strstr((char *)payload, "set=")) {
                // change hue, saturation, brightness via web page
                String sPayload = String((char *)payload);

                int start = sPayload.indexOf("h:") + 2;
                int end   = sPayload.indexOf("s:");
                uint16_t u16NewHue = (uint16_t)sPayload.substring(start, end).toInt();

                start = sPayload.indexOf("s:") + 2;
                end   = sPayload.indexOf("b:");
                uint8_t u8NewSaturation = (uint8_t)sPayload.substring(start, end).toInt();

                start = sPayload.indexOf("b:") + 2;
                end   = sPayload.length();
                uint8_t u8NewBrightness = (uint8_t)sPayload.substring(start, end).toInt();

                pLedStripe->vSetValues(u16NewHue, u8NewSaturation, u8NewBrightness);
                pLedStripe->vSetColor(clientNumber);
            } else if (strstr((char *)payload, "ledCount")) {
                // number of LEDs changed via web page
                String sPayload = String((char *)payload);
                int start = sPayload.indexOf("ledCount:") + 9;
                int end = sPayload.indexOf("bMin:");
                uint16_t u16NewLedCount = (uint16_t)sPayload.substring(start, end).toInt(); // get the new value
                pEep->vSetLedCount(u16NewLedCount, true); // store the new value in EEP

                start = sPayload.indexOf("bMin:") + 5;
                end   = sPayload.indexOf("bMax:");
                pEep->vSetBrightnessMin((uint8_t)sPayload.substring(start, end).toInt(), true); // store the new value in EEP

                start = sPayload.indexOf("bMax:") + 5;
                end   = sPayload.indexOf("offDelay:");
                pEep->vSetBrightnessMax((uint8_t)sPayload.substring(start, end).toInt(), true); // store the new value in EEP

                start = sPayload.indexOf("offDelay:") + 9;
                end   = sPayload.length();
                pEep->vSetMotionOffDelay((uint8_t)sPayload.substring(start, end).toInt(), true); // store the new value in EEP

                start = sPayload.indexOf("bDay:") + 5;
                end = sPayload.length();
                pEep->vSetBrightnessDay((uint8_t)sPayload.substring(start, end).toInt(), true); // store the new value in EEP

                start = sPayload.indexOf("bNight:") + 7;
                end = sPayload.length();
                pEep->vSetBrightnessNight((uint8_t)sPayload.substring(start, end).toInt(), true); // store the new value in EEP

                bool boSwitchStatus = pLedStripe->boGetSwitchStatus(); // get the current switch status
                if (boSwitchStatus) pLedStripe->vTurn(false, true);    // set the last switch status again
                pLedStripe->vInit(pEep, pNtpTime);                     // reinitialize the the new LedCount
                if (boSwitchStatus) pLedStripe->vTurn(boSwitchStatus, true); // set the last switch status again
                vSendStripeStatus(clientNumber, true); // update values for every client expect himself
            } else if (strstr((char *)payload, "Ssid")) {
                // WiFi config changed via web page
                String sPayload = String((char *)payload);

                char acWifiSsid[EepStringSize];
                char acWifiPwd[EepStringSize];

                int start = sPayload.indexOf("Ssid:") + 5;
                int end   = sPayload.indexOf("Pwd:");
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    acWifiSsid[i]   = sPayload[start+i];
                    acWifiSsid[i+1] = 0;
                }

                start = sPayload.indexOf("Pwd:") + 4;
                end   = sPayload.length();
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    acWifiPwd[i]   = sPayload[start+i];
                    acWifiPwd[i+1] = 0;
                }

                pEep->vSetWifiSsidPwd(acWifiSsid, acWifiPwd, true);
                pEep->vSetWiFiMode(0, true); // on next Reset start SSID mode
                ESP.restart(); // reset
            } else if (strstr((char *)payload, "factoryReset")) {
                pEep->vFactoryReset();
            } else if (strstr((char *)payload, "calib")) {
                // Calibration button pressed/released via web page
                String sPayload = String((char *)payload);
                int start = sPayload.indexOf("calib:") + 6;
                int end         = sPayload.length();
                if ((uint8_t)sPayload.substring(start, end).toInt()) {
                    pButtons->vSet(nCalibrationButton_Pressed);
                } else {
                    pButtons->vSet(nCalibrationButton_Released);
                }
            } else if (strstr((char *)payload, "colorMode")) {
                // color mode changed via web page
                String sPayload = String((char *)payload);
                int start       = sPayload.indexOf("colorMode:") + 10;
                int end         = sPayload.length();
                pLedStripe->vSetColorMode((tColorMode)sPayload.substring(start, end).toInt(), clientNumber);
                vSendColorMode(clientNumber, true);
            } else if (strstr((char *)payload, "speed")) {
                // speed changed via web page
                String sPayload = String((char *)payload);
                int start       = sPayload.indexOf("speed:") + 6;
                int end         = sPayload.length();
                pEep->vSetSpeed((uint8_t)(sPayload.substring(start, end).toInt()),true);
                vSendColorMode(clientNumber, true);
            } else if (strstr((char *)payload, "dSens")) {
                // speed changed via web page
                String sPayload = String((char *)payload);
                int start       = sPayload.indexOf("dSens:") + 6;
                int end         = sPayload.length();
                pEep->vSetDistanceSensorEnabled((uint8_t)(sPayload.substring(start, end).toInt()), true);
                vSendDistanceSensorEnabled(clientNumber, true);
            } else if (strstr((char *)payload, "mSens")) {
                // speed changed via web page
                String sPayload = String((char *)payload);
                int start       = sPayload.indexOf("mSens:") + 6;
                int end         = sPayload.length();
                pEep->vSetMotionSensorEnabled((uint8_t)(sPayload.substring(start, end).toInt()), true);
                vSendMotionSensorEnabled(clientNumber, true);
            } else if (strstr((char *)payload, "TimeZoneName:")) {
                // change hue, saturation, brightness via web page
                String sPayload = String((char *)payload);

                int start = sPayload.indexOf("TimeZoneName:") + 13;
                int end = sPayload.indexOf("TimeZone:");
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    pEep->acTimeZoneName[i]   = sPayload[start+i];
                    pEep->acTimeZoneName[i+1] = 0;
                }

                start = sPayload.indexOf("TimeZone:") + 9;
                end   = sPayload.indexOf("NTPserver1:");
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    pEep->acTimeZone[i]   = sPayload[start+i];
                    pEep->acTimeZone[i+1] = 0;
                }

                start = sPayload.indexOf("NTPserver1:") + 11;
                end   = sPayload.indexOf("NTPserver2:");
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    pEep->acNtpServer1[i]   = sPayload[start+i];
                    pEep->acNtpServer1[i+1] = 0;
                }

                start = sPayload.indexOf("NTPserver2:") + 11;
                end   = sPayload.indexOf("Latitude:");
                for (int i = 0; (i < EepStringSize-1) && (i < (end-start)); i++) {
                    pEep->acNtpServer2[i]   = sPayload[start+i];
                    pEep->acNtpServer2[i+1] = 0;
                }

                start = sPayload.indexOf("Latitude:") + 9;
                end   = sPayload.indexOf("Longitude:");
                double dLatitude = (double)sPayload.substring(start, end).toDouble();

                start = sPayload.indexOf("Longitude:") + 10;
                end   = sPayload.length();
                double dLongitude = (double)sPayload.substring(start, end).toDouble();

                pEep->vSetNtp(pEep->acTimeZoneName, pEep->acTimeZone, pEep->acNtpServer1, pEep->acNtpServer2, true); // store time zone and NTM server
                pEep->vSetLatitude(dLatitude, true);    // store latitude
                pEep->vSetLongitude(dLongitude, true);  // store longitude
                vSendTimeSetup(clientNumber, true);     // update time setup for every client
            } else {
                // Message not recognized
            }
            break;
        case WStype_BIN:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" BIN");
            }
            break;
        case WStype_ERROR:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" ERROR");
            }
            break;
        case WStype_FRAGMENT_TEXT_START:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" FRAGMENT_TEXT_START");
            }
            break;
        case WStype_FRAGMENT_BIN_START:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" FRAGMENT_BIN_START");
            }
            break;
        case WStype_FRAGMENT:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" FRAGMENT");
            }
            break;
        case WStype_FRAGMENT_FIN:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" FRAGMENT_FIN");
            }
            break;
        case WStype_PING:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" PING");
            }
            break;
        case WStype_PONG:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" PONG");
            }
            vSendInitValues(clientNumber, false);
            break;
        default:
            if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
                Serial.println(" UNKNOWN");
            }
            break;
    }
}

//=======================================================================
// send the current u8DistanceSensorEnabled mode to all active clients
void WebServer::vSendDistanceSensorEnabled(int clientNumber, bool boToAllClients) {
    char msg_buf[100];

    // get the current stripe status
    sprintf(msg_buf, "dSens:%d",
            pEep->u8DistanceSensorEnabled);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send the current u8MotionSensorEnabled mode to all active clients
void WebServer::vSendMotionSensorEnabled(int clientNumber, bool boToAllClients) {
    char msg_buf[100];

    // get the current stripe status
    sprintf(msg_buf, "mSens:%d",
            pEep->u8MotionSensorEnabled);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send the current color mode to all active clients
void WebServer::vSendColorMode(int clientNumber, bool boToAllClients) {
    char msg_buf[100];

    // get the current stripe status
    sprintf(msg_buf, "colorMode:%dspeed:%d",
            pEep->u8ColorMode,
            pEep->u8Speed);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send the current time setup to all active clients
void WebServer::vSendTimeSetup(int clientNumber, bool boToAllClients) {
    char msg_buf[250];

    // get the current stripe status
    sprintf(msg_buf, "TimeZoneName:%sTimeZone:%sNTPserver1:%sNTPserver2:%sLatitude:%fLongitude:%f",
            pEep->acTimeZoneName,
            pEep->acTimeZone,
            pEep->acNtpServer1,
            pEep->acNtpServer2,
            pEep->dLatitude,
            pEep->dLongitude);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send the current sun data to all active clients
void WebServer::vSendSunData(int clientNumber, bool boToAllClients) {
    char msg_buf[250];

    // get the current stripe status
    sprintf(msg_buf, "sunrise:%02d:%02dsunset:%02d:%02d",
            pNtpTime->stSunRise.u8Hour,
            pNtpTime->stSunRise.u8Minute,
            pNtpTime->stSunSet.u8Hour,
            pNtpTime->stSunSet.u8Minute);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send the current stripe status to all active clients
void WebServer::vSendStripeStatus(int clientNumber, bool boToAllClients) {
    char msg_buf[100];

    // get the current stripe status
    sprintf(msg_buf, "sw:%dh:%ds:%db:%dbDay:%dbNight:%dDay:%d",
            pLedStripe->boGetSwitchStatus(),
            pEep->u16Hue,
            pEep->u8Saturation,
            pNtpTime->stLocal.boSunHasRisen ? pEep->u8BrightnessDay : pEep->u8BrightnessNight,
            pEep->u8BrightnessDay,
            pEep->u8BrightnessNight,
            pNtpTime->stLocal.boSunHasRisen);
    if (boToAllClients) {
        // send to all clients expect the selected one
        vSendBufferToAllClients(msg_buf, clientNumber);
    } else {
        // send only to the selected client
        vSendBufferToOneClient(msg_buf, clientNumber);
    }
}

//=======================================================================
// send a buffer to all active clients expect the selected clientNumber
void WebServer::vSendBufferToAllClients(char *msg_buf,int clientNumber) {
    for (int clientIndex = 0; clientIndex < pWebSocket->connectedClients(false); clientIndex++) {
        // loop over all active clients
        if (   (clientNumber < 0)               // should be send to all clients
            || (clientNumber != clientIndex)) { // or should not send to himself
            vSendBufferToOneClient(msg_buf, clientIndex);
        }
    }
}

//=======================================================================
// send the current stripe status to all active clients
void WebServer::vSendBufferToOneClient(char *msg_buf,int clientNumber) {
    if (clientNumber >= 0) {
        if (u8DebugLevel & DEBUG_WEBSERVER_EVENTS) {
            Serial.printf("[%s::%s] client[%u]->", CLASS_NAME, __FUNCTION__, clientNumber);
            Serial.print(pWebSocket->remoteIP(clientNumber).toString()); // print client IP
            Serial.printf(" Tx : %s\n", msg_buf);                        // print message
        }
        pWebSocket->sendTXT(clientNumber, msg_buf);
    }
}

//=======================================================================
// send init values to the selected clients
void WebServer::vSendInitValues(int clientNumber, bool boToAllClients) {
    vSendStripeStatus(clientNumber, boToAllClients);          // update values for every client
    vSendColorMode(clientNumber, boToAllClients);             // update colorMode for every client
    vSendDistanceSensorEnabled(clientNumber, boToAllClients); // update sensor usage for every client
    vSendMotionSensorEnabled(clientNumber, boToAllClients);   // update sensor usage for every client
    vSendTimeSetup(clientNumber, boToAllClients);             // update time setup for every client
    vSendSunData(clientNumber, boToAllClients);               // update sun data for every client
}
