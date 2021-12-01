#include "global.h"
#include "rtcc.h"
#include <stdarg.h>
#include <stdbool.h> // Defines true
#include <stddef.h>  // Defines NULL
#include <stdio.h>
#include <stdlib.h> // Defines EXIT_FAILURE
#include <stm32f767xx.h>
#include <string.h>
#include "stm32f7xx_hal.h"
//#include "json.h"
//#include "configsave.h"
//#include "wireshark.h"
#include "xmem.h"
//#include "tcpip/tcpip.h"
//#include "tcpip/dhcp.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
// #include "flashaddress.h"
#include "list.h"
#include "logs.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "debugUart.h"
#include "buildversion.h"

#define NOT_IMPLEMENTED (1)
static BaseType_t PrintIpConfig(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t PrintName(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t PrintSerial(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t PrintFirmware(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t ViewMalloc(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t PingHost(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t DigHost(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t CpuStats(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t RebootSystem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static BaseType_t viewTime(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state);
static const CLI_Command_Definition_t xIpconfig = {
  "ipconfig",
  "ipconfig : show ip configuration\r\n",
  PrintIpConfig,
  0,
  0
};

static const CLI_Command_Definition_t xViewMalloc = {
  "view alloc",
  "view alloc <slow,fast,sdram,slowi,fasti,sdrami>: view mem allocation and integrity\r\n",
  ViewMalloc,
  2,
  0
};

static const CLI_Command_Definition_t xPingHost = {
  "ping",
  "ping <ip addr>: ping external host\r\n",
  PingHost,
  1,
  0
};

static const CLI_Command_Definition_t xDigHost = {
  "dig",
  "dig <host>: dns host\r\n",
  DigHost,
  1,
  0
};

static const CLI_Command_Definition_t xCpuStats = {
  "cpu",
  "cpu : read cpu stats\r\n",
  CpuStats,
  0,
  0
};

static const CLI_Command_Definition_t xViewTime = {
  "time",
  "time : read date time\r\n",
  viewTime,
  0,
  0
};


static const CLI_Command_Definition_t xRebootSystem = {
  "reboot",
  "reboot : reboot system\r\n",
  RebootSystem,
  0,
  0
};

int cliSafePrint(char *apBuf, size_t *aMaxLen, const char *apFmt, ...) {
  if (*aMaxLen == 0) {
    return 0;
  }
  int len;
  va_list args;
  va_start(args, apFmt);
  len = vsnprintf(apBuf, *aMaxLen, apFmt, args);
  va_end(args);
  *aMaxLen -= len;
  return len;
}

void CLI_RegisterCommands(void) {
  (void)FreeRTOS_CLIRegisterCommand(&xIpconfig);
  (void)FreeRTOS_CLIRegisterCommand(&xViewMalloc);
  (void)FreeRTOS_CLIRegisterCommand(&xPingHost);
  (void)FreeRTOS_CLIRegisterCommand(&xDigHost);
  (void)FreeRTOS_CLIRegisterCommand(&xCpuStats);
  (void)FreeRTOS_CLIRegisterCommand(&xRebootSystem);
  (void)FreeRTOS_CLIRegisterCommand(&xViewTime);
}

static BaseType_t PrintIpConfig(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {

  BaseType_t xReturn;
  (void)pcCommandString;
  (void)*state;
  configASSERT(pcWriteBuffer);
  if (xWriteBufferLen < 300) {
    pcWriteBuffer[0] = 0;
    return pdFALSE;
  }
  int8_t cbuffer[20];
  uint32_t ulIPAddress;
  const uint8_t *MacAddress = FreeRTOS_GetMACAddress();
  time_t ut;
  gettime(&ut);
  //(void) TCPIP_SNTP_TimeGet(&pUTCSeconds, &pMs);
  //TODO: Fix
  pcWriteBuffer += sprintf(pcWriteBuffer, "\r\nEthernet Adapter %s\r\n", "STM32F7 INT");
  ulIPAddress = FreeRTOS_GetIPAddress();
  FreeRTOS_inet_ntoa(ulIPAddress, (char *)cbuffer);
  pcWriteBuffer += sprintf(pcWriteBuffer, "IPv4 Address.  .  . %s\r\n", cbuffer);
  ulIPAddress = FreeRTOS_GetNetmask();
  FreeRTOS_inet_ntoa(ulIPAddress, (char *)cbuffer);
  pcWriteBuffer += sprintf(pcWriteBuffer, "Subnet Mask .  .  . %s\r\n", cbuffer);
  ulIPAddress = FreeRTOS_GetGatewayAddress();
  FreeRTOS_inet_ntoa(ulIPAddress, (char *)cbuffer);
  pcWriteBuffer += sprintf(pcWriteBuffer, "Default Gateway.  . %s\r\n", cbuffer);
  ulIPAddress = FreeRTOS_GetDNSServerAddress();
  FreeRTOS_inet_ntoa(ulIPAddress, (char *)cbuffer);
  pcWriteBuffer += sprintf(pcWriteBuffer, "DNS Servers .  .  . %s\r\n", cbuffer);
  pcWriteBuffer += sprintf(pcWriteBuffer, "Physical Address  . %02X-%02X-%02X-%02X-%02X-%02X\r\n", MacAddress[0], MacAddress[1], MacAddress[2], MacAddress[3], MacAddress[4], MacAddress[5]);
  pcWriteBuffer += sprintf(pcWriteBuffer, "UTC TIME .  .  .  . %u\r\n", ut);
  xReturn = pdFALSE;

  return xReturn;
}


static BaseType_t viewTime(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  BaseType_t xReturn;
  (void)pcCommandString;
  (void)*state;
  configASSERT(pcWriteBuffer);
  uint32_t n;
  if (xWriteBufferLen < 300) {
    pcWriteBuffer[0] = 0;
    return pdFALSE;
  }
  _time now;
  getRTCC(&now);
  pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\n%02i/%02i/%02i %02i:%02i:%02i.%03i\r\n",
      (now.mo),
      (now.day),
      (now.year),
      (now.hour),
      (now.min),
      (now.sec),
      (now.ms));
  return pdFALSE;
}


void Reboot_Callback5s( TimerHandle_t xTimer ) {
  SysLog(LOG_DEBUG, "CLI SoftReset()\r\n");
  while (uartTxInProgress())
    ;
  NVIC_SystemReset();
}

static BaseType_t RebootSystem(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  configASSERT(pcWriteBuffer);
  if (xWriteBufferLen < 32) {
    pcWriteBuffer[0] = 0;
    return pdFALSE;
  }
  pcWriteBuffer += cliSafePrint(pcWriteBuffer, &xWriteBufferLen, "Going down for reboot in 5 s\r\n");
  xTimerCreate("Reboot", 5000, pdFALSE, NULL, Reboot_Callback5s);
  return pdFALSE;
}

static BaseType_t UpdateFirmware(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  configASSERT(pcWriteBuffer);
  if (xWriteBufferLen < 32) {
    pcWriteBuffer[0] = 0;
    return pdFALSE;
  }
  (void)xSemaphoreTake(ConfigMutex, portMAX_DELAY);
  {
#ifndef NOT_IMPLEMENTED
    SysServices.DoUpdate = 1;
#endif
  }
  (void)xSemaphoreGive(ConfigMutex);
  pcWriteBuffer += snprintf(pcWriteBuffer, 32, "Starting web update");
  return pdFALSE;
}

static BaseType_t ViewMalloc(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  static const int MAX_LENGTH = 0x16000;
  configASSERT(pcWriteBuffer);
  uint32_t outlen;
  BaseType_t s_len;
  unsigned char *info;
  const char *pcParameter;

  if (state->state) {
    //state->state holds total length
    //state->index holds current position
    info = (unsigned char *)state->ptr;
    outlen = state->state - state->index;
    if (outlen >= xWriteBufferLen - 1) {
      outlen = xWriteBufferLen - 1;
      memcpy(pcWriteBuffer, &info[state->index], outlen);
      pcWriteBuffer[outlen] = 0;
      state->index += outlen;
      return pdTRUE;
    } else {
      memcpy(pcWriteBuffer, &info[state->index], outlen);
      pcWriteBuffer[outlen] = 0;
    }
    x_free(info);
    return pdFALSE;
  } else {
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2, &s_len);
    if (pcParameter == NULL) {
      pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "Enter selection");
      return pdFALSE;
    }
    info = x_malloc(MAX_LENGTH, XMEM_HEAP_SLOW);
    if (info == NULL) {
      pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "Unable to allocate SDRAM");
      return pdFALSE;
    }
    if (memcmp(pcParameter, "fasti", 5) == 0) {
      (void)x_integrity(XMEM_HEAP_FAST, info, MAX_LENGTH);
    } else if (memcmp(pcParameter, "slowi", 5) == 0) {
      (void)x_integrity(XMEM_HEAP_SLOW, info, MAX_LENGTH);
    }  else if (memcmp(pcParameter, "fast", 4) == 0) {
      x_info(XMEM_HEAP_FAST, info, MAX_LENGTH);
    } else if (memcmp(pcParameter, "slow", 4) == 0) {
      x_info(XMEM_HEAP_SLOW, info, MAX_LENGTH);
    } else {
      pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "%s not found", pcParameter);
      return pdFALSE;
    }
    state->state = strlen(info);

    if (state->state >= xWriteBufferLen - 1) {
      outlen = xWriteBufferLen - 1;
      memcpy(pcWriteBuffer, info, outlen);
      pcWriteBuffer[outlen] = 0;
      state->index = outlen;
      state->ptr = (void *)info;
      return pdTRUE;
    } else {
      memcpy(pcWriteBuffer, info, state->state);
      pcWriteBuffer[state->state] = 0;
    }
    x_free(info);
    return pdFALSE;
  }
}

static int EchoReady = 0;
static _Timer PingTimer;

void vApplicationPingReplyHook(ePingReplyStatus_t eStatus, uint16_t usIdentifier) {
  switch (eStatus) {
  case eSuccess:
    EchoReady = 1;
    PingTimer = HAL_GetTick();
    break;

  case eInvalidChecksum:
  case eInvalidData:
    /* A reply was received but it was not valid. */
    EchoReady = -1;
    break;
  }
}

static BaseType_t PingHost(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  configASSERT(pcWriteBuffer);
  BaseType_t s_len;
  const char *pcParameter;
  uint16_t usRequestSequenceNumber;
  static uint32_t ulIPAddress;
#define PingBytes 16
  static _Timer AuxTimer;
  pcWriteBuffer[0] = 0;
  if (state->Cancel) {
    if (state->state > 0) {
      free(state->ptr);
      (void)xSemaphoreGive(PingMutex);
    }
    return pdFALSE;
  } else {
    switch (state->state) {
    case 0:
      pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &s_len);
      if (pcParameter == NULL) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "No name");
        return pdFALSE;
      }
      if (strlen(pcParameter) > 128) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "Name length > 128");
        return pdFALSE;
      }
      if (xSemaphoreTake(PingMutex, 10 / portTICK_PERIOD_MS) != pdTRUE) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "ping module in use");
        return pdFALSE;
      }
      ulIPAddress = FreeRTOS_inet_addr(pcParameter);
      if (!ulIPAddress && !state->index) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "Use dig <host> to find ip");
        (void)xSemaphoreGive(PingMutex);
        return pdFALSE;
      }

      pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nPinging %s with %i bytes data", pcParameter, PingBytes);
      state->ptr = strdup(pcParameter);
      state->state = 1;
      //lint -fallthrough
    case 1:
      AuxTimer = HAL_GetTick();
      if (FreeRTOS_IsNetworkUp()) {
        usRequestSequenceNumber = FreeRTOS_SendPingRequest(ulIPAddress, PingBytes, 100 / portTICK_PERIOD_MS);
        if (usRequestSequenceNumber == pdFAIL) {
          /* The ping could not be sent because a network buffer could not be
			obtained within 100ms of FreeRTOS_SendPingRequest() being called. */
          state->state = 4;
          pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nNo network");
        } else {
          state->state = 2;
        }
      } else {
        state->state = 4;
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nNo network");
      }
      return pdTRUE;
    case 2:
      if (EchoReady > 0) {
        _Timer t = PingTimer - AuxTimer;
        t /= (HAL_TICK_FREQ / 1000);
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nreply from %s: bytes=%i time=%ims", (char *)state->ptr, PingBytes, t);
        EchoReady = 0;
        state->state = 3;
      } else if (EchoReady < 0) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nTimeout");
        EchoReady = 0;
        state->state = 3;
      }
      if (HAL_GetTick() - AuxTimer > HAL_TICK_FREQ * 2) {
        pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "\r\nTimeout");
        state->state = 1;
      }
      //lint -fallthrough
    case 3:
      if (HAL_GetTick() - AuxTimer > HAL_TICK_FREQ * 1) {
        state->state = 1;
        if (state->Remote) {
          state->index++;
          if (state->index >= 5) {
            state->state = 4;
          }
        }
      }
      return pdTRUE;
    case 4:
      free(state->ptr);
      (void)xSemaphoreGive(PingMutex);
      return pdFALSE;
    default:
      return pdFALSE;
    }
  }
}

