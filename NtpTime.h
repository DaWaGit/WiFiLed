#ifndef ntpTime_h
#define ntpTime_h

#include <time.h>

struct tstSunTime {
    uint8_t u8Hour;   // hour
    uint8_t u8Minute; // minute
};

struct tstLocalTime {
    uint16_t u16Year;   // year
    uint8_t u8Month;    // month
    uint8_t u8Day;      // day
    uint8_t u8Hour;     // hour
    uint8_t u8Minute;   // minute
    uint8_t u8Second;   // second
    bool boDST;         // 1:daylight saving time / 0:standard time
    bool boSunHasRisen; // 1:sun above the horizon / 0:sun below the horizon
};

class NtpTime {
    public:
        NtpTime(uint8_t u8NewDebugLevel);                           // constructor
        void vInit(char *, char *, char *, char *, double, double); // initialize the NTP server and set your location
        void vLoop();                                               // call thi in the main loop to refresh the stLocal, SunRise, SunSet automatically
        tstLocalTime stLocal; // local time
        tstSunTime stSunRise; // time SunRise
        tstSunTime stSunSet;  // time SunSet

    private:
        double dJulianDate(int, int, int);
        double dInPi(double);
        double dCalculateEOT(double &, double);
        tstSunTime stGetSunTime(double);
        uint8_t u8DebugLevel = 0;
        double longitude;
        double latitude;
};
#endif
