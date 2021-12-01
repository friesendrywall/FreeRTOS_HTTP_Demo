#include <__cross_studio_io.h>

/* Standard includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "debugUart.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_usart.h"
#include "init.h"
#include "priorities.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stm32f767xx.h>

static volatile uint8_t RxCommand[256];
static volatile uint8_t UartRxPtr = 0;
static unsigned char Uarttail = 0;

static uint8_t txDmaBuff[64]__attribute__((section(".first_data")));
static volatile uint8_t * Uart_TxPtr;
static volatile uint8_t TxInProgress = 0;
static volatile uint8_t TxBuff[4096];
static volatile uint32_t TxHead;
static volatile uint32_t TxTail;
static SemaphoreHandle_t uartMutex;

void initDebugUart(void) {
  NVIC_SetPriority(USART1_IRQn, PRIORITY_USART1_RX);
  NVIC_EnableIRQ(USART1_IRQn);
  LL_USART_EnableIT_RXNE(USART1);
  LL_USART_EnableIT_ERROR(USART1);
  //Dma setup
  NVIC_SetPriority(DMA2_Stream7_IRQn, PRIORITY_USART1_TX_DMA);
  uartMutex = xSemaphoreCreateMutex();
  configASSERT(uartMutex != NULL);
}

void PrintUart(const char* format, ...) {
    static char buf[2048];//Not thread safe
    int n;
    va_list argptr;
    va_start(argptr, format);
    n = vsprintf(buf, format, argptr);
    va_end(argptr);
    sendUartBytes(buf, n);
}

uint32_t UartTxSpace(void) {
  uint32_t rv = 0;
  if (xTaskGetSchedulerState() != taskSCHEDULER_SUSPENDED) {
    xSemaphoreTake(uartMutex, portMAX_DELAY);
  }
  NVIC_DisableIRQ(DMA2_Stream7_IRQn); //Pin stuff so it doesn't move on us
  if (TxHead == TxTail) {
    rv = sizeof(TxBuff) - 1;
  } else {
    if (TxHead > TxTail) {
      rv = sizeof(TxBuff) - (TxHead - TxTail) - 1;
    } else {
      rv = sizeof(TxBuff) - ((sizeof(TxBuff) - TxTail) + TxHead) - 1;
    }
  }
  NVIC_EnableIRQ(DMA2_Stream7_IRQn);
  if (xTaskGetSchedulerState() != taskSCHEDULER_SUSPENDED) {
    xSemaphoreGive(uartMutex);
  }
  return rv;
}

uint32_t uartTxInProgress(void) {
  return TxInProgress;
}

void sendUartBytes(uint8_t *buff, uint32_t length) {
  int a;
  if (!length) {
    return;
  }
  xSemaphoreTake(uartMutex, portMAX_DELAY);
  NVIC_DisableIRQ(DMA2_Stream7_IRQn); //Pin stuff so it doesn't move on us
  //Get max available length
  if (TxHead == TxTail) {
    a = sizeof(TxBuff) - 1;
  } else {
    if (TxHead > TxTail) {
      a = sizeof(TxBuff) - (TxHead - TxTail) - 1;
    } else {
      a = sizeof(TxBuff) - ((sizeof(TxBuff) - TxTail) + TxHead) - 1;
    }
  }
  if (length > a) {
    length = a;
  }

  for (a = 0; a < length; a++) {
    TxBuff[TxHead++] = buff[a];
    if (TxHead > sizeof(TxBuff) - 1) {
      TxHead -= sizeof(TxBuff);
    }
  }
  if (!TxInProgress) {
    for (a = 0; a < sizeof(txDmaBuff); a++) { //repeat n times to fill buffer
      if (TxHead != TxTail) {                 //only if count is not reached
        txDmaBuff[a] = TxBuff[TxTail++];
        if (TxTail == sizeof(TxBuff)) {
          TxTail = 0;
        }
      } else {
        break;
      }
    }
    TxInProgress = 1;
    LL_USART_EnableDMAReq_TX(USART1);

    LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_7, LL_DMA_CHANNEL_4);
    LL_DMA_ConfigTransfer(DMA2, LL_DMA_STREAM_7,
        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | LL_DMA_MODE_NORMAL |
            LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |
            LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE |
            LL_DMA_PRIORITY_LOW);

    LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_7,
        (uint32_t)txDmaBuff,
        LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_TRANSMIT),        
        LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_7,
        length > sizeof(txDmaBuff) ? sizeof(txDmaBuff) : length);
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_7);
    LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_7);
    LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_7);
  }

  NVIC_EnableIRQ(DMA2_Stream7_IRQn);
  xSemaphoreGive(uartMutex);
  //TxInProgress = 1;
}

uint32_t GetUartReadyBytes(uint8_t *Buff) {
  int a;
  if (Uarttail == UartRxPtr) {
    return 0;
  }
  uint8_t PinnedPtr = UartRxPtr;
  uint8_t TotalBytes = PinnedPtr - Uarttail;
  uint8_t Loc = Uarttail;
  for (a = 0; a < TotalBytes; a++) {
    Buff[a] = RxCommand[Loc++];
  }
  Uarttail = PinnedPtr;
  return TotalBytes;
}

uint32_t UartReadyBytes(void) {
  uint8_t TotalBytes = UartRxPtr - Uarttail;
  return TotalBytes;
}

void USART1_IRQHandler(void) {
  static uint8_t character;
  if (LL_USART_IsActiveFlag_RXNE(USART1)) {
    /* RXNE flag will be cleared by reading of RDR register (done in call) */
    /* Call function in charge of handling Character reception */
    RxCommand[UartRxPtr++] = LL_USART_ReceiveData8(USART1);

  } else if (LL_USART_IsActiveFlag_FE(USART1)) {
    LL_USART_ClearFlag_FE(USART1);
  } else if (LL_USART_IsActiveFlag_PE(USART1)) {
    LL_USART_ClearFlag_PE(USART1);
  } else if (LL_USART_IsActiveFlag_NE(USART1)) {
    LL_USART_ClearFlag_NE(USART1);
  } else if (LL_USART_IsActiveFlag_ORE(USART1)) {
    LL_USART_ClearFlag_ORE(USART1);
  } else {
    __NOP();//__asm__("BKPT");
  }
  (void)USART1->ISR;
}

