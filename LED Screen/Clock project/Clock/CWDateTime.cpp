#include "CWDateTime.h"
#include <Arduino.h>
#include "pin_config.h"

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//**********************************************************************************************
void CWDateTime::begin()
{
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, please set the time!");
  }
}
//**********************************************************************************************
char *CWDateTime::getHour(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.hour());
  return buffer;
}
//**********************************************************************************************
char *CWDateTime::getMinute(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.minute());
  return buffer;
}
//**********************************************************************************************
char *CWDateTime::getSecond(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.second());
  return buffer;
}
//**********************************************************************************************
int CWDateTime::getHour()
{
  DateTime now = rtc.now();
  return now.hour();
}
//**********************************************************************************************
int CWDateTime::getMinute()
{
  DateTime now = rtc.now();
  return now.minute();
}
//**********************************************************************************************
int CWDateTime::getSecond()
{
  DateTime now = rtc.now();
  return now.second();
}
//**********************************************************************************************
int CWDateTime::getWeekday() 
{
  DateTime now = rtc.now();
  return now.dayOfTheWeek();
}
//**********************************************************************************************
bool CWDateTime::isAM() 
{
  DateTime now = rtc.now();
  return now.hour() < 12; // 判断是否为上午
}
//**********************************************************************************************
int CWDateTime::getDay() 
{
  DateTime now = rtc.now();
  return now.day();
}
//**********************************************************************************************
int CWDateTime::getMonth()
{
  DateTime now = rtc.now();
  return now.month();
}
//**********************************************************************************************