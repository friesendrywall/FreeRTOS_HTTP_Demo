#ifndef EUI_MAC_H
#define EUI_MAC_H
#include <stdint.h>

uint32_t loadEUI48(uint8_t *addr);

#define EUI_MAC_TMO 1
#define EUI_MAC_NAK 2

#define AT24MAC602_ADDR 0x98

#endif