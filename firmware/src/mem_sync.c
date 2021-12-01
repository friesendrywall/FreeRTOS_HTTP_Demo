#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stm32f769xx.h>
#include <__cross_studio_io.h>
#include "mem_sync.h"
#include "global.h"
#include "FreeRTOS.h"
#include "list.h"
#include "task.h"
#include "priorities.h"
#include "dsp/symetrix.h"

static internal_event_cb internalEventCallback = NULL;
static external_event_cb externalEventCallback = NULL;
static dsp_event_cb dspEventCallback = NULL;

void registerInternalCallback(internal_event_cb cb) {
  internalEventCallback = cb;
}

void registerExternalCallback(external_event_cb cb) {
  externalEventCallback = cb;
}

void registerDspEvent(dsp_event_cb cb) {
  dspEventCallback = cb;
}

uint32_t syncInternalExternal(uint32_t sync_item, uint32_t index, emValue value) {
  if (externalEventCallback) {
    return externalEventCallback(sync_item, index, value);
  } else {
    return 0;
  }
}

void syncExternalInternal(uint32_t sync_item, uint32_t index, emValue value, double optVal) {
  if (internalEventCallback) {
    internalEventCallback(sync_item, index, value, optVal);
  }
}

void sendDspEvent(uint32_t event) {
  if (dspEventCallback) {
    dspEventCallback(event);
  }
}

void resetDspErrors(void) {
  configASSERT(MEM_SYNC_LENGTH == CONFIG_LAYOUT_LENGTH); //Make sure these are synced
  memset(confSiteErrors.all, MEM_SYNC_SITE_UNKOWN, sizeof(conf_siteErrors));
}

void setDspError(uint32_t index, uint32_t value) {
  if (index >= CONFIG_LAYOUT_MEMORY_LENGTH) {
    return;
  }
  confSiteErrors.all[index] = value;
}

void setDspInfo(uint8_t *str) {
  (void)xSemaphoreTake(ConfigMutex, portMAX_DELAY);
  {
    snprintf(confInfo.deviceInfo, CONFIG_SITE_MAXLEN_STRING, str);
    confInfo.deviceInfo[CONFIG_SITE_MAXLEN_STRING] = 0;
  }
  (void)xSemaphoreGive(ConfigMutex);
}