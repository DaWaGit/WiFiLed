#include <ESP8266WiFi.h> // we need wifi to get internet access
#include "NtpTime.h"
#include "Utils.h"
#include "DebugLevel.h"

#define CLASS_NAME "NtpTime"
#define INTERVAL 1000 // update time every x ms

//=============================================================================
NtpTime::NtpTime(uint8_t u8NewDebugLevel) {
    u8DebugLevel = u8NewDebugLevel; // store debug level
}

//=============================================================================
void NtpTime::vInit(char *timeZone, char *ntpServer1, char *ntpServer2, char *ntpServer3, double newLongitude, double newLatitude) {
    longitude = newLongitude;
    latitude  = newLatitude;
    configTime(timeZone, ntpServer1, ntpServer2, ntpServer3); // by default, the NTP will be started after 60 secs

    char buffer[200];
    sprintf(
        buffer,
        "TimeZone:'%s' NtpServer1:'%s' NtpServer2:'%s' NtpServer3:'%s'",
        timeZone,
        ntpServer1,
        ntpServer2,
        ntpServer3
      );
    vConsole(u8DebugLevel, DEBUG_TIME_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
}

//=============================================================================
void NtpTime::vLoop() {
    static ulong ulLastInterval = 0;
    static uint16_t u16SunRise, u16SunSet; // SunRise/SunSet in hhmm
    static uint8_t u8LastDay, u8LastDst;
    static double dLastLongitude, dLastLatitude;
    time_t now;
    tm tm;

    if ((millis() - ulLastInterval) >= INTERVAL) {
        ulLastInterval = millis();

        time(&now);             // read the current time
        localtime_r(&now, &tm); // update the structure tm with the current time

        if (tm.tm_mday    != u8LastDay   || tm.tm_isdst  != u8LastDst ||  // calculate new each u8Day or boDST changed
            dLastLongitude != longitude || dLastLatitude != latitude) { // or the location changed
            u8LastDay = tm.tm_mday;
            u8LastDst = tm.tm_isdst;
            dLastLongitude = longitude;
            dLastLatitude  = latitude;
            const double h = -0.833333333333333 * DEG_TO_RAD;
            const double w = latitude * DEG_TO_RAD;
            double JD = dJulianDate(1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday);
            double T = (JD - 2451545.0) / 36525.0;
            double DK;
            double EOT = dCalculateEOT(DK, T);
            double differenceTime = 12.0 * acos((sin(h) - sin(w) * sin(DK)) / (cos(w) * cos(DK))) / PI;

            stSunRise = stGetSunTime((12.0 - differenceTime - EOT) - longitude / 15.0 + (_timezone* -1) / 3600 + tm.tm_isdst);
            stSunSet  = stGetSunTime((12.0 + differenceTime - EOT) - longitude / 15.0 + (_timezone* -1) / 3600 + tm.tm_isdst);

            u16SunRise = ((uint16_t)stSunRise.u8Hour * (uint16_t)100) + (uint16_t)stSunRise.u8Minute;
            u16SunSet  = ((uint16_t)stSunSet.u8Hour  * (uint16_t)100) + (uint16_t)stSunSet.u8Minute;
        }

        stLocal.u8Hour   = tm.tm_hour;
        stLocal.u8Minute = tm.tm_min;
        stLocal.u8Second = tm.tm_sec;
        stLocal.u8Day    = tm.tm_mday;
        stLocal.u8Month  = tm.tm_mon + 1;
        stLocal.u16Year  = tm.tm_year + 1900;
        stLocal.boDST    = tm.tm_isdst ? true : false;

        // check if sun above the horizon
        uint16_t u16CurrentTime = ((uint16_t)stLocal.u8Hour * (uint16_t)100) + (uint16_t)stLocal.u8Minute; // current time in hhmm
        if (u16CurrentTime > u16SunRise && u16CurrentTime < u16SunSet) {
            stLocal.boSunHasRisen = true;
        } else {
            stLocal.boSunHasRisen = false;
        }

        char buffer[200];
        if (u8DebugLevel & DEBUG_TIME_EVENTS) {
            sprintf(
                buffer,
                "%02d:%02d:%02d %02d.%02d.%04d / %s / SunRise: %02d:%02d SunSet: %02d.%02d / sun %s the horizon",
                stLocal.u8Hour,
                stLocal.u8Minute,
                stLocal.u8Second,
                stLocal.u8Day,
                stLocal.u8Month,
                stLocal.u16Year,
                stLocal.boDST ? "Daylight Saving Time" : "Normal Time",
                stSunRise.u8Hour,
                stSunRise.u8Minute,
                stSunSet.u8Hour,
                stSunSet.u8Minute,
                stLocal.boSunHasRisen ? "above" : "below");
            vConsole(u8DebugLevel, DEBUG_TIME_EVENTS, CLASS_NAME, __FUNCTION__, buffer);
        }
    }
}

double NtpTime::dJulianDate (int y, int m, int d) {        // Gregorianischer Kalender
  if (m <= 2) {
    m = m + 12;
    y = y - 1;
  }
  int gregorian = (y / 400) - (y / 100) + (y / 4); // Gregorianischer Kalender
  return 2400000.5 + 365.0 * y - 679004.0 + gregorian + (30.6001 * (m + 1)) + d + 12.0 / 24.0;
}

double NtpTime::dInPi(double x) {
  int n = x / TWO_PI;
  x = x - n * TWO_PI;
  if (x < 0) x += TWO_PI;
  return x;
}

double NtpTime::dCalculateEOT(double &DK, double T) {
  double RAm = 18.71506921 + 2400.0513369 * T + (2.5862e-5 - 1.72e-9 * T) * T * T;
  double M  = dInPi(TWO_PI * (0.993133 + 99.997361 * T));
  double L  = dInPi(TWO_PI * (  0.7859453 + M / TWO_PI + (6893.0 * sin(M) + 72.0 * sin(2.0 * M) + 6191.2 * T) / 1296.0e3));
  double e = DEG_TO_RAD * (23.43929111 + (-46.8150 * T - 0.00059 * T * T + 0.001813 * T * T * T) / 3600.0);    // Neigung der Erdachse
  double RA = atan(tan(L) * cos(e));
  if (RA < 0.0) RA += PI;
  if (L > PI) RA += PI;
  RA = 24.0 * RA / TWO_PI;
  DK = asin(sin(e) * sin(L));
  RAm = 24.0 * dInPi(TWO_PI * RAm / 24.0) / TWO_PI;
  double dRA = RAm - RA;
  if (dRA < -12.0) dRA += 24.0;
  if (dRA > 12.0) dRA -= 24.0;
  dRA = dRA * 1.0027379;
  return dRA ;
}

tstSunTime NtpTime::stGetSunTime(double sunTime) {
  static tstSunTime newSunTime;

  if (sunTime < 0) sunTime += 24;
  else if (sunTime >= 24) sunTime -= 24;
  newSunTime.u8Minute = 60 * (sunTime - static_cast<int>(sunTime)) + 0.5;
  newSunTime.u8Hour = sunTime;
  if (newSunTime.u8Minute >= 60) {
    newSunTime.u8Minute -= 60;
    newSunTime.u8Hour++;
  }
  else if (newSunTime.u8Minute < 0) {
    newSunTime.u8Minute += 60;
    newSunTime.u8Hour--;
    if (newSunTime.u8Hour < 0) newSunTime.u8Hour += 24;
  }
  return newSunTime;
}
