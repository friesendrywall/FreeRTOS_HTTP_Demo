#include <stm32f767xx.h>
#include <stdio.h>
#include "rtcc.h"

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

void getRTCC(_time *t) {
  uint32_t ss;
  while (1) {
    ss = LL_RTC_TIME_GetSubSecond(RTC);
    t->mo = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC));
    t->day = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
    t->year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC));
    t->hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
    t->min = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
    t->sec = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
    if (ss == LL_RTC_TIME_GetSubSecond(RTC)) {
      break;
    }
  }
  t->ms = ((0x8000 - LL_RTC_TIME_GetSubSecond(RTC)) * 1000 / 0x8000);
}

int CalculateWeekDay_(unsigned int Year, unsigned int Month, unsigned int Day) {
  const char MonthOffset[] =
      //jan feb mar apr may jun jul aug sep oct nov dec
      { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
  unsigned Offset;

  // 2000s century offset = 6 +
  // every year 365%7 = 1 day shift +
  // every leap year adds 1 day
  Offset = 6 + Year + Year / 4;
  // Add month offset from table
  Offset += MonthOffset[Month - 1];
  // Add day
  Offset += Day;

  // If it's a leap year and before March there's no additional day yet
  if ((Year % 4) == 0) {
    if (Month < 3) {
      Offset -= 1;
    }
  }

  // Week day is
  Offset %= 7;

  return Offset;
}

void WriteUtcTime(struct tm *dt) {
  LL_RTC_TimeTypeDef RTC_TimeStruct = { 0 };
  LL_RTC_DateTypeDef RTC_DateStruct = { 0 };
  RTC_TimeStruct.Hours = dt->tm_hour;
  RTC_TimeStruct.Minutes = dt->tm_min;
  RTC_TimeStruct.Seconds = dt->tm_sec;
  LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_TimeStruct);
  RTC_DateStruct.WeekDay = CalculateWeekDay_(dt->tm_year, dt->tm_mon + 1, dt->tm_mday);
  RTC_DateStruct.Month = dt->tm_mon;
  RTC_DateStruct.Day = dt->tm_mday;
  RTC_DateStruct.Year = dt->tm_year;
  LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_DateStruct);
}

int settime(time_t *t) {
  struct tm res;
  (void)gmtime_r(t, &res);
  res.tm_year -= 100;
  WriteUtcTime(&res);
  return 0;
}

int GetUtcTime(struct tm * dt) {
    static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int a;
    int yday = 0;
    _time t = {0};
    getRTCC(&t);
    //unsigned int t;
    dt->tm_sec = t.sec;
    dt->tm_min = t.min;
    dt->tm_hour = t.hour;
    dt->tm_mday = t.day;
    dt->tm_mon = t.mo;
    dt->tm_year = 100 + t.year;
    dt->tm_isdst = 0;
    dt->tm_wday = 0;
    for (a = 0; a < 12; a++) {
	if (a + 1 < dt->tm_mon) {
	    yday += DAYS_IN_MONTH[a];
	    if (isleap(dt->tm_year) && a == 1) {
		yday -= 1;
	    }
	} else {
	    yday += dt->tm_mday;
	    break;
	}
    }
    dt->tm_yday = yday;
    return 1;
}

time_t gettime(time_t * t) {

    struct tm now;
    if (!GetUtcTime(&now)) {
	if (t != NULL) {
	    *t = 0;
	}
	return 0;
    }
    time_t res = mktime(&now);
    if (t != NULL) {
	*t = res;
    }
    return res;
}

time_t time(time_t *t) {
  return gettime(t);
}