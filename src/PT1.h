#ifndef PT1_h
#define PT1_h
#include <Arduino.h>

class PT1 {
    public:
        PT1(unsigned long, unsigned long);
        void vInitDampVal(float);
        float fGetDampedVal(int);

    private:
        float         fDampedValue;
        unsigned long ulLastRun;
        unsigned long ulPeriode;
        unsigned long ulTau;
};

#endif