static BaseType_t DigHost(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  configASSERT(pcWriteBuffer);
  BaseType_t s_len;
  const char *pcParameter;
  uint32_t ulIPAddress;
  int8_t cBuffer[20];
  pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &s_len);
  if (pcParameter == NULL) {
    pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "No host name");
    return pdFALSE;
  }
  if (strlen(pcParameter) > 128) {
    pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "Host Name length > 128");
    return pdFALSE;
  }
  ulIPAddress = FreeRTOS_gethostbyname(pcParameter);
  if (ulIPAddress != 0) {
    /* Convert the IP address to a string. */
    FreeRTOS_inet_ntoa(ulIPAddress, (char *)cBuffer);

    /* Print out the IP address. */
    pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "%s is at IP address %s\r\n", pcParameter, cBuffer);
  } else {
    pcWriteBuffer += snprintf(pcWriteBuffer, xWriteBufferLen, "DNS lookup failed.");
  }

  return pdFALSE;
}

static BaseType_t CpuStats(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString, CLI_CmdState *state) {
  extern const _StackMeasure StacksToCheck[];
  BaseType_t s_len;
  char *buf;
  int i, a, len;
  uint32_t Total;
  uint32_t Percent;
  pcWriteBuffer[0] = 0;
  if (state->state) {
    //state->index = index
    //state->state = length;
    i = state->state - state->index;
    buf = (char *)state->ptr;
    if (i >= xWriteBufferLen) {
      memcpy(pcWriteBuffer, &buf[state->index], xWriteBufferLen - 1);
      pcWriteBuffer[xWriteBufferLen - 1] = 0;
      state->index += xWriteBufferLen - 1;
      return pdTRUE;
    } else {
      memcpy(pcWriteBuffer, &buf[state->index], i + 1);
      free(buf);
      return pdFALSE;
    }
  }
  i = 0;
  if (xLocalIdleHandle == NULL) {
    xLocalIdleHandle = xTaskGetIdleTaskHandle();
  }
  while (StacksToCheck[i].handle != NULL) {
    i++;
  }
  int CpuCheckCount = i;
  TaskStatus_t *Status1 = x_calloc(CpuCheckCount, sizeof(TaskStatus_t), XMEM_HEAP_SLOW);
  TaskStatus_t *Status2 = x_calloc(CpuCheckCount, sizeof(TaskStatus_t), XMEM_HEAP_SLOW);
  uint32_t *ET = x_calloc(CpuCheckCount, sizeof(uint32_t), XMEM_HEAP_FAST);
  configASSERT(Status1);
  configASSERT(Status2);
  configASSERT(ET);
  for (i = 0; i < CpuCheckCount; i++) {
    vTaskGetInfo(*StacksToCheck[i].handle, &Status1[i], pdFALSE, eRunning);
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  for (i = 0; i < CpuCheckCount; i++) {
    vTaskGetInfo(*StacksToCheck[i].handle, &Status2[i], pdTRUE, eRunning);
  }

  size_t MaxLen = 2048;
  state->ptr = malloc(MaxLen);
  configASSERT(state->ptr);
  buf = (char *)state->ptr;
  buf += cliSafePrint(buf, &MaxLen, "Task Name       | CPU  | Stack\r\n");
  Total = 0;
  for (i = 0; i < CpuCheckCount; i++) {
    ET[i] = Status2[i].ulRunTimeCounter - Status1[i].ulRunTimeCounter;
    Total += ET[i];
  }
  if (!Total) {
    Total = 1;
  }
  for (i = 0; i < CpuCheckCount; i++) {
    Percent = (uint32_t)((uint64_t)ET[i] * 1000ULL / Total);
    //Status2[i].pcTaskName = "Test";
    len = strlen(Status2[i].pcTaskName);
    buf += cliSafePrint(buf, &MaxLen, "%s", Status2[i].pcTaskName);
    for (a = len; a < 16; a++) {
      if (MaxLen > 2) {
        MaxLen--;
        *buf++ = ' ';
      }
    }
    buf += cliSafePrint(buf, &MaxLen, "| %02i.%i | %i\r\n",
        Percent / 10, Percent % 10, Status2[i].usStackHighWaterMark);
  }
  x_free(Status1);
  x_free(Status2);
  x_free(ET);
  //pcWriteBuffer += cliSafePrint(pcWriteBuffer, &xWriteBufferLen, "Total\r\n",
  //	Percent / 10, Percent % 10, Status2.usStackHighWaterMark);
  buf = (char *)state->ptr;
  state->state = strlen(buf);
  if (state->state >= xWriteBufferLen) {
    memcpy(pcWriteBuffer, buf, xWriteBufferLen - 1);
    pcWriteBuffer[xWriteBufferLen - 1] = 0;
    state->index = xWriteBufferLen - 1;
    return pdTRUE;
  } else {
    memcpy(pcWriteBuffer, buf, state->state + 1);
    free(buf);
    return pdFALSE;
  }
  return pdFALSE;
}