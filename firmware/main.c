
#include <stm32f767xx.h>
//#include "inc/stm32_ll/stm32f7xx_ll_system.h"
//#include "inc/stm32_ll/stm32f7xx_ll_pwr.h"
//#include "inc/stm32_ll/stm32f7xx_ll_rcc.h"
#if DEBUG
#include <__cross_studio_io.h>
#endif

/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "list.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_IP.h"
#include "init.h"
#include "debugUart.h"
#include "semphr.h"
#include "task.h"
#include "http/httpROMFS.h"

#include "stm32f7xx_ll_gpio.h"
//#include "stm32f7xx_hal_dma2d.h"

#include "global.h"
#include "iperf_task.h"
#include "logs.h"
#include "cmd.h"
#include "cli.h"
#include "http/httpserver.h"
#include "sntp.h"
#include "buildversion.h"

const int __attribute__((used)) uxTopUsedPriority = configMAX_PRIORITIES - 1;

static void prvCheckTask1(void *pvParameters);
static void prvCheckTask2(void *pvParameters);
static void NetworkReadyTask(void *pvParameters);

TaskHandle_t xLoggingTaskHandle = NULL;
TaskHandle_t xLocalIdleHandle = NULL;
TaskHandle_t xCmdTaskHandle = NULL;
TaskHandle_t xTcpPuttyHandle = NULL;
TaskHandle_t xConfigurationHandle = NULL;
TaskHandle_t xDspTaskHandle = NULL;

static const uint8_t ucIPAddress[4] = { 192, 168, 0, 123 };
static const uint8_t ucNetMask[4] = { 255, 255, 255, 0 };
static const uint8_t ucGatewayAddress[4] = { 192, 168, 0, 1 };
static const uint8_t ucDNSServerAddress[4] = { 8, 8, 8, 8 };
uint8_t ucMACAddress[6] = { 0xEE, 0xAA, 0x0, 0xEE, 0xEE, 0xEE };

SemaphoreHandle_t debugMutex = NULL;
SemaphoreHandle_t printfMutex = NULL;
SemaphoreHandle_t ConfigMutex = NULL;
SemaphoreHandle_t PingMutex = NULL;
SemaphoreHandle_t ConsoleSingleMutex;

extern TaskHandle_t xServerWorkTaskHandle;

const _StackMeasure StacksToCheck[] = {
  { &xTcpPuttyHandle,       "Putty  " },
  { &xLoggingTaskHandle,    "Logging" },
  { &xServerWorkTaskHandle, "HTTP   " },
  { &xSntpHandle,           "SNTP"    },
  { &xLocalIdleHandle,      "IDLE   " },
  { NULL, "" }
};

void __debug_io_lock(void) {

  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xSemaphoreTake(debugMutex, portMAX_DELAY);
  }
}
void __debug_io_unlock(void) {
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xSemaphoreGive(debugMutex);
  }
}

void __printf_lock(void) {

  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xSemaphoreTake(printfMutex, portMAX_DELAY);
  }
}
void __printf_unlock(void) {
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xSemaphoreGive(printfMutex);
  }
}

void main(void) {

  int a;
  /* We need these lines to prevent fader movement on debug stop */
  // DBGMCU->APB2FZ |= DBGMCU_APB2_FZ_DBG_TIM1_STOP;
  //const unsigned char * test = hello;
  //char * test = malloc(4);
  initializeSystem();
#ifdef DEBUG
  vTraceEnable(TRC_START);
#endif

  initDebugUart();
  // drivQSPIinit();
  
  debugMutex = xSemaphoreCreateMutex();
  configASSERT(debugMutex != NULL);
  printfMutex = xSemaphoreCreateMutex();
  configASSERT(printfMutex != NULL);
  ConfigMutex = xSemaphoreCreateMutex();
  configASSERT(ConfigMutex != NULL);
  //controlMutex = xSemaphoreCreateMutex();
  //configASSERT(controlMutex != NULL);
  PingMutex = xSemaphoreCreateMutex();
  configASSERT(PingMutex != NULL);
  ConsoleSingleMutex = xSemaphoreCreateMutex();
  configASSERT(ConsoleSingleMutex != NULL);
  // (void)xTaskCreate(LogTasks,           "Logging",      256, NULL, PRIORITY_RTOS_LOGS, &xLoggingTaskHandle);
  (void)xTaskCreate(cmdtasks, "Cmd", 384, NULL, PRIORITY_RTOS_CMD, &xCmdTaskHandle);
  (void)xTaskCreate(PuttyTcpTasks, "Tcp putty", 384, NULL, PRIORITY_RTOS_PUTTY, &xTcpPuttyHandle);
  (void)xTaskCreate(NetworkReadyTask, "Net ready", 512, NULL, 1, NULL);
  sntp_init();
//Trace names
#ifdef DEBUG
  vTraceSetMutexName(debugMutex, "debug");
  vTraceSetMutexName(printfMutex, "printf");
  vTraceSetMutexName(ConfigMutex, "config");
#endif
  FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
  sendUartBytes("\r\n", 2);
  SysLog(LOG_NORMAL, "HTTP test %s - Build - %s %s\r\n", HTTP_BUILD_VERSION, __DATE__, __TIME__);
  CLI_RegisterCommands();
  vTaskStartScheduler();

  for (;;)
    ;
}

#include "stm32f7xx_ll_rtc.h"
static void prvCheckTask1(void *pvParameters) {
  static uint8_t aShowTime[50] = { 0 };
  static uint8_t aShowDate[50] = { 0 };
  while (1) {
    vTaskDelay(250);
  }
}

static void NetworkReadyTask(void *pvParameters) {

  HttpServerInit();
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (FreeRTOS_IsNetworkUp() && FreeRTOS_GetGatewayAddress()) {
      vTaskDelay(250 / portTICK_PERIOD_MS);
      SysLog(LOG_DEBUG, "Ethernet Adapter STM32 INT ready\r\n");
      (void)xTaskNotifyGive(xServerWorkTaskHandle);
      (void)xTaskNotifyGive(xSntpHandle);
      (void)xTaskNotifyGive(xTcpPuttyHandle);
      vTaskDelay(25 / portTICK_PERIOD_MS);
      vTaskDelete(NULL);
    }
  }
}

void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent) {

}

void vApplicationMallocFailedHook(void) {
  /* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

  /* Force an assert. */
  configASSERT((volatile void *)NULL);
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pcTaskName;
  (void)pxTask;

  /* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

  /* Force an assert. */
  configASSERT((volatile void *)NULL);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
  volatile size_t xFreeHeapSpace;

  /* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task.  It must *NOT* attempt to block.  In this case the
	idle task just queries the amount of FreeRTOS heap that remains.  See the
	memory management section on the http://www.FreeRTOS.org web site for memory
	management options.  If there is a lot of heap memory free then the
	configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
	RAM. */
  //xFreeHeapSpace = xPortGetFreeHeapSize();
  __NOP();
  /* Remove compiler warning about xFreeHeapSpace being set but never used. */
  (void)xFreeHeapSpace;
}
/*-----------------------------------------------------------*/
#if 0
void vAssertCalled(uint32_t ulLine, const char *pcFile) {
  volatile unsigned long ul = 0;

  (void)pcFile;
  (void)ulLine;

  taskENTER_CRITICAL();
  {
    /* Set ul to a non-zero value using the debugger to step out of this
		function. */
    while (ul == 0) {
      __NOP();
    }
  }
  taskEXIT_CRITICAL();
}
#endif
/*-----------------------------------------------------------*/