#ifndef BBURAM_H
#define BBURAM_H

#include <stdint.h>

typedef struct {
  uint8_t bootcmd[16];
  struct {
    uint32_t MagicNumber;
    uint32_t StackPointer;
    uint32_t ExceptionAddress;
    uint32_t Code;
    uint32_t Handler;
    uint8_t Information[256];
  } exception;
} _bburam;

extern volatile _bburam * nvRAM;
#endif