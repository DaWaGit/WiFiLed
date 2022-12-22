#include "PT1.h"

//=======================================================================
PT1::PT1(unsigned long ulNewPeriode, unsigned long ulNewTau) {
    ulPeriode = ulNewPeriode;
    ulTau     = ulNewTau;
}

//=======================================================================
void PT1::vInitDampVal(float fNewDampVal) {
    fDampedValue = fNewDampVal;
}

//=======================================================================
float PT1::fGetDampedVal(int iNewVal) {
    unsigned long FF = ulTau / ulPeriode;
    if (millis() - ulLastRun < ulPeriode)
        return fDampedValue;
    ulLastRun = millis();
    fDampedValue = (((fDampedValue * FF) + iNewVal) / (FF + 1));

    return fDampedValue;
}