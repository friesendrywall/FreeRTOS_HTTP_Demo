
#include <stdint.h>
#include "FreeRTOS.h"
#include "rand.h"
#include "stm32f7xx_ll_rng.h"

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber) {
  while (!LL_RNG_IsActiveFlag_DRDY(RNG)) {
  }
  *pulNumber = LL_RNG_ReadRandData32(RNG);
  return 1;
}

/* TODO: not to be left here */
uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
    uint16_t usSourcePort,
    uint32_t ulDestinationAddress,
    uint16_t usDestinationPort) {
  static uint16_t port = 0;
  if (port == 0 || port == 0xFFFF) {
    port = (LL_RNG_ReadRandData32(RNG) >> 16);
  }
  return port++;
}