#ifndef CWDateTimeCf_h
#define CWDateTimeCf_h

#include <Arduino.h>
#include "RTClib.h"

class CWDateTime
{
  private:
    RTC_PCF8563 rtc;

  public:
    void begin();

    char *getHour(const char *format);
    char *getMinute(const char *format);
    char *getSecond(const char *format);
    int getHour();
    int getMinute();
    int getSecond();

    int getWeekday();
    bool isAM();

    int getDay();
    int getMonth();

};
#endif
