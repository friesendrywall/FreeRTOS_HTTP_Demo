/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_it.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_tim.h"
#include "stm32f7xx_ll_dma.h"
#include "FreeRTOS.h"
#include "task.h"

extern TIM_HandleTypeDef htim7;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M7 Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
uint32_t faultData[16];

void hard_fault_handler_c(unsigned int *hardfault_args) {
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;

  //Exception stack frame
  faultData[0] = stacked_r0 = ((unsigned long)hardfault_args[0]);
  faultData[1] = stacked_r1 = ((unsigned long)hardfault_args[1]);
  faultData[2] = stacked_r2 = ((unsigned long)hardfault_args[2]);
  faultData[3] = stacked_r3 = ((unsigned long)hardfault_args[3]);

  faultData[4] = stacked_r12 = ((unsigned long)hardfault_args[4]);
  faultData[5] = stacked_lr = ((unsigned long)hardfault_args[5]);
  faultData[6] = stacked_pc = ((unsigned long)hardfault_args[6]);
  faultData[7] = stacked_psr = ((unsigned long)hardfault_args[7]);
  faultData[8] = (*((volatile unsigned long *)(0xE000ED38)));
  faultData[9] = (*((volatile unsigned long *)(0xE000ED28)));
  faultData[10] = (*((volatile unsigned long *)(0xE000ED2C)));
  faultData[11] = (*((volatile unsigned long *)(0xE000ED30)));
  faultData[12] = (*((volatile unsigned long *)(0xE000ED3C)));
  //TODO: Dump this data into a loggable location

  for (;;) {
  }
}

void HardFault_Handler(void) {
  /* USER CODE BEGIN HardFault_IRQn 0 */
  asm volatile(
      "tst lr, #4\t\n" /* Check EXC_RETURN[2] */
      "ite eq\t\n"
      "mrseq r0, msp\t\n"
      "mrsne r0, psp\t\n"
      "b hard_fault_handler_c\t\n"
      :      /* no output */
      :      /* no input */
      : "r0" /* clobber */
  );
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM7 global interrupt.
  */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  /* USER CODE END TIM7_IRQn 1 */
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
