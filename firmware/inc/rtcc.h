#ifndef RTCC_H
#define RTCC_H

#include <stdint.h>
#include <time.h>
#include "stm32f7xx_ll_rtc.h"

typedef struct {
  uint32_t mo;
  uint32_t day;
  uint32_t year;
  uint32_t hour;
  uint32_t min;
  uint32_t sec;
  uint32_t ms;
} _time;

void getRTCC(_time *t);
int GetUtcTime(struct tm * dt);
time_t gettime(time_t * t);
int settime(time_t * t);

#endif