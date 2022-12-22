#ifndef DebugLevel_h
#define DebugLevel_h

//                              +-------- global debug output
//                              | +------ WebServer events
//                              | |+----- WLAN events
//                              | ||+---- EEP events
//                              | |||+--- LED details
//                              | ||||+-- LED events
//                              | |||||+- Button events
//                              | ||||||
#define DEBUG_LEVEL            B10111011 // current enabled debug level
#define DEBUG_GLOBAL_OUTPUT    B10000000
#define DEBUG_WEBSERVER_EVENTS B00100000
#define DEBUG_WLAN_EVENTS      B00010000
#define DEBUG_EEP_EVENTS       B00001000
#define DEBUG_LED_DETAILS      B00000100
#define DEBUG_LED_EVENTS       B00000010
#define DEBUG_BUTTON_EVENTS    B00000001

#endif
