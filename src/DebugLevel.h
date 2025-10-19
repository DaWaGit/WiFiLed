#ifndef DebugLevel_h
#define DebugLevel_h

//                              +-------- global debug output
//                              |+------- time events
//                              ||+------ WebServer events
//                              |||+----- WLAN events
//                              ||||+---- EEP events
//                              |||||+--- LED details
//                              ||||||+-- LED events
//                              |||||||+- Button events
//                              ||||||||
#define DEBUG_LEVEL            B10100000 // current enabled debug level
#define DEBUG_GLOBAL_OUTPUT    B10000000 // enable serial debug output
#define DEBUG_TIME_EVENTS      B01000000 // enable NTP time events
#define DEBUG_WEBSERVER_EVENTS B00100000 // show all web server events
#define DEBUG_WLAN_EVENTS      B00010000 // show all WiFi events
#define DEBUG_EEP_EVENTS       B00001000 // show all EEP events to read/write values
#define DEBUG_LED_DETAILS      B00000100 // show LED stripe details
#define DEBUG_LED_EVENTS       B00000010 // show LED stipe events
#define DEBUG_BUTTON_EVENTS    B00000001 // show all detected Button events

#endif