/* UART1_TX handler */
void DMA2_Stream7_IRQHandler(void) {
  int len;
  /* Check whether DMA transfer complete caused the DMA interruption */
  if (LL_DMA_IsActiveFlag_TC7(DMA2) == 1) {
    /* Clear flag DMA transfer complete */
    LL_DMA_ClearFlag_TC7(DMA2);
    /* Is there more queued? */
    if (TxTail == TxHead) {
      TxInProgress = 0;
    } else {
      for (len = 0; len < sizeof(txDmaBuff); len++) { //repeat n times to fill buffer
        if (TxHead != TxTail) {                 //only if count is not reached
          txDmaBuff[len] = TxBuff[TxTail++];
          if (TxTail == sizeof(TxBuff)) {
            TxTail = 0;
          }
        } else {
          break;
        }
      }
      //Restart
      LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_7, len);
      LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_7);
    }
  }

  /* Check whether DMA half transfer caused the DMA interruption */
  if (LL_DMA_IsActiveFlag_HT7(DMA2) == 1) {
    /* Clear flag DMA half transfer */
    LL_DMA_ClearFlag_HT7(DMA2);
    //__asm__("BKPT");
  }

  if (LL_DMA_IsActiveFlag_TE7(DMA2) == 1) {
    /* Clear flag DMA transfer error */
    LL_DMA_ClearFlag_TE7(DMA2);
    __asm__("BKPT");//TODO: remove at some point after testing
  }
}