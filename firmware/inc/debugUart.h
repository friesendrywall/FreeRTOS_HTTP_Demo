#ifndef DEBUGUART_H
#define DEBUGUART_H

#include <stdint.h>
void initDebugUart(void);
void sendUartBytes(uint8_t * buff, uint32_t length);
uint32_t GetUartReadyBytes(uint8_t *Buff);
uint32_t UartReadyBytes(void);
uint32_t uartTxInProgress(void);
void PrintUart(const char* format, ...);
uint32_t UartTxSpace(void);

#endif