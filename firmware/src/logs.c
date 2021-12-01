#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "debugUart.h"
#include "logs.h"
// #include "flashaddress.h"
#include "xmem.h"
#include "time.h"
// #include "circularflash.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "list.h"
#include "global.h"
#include "rtcc.h"

#define DBG_MaxLen 512
#define DBG_LOG_LEVEL LOG_MAX
#if 0 
_ExceptionInfo ExceptionInfo __attribute__((address(0x8009FF00), coherent, persistent));
#endif

typedef struct {
    unsigned char * text;
    unsigned long len;
} LogItem;

static LogItem * PreScheduleLogs = NULL;
static unsigned long PreLogCount = 0;
SemaphoreHandle_t LogMutex = NULL;
static QueueHandle_t xQueueLog = NULL;
StreamBufferHandle_t cmdLogStream = NULL;

void SysLog(int LogLevel, const char* format, ...) {
    if (LogLevel > DEBUG_PRINT_LEVEL && LogLevel > DBG_LOG_LEVEL) {
      return;
    }
    configASSERT(!(xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED));
    LogItem log;
    log.text = x_malloc(DBG_MaxLen + 128, XMEM_HEAP_SLOW | XMEM_HEAP_ALIGN32);
    //debug_printf("x_malloc(%i) 0x%X\n", DBG_MaxLen + 128, (uint32_t)buf);

    if (!log.text) {
      return;
    }

    _time now;
    getRTCC(&now);
    log.len = snprintf(log.text, 128, "<%i> %02i/%02i/%02i %02i:%02i:%02i.%03i ",
        LogLevel,
        (now.mo),
        (now.day),
        (now.year),
        (now.hour),
        (now.min),
        (now.sec),
        (now.ms));

    va_list argptr;
    va_start(argptr, format);
    log.len += vsnprintf(&log.text[log.len], DBG_MaxLen, format, argptr);
    va_end(argptr);
    if (LogLevel <= DEBUG_PRINT_LEVEL) {
      if (cmdLogStream) {
        xStreamBufferSend(cmdLogStream, (void *)log.text, log.len, 1);
      } else {
        sendUartBytes(log.text, log.len); //For now, everything gets sent out the serial port
      }
    }
    x_free(log.text);
}

void vAssertCalled(uint32_t ulLine, const char *pcFileName) {
    #if 0
    ExceptionInfo.MagicNumber = EXC_MAGIC_NUM;
    ExceptionInfo.StackPointer = 0;
    ExceptionInfo.ExceptionAddress = 0;
    ExceptionInfo.Code = 0;
    ExceptionInfo.Handler = 'A';
    strcpy(ExceptionInfo.Information, pcFileName);
    sprintf(ExceptionInfo.Information, "%s : %i", pcFileName, ulLine);
    #endif
    if (xTaskGetSchedulerState() != taskSCHEDULER_SUSPENDED) {
	SysLog(LOG_NORMAL, "\r\nfreeRTOS ASSERT %s:%i\r\n", pcFileName, ulLine);
    }
    __asm__("BKPT");
    while (1);
}