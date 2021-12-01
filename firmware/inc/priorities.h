#ifndef PRIORITIES_H
#define PRIORITIES_H

//Freertos priorities Highest = 10

#define PRIORITY_RTOS_QSPI      (configMAX_PRIORITIES - 2)
#define PRIORITY_RTOS_IP        (configMAX_PRIORITIES - 2)
#define PRIORITY_RTOS_MAC       (configMAX_PRIORITIES - 1)
#define PRIORITY_RTOS_DISCOVERY ( 1 )
#define PRIORITY_RTOS_CMD       ( 1 )
#define PRIORITY_RTOS_PUTTY     ( 1 )
#define PRIORITY_RTOS_DISPLAY   (configMAX_PRIORITIES - 1)
#define PRIORITY_RTOS_GUI       (configMAX_PRIORITIES - 3)
#define PRIORITY_RTOS_CONTROL   ( 1 )
#define PRIORITY_RTOS_DSPMAN    ( 1 )
#define PRIORITY_RTOS_DSP       ( 1 )
#define PRIORITY_RTOS_LOGS      ( 2 )
#define PRIORITY_RTOS_FADERS    (configMAX_PRIORITIES)
#define PRIORITY_RTOS_INPUT     ( 1 )
#define PRIORITY_RTOS_IPERF     ( 2 )

//Interrupt priorities Highest = 0
#define PRIORITY_DISPLAY_DMA    (3)
#define PRIORITY_DISPLAY_DMA2D  (3)
#define PRIORITY_USART1_RX      (2)
#define PRIORITY_USART1_TX_DMA  (3)
#define PRIORITY_TIMER1_PWM     (2)
#define PRIORITY_INPUT_I2C      (3)
#define PRIORITY_QUADSPI_INT    (3)
#define PRIORITY_QUADSPI_DMA    (3)
#define PRIORITY_ETH_INT        (3)
#define PRIORITY_EUIMAC         (5)

#endif 