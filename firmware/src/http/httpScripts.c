#include <__cross_studio_io.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "init.h"
#include <stm32f7xx_ll_gpio.h>

#include "rtcc.h"
#include "time.h"

#include "http/httpROMFS.h"
#include "logs.h"
#include "parson.h"
#define MAX_PARAM_LEN 32

//void testButtonTask(void *params);
static void testFaderTask(void *params);

_httpResponse custom_GET_txt(uint8_t *data, uint32_t len, uint8_t *urlParams) {
  _httpResponse resp = { .responseCode = 200, .gz = 0, .sendFile = 0 };
  resp.data = malloc(64);
  resp.len = sprintf(resp.data, "Hello world, here we are!");
  resp.len = strlen(resp.data);
  return resp;
}

_httpResponse choose_file_GET_txt(uint8_t *data, uint32_t len, uint8_t *urlParams) {
  _httpResponse resp = { .responseCode = 200, .gz = 0, .sendFile = 0 };
  resp.data = malloc(64);
  if (urlParams) {
    resp.sendFile = 1;
    char *selected = strnstr(urlParams, "=", MAX_PARAM_LEN);
    if (!selected) {
      resp.len = sprintf(resp.data, "Bad params!");
      resp.len = strlen(resp.data);
    } else {
      int fileSelection = atoi(&selected[1]);
      if (fileSelection == 1) {
        resp.sendFile = 1;
        resp.len = sprintf(resp.data, "/choice1.txt");
      } else {
        resp.sendFile = 1;
        resp.len = sprintf(resp.data, "/choice2.txt");
      }
    }

    return resp;
  } else {
    sprintf(resp.data, "No URL params");
    resp.len = strlen(resp.data);
    return resp;
  }
}

_httpResponse file_POST_php(uint8_t *data, uint32_t len, uint8_t *urlParams) {
  _httpResponse resp = { .responseCode = 200, .gz = 0, .sendFile = 0 };
  data[len] = 0; //Null terminate, we have extra bytes malloced for this
  resp.data = malloc(64);
  if (len > 1024) {
      resp.responseCode = 400;
      resp.len = sprintf(resp.data, "Too long or something");
  }
  else {
      printf("File was posted, length = %i, %02X\r\n", len, data[0]);
      resp.len = sprintf(resp.data, "We got the file length was %i", len);
  }
  return resp;
}

_httpResponse checkbox_GET(uint8_t *data, uint32_t len, uint8_t *urlParams) {
  _httpResponse resp = {.responseCode = 200, .gz = 0, .sendFile = 0};
  resp.data = NULL;
  // debug_printf(urlParams);
  debug_printf("%s\n", urlParams);

  // Turn on LD1
  if (strcmp(urlParams, "value=true") == 0) {
    LL_GPIO_SetOutputPin(LD1_GPIO_Port, LD1_Pin);
  } else if (strcmp(urlParams, "value=false") == 0) {
    LL_GPIO_ResetOutputPin(LD1_GPIO_Port, LD1_Pin);
  }
  /*************************/
  if (resp.data == NULL) {
    resp.data = malloc(324);
    resp.responseCode = 200;
    resp.len = sprintf(resp.data, "Yay");
  }
  resp.len = strlen(resp.data);
  return resp;
}