#include "Utils.h"

//=======================================================================
void vConsole(
    uint8_t debugLevel,
    uint8_t debugFilter,
    const char *pClassName,
    const char *pFunction,
    char *pTxt)
{
    if (debugLevel & debugFilter) {
        Serial.printf("[%s::%s] %s\n", pClassName, pFunction, pTxt);
    }
}
