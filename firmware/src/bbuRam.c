#include <stm32f767xx.h>
#include "bbuRam.h"

volatile _bburam * nvRAM = (_bburam*)BKPSRAM_BASE;