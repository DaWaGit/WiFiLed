#include "Utils.h"

//=======================================================================
void vConsole(
    uint8_t u8DebugLevel,
    uint8_t u8DebugFilter,
    const char *pClassName,
    const char *pFunction,
    char *pTxt)
{
    if (u8DebugLevel & u8DebugFilter) {
        Serial.printf("[%s::%s] %s\n", pClassName, pFunction, pTxt);
    }
}
