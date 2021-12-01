#ifndef GLOBAL_H
#define GLOBAL_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include "buildversion.h"

#define USE_DHCP 1
//#ifndef BUILDNUMBER
//#include "buildnum.h"
//#endif

//#define EASYMIXVERSION BUILDVERSION

#define HAL_TICK_FREQ (1000) //Hack, but stock API is weird

/*typedef struct {
  unsigned char DeviceSerial[9];
  int LogLevel;
} _Info;*/

typedef struct {
  TaskHandle_t *handle;
  const char *name;
} _StackMeasure;

typedef uint32_t _Timer;
//extern volatile _Info Info;

extern SemaphoreHandle_t ConfigMutex;
extern SemaphoreHandle_t PingMutex;
extern SemaphoreHandle_t ConsoleSingleMutex;
//extern SemaphoreHandle_t controlMutex;

extern TaskHandle_t xLocalIdleHandle;

#endif