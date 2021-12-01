/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : stm32f7xx_hal_msp.c
  * Description        : This file provides code for the MSP Initialization 
  *                      and de-Initialization codes.
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
#include "stm32f7xx_hal_conf.h"
#include "stm32f7xx_hal_dma.h"
#include "stm32f7xx_hal_rcc.h"
#include "stm32f7xx_hal_cortex.h"
#include "stm32f7xx_hal_eth.h"
// #include "stm32f7xx_hal_sdram.h"
//#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_hal_qspi.h"
#include "priorities.h"
#include "init.h"

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
* @brief ETH MSP Initialization
* This function configures the hardware resources used in this example
* @param heth: ETH handle pointer
* @retval None
*/
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth) {
  //GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_GPIO_InitTypeDef LL_GPIO_InitStruct = {0};
  if (heth->Instance == ETH) {
    /* USER CODE BEGIN ETH_MspInit 0 */

    /* USER CODE END ETH_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // __HAL_RCC_GPIOG_CLK_ENABLE(); Redundant
    // __HAL_RCC_GPIOC_CLK_ENABLE();
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ETH GPIO Configuration    
    PG14     ------> ETH_TXD1
    PG13     ------> ETH_TXD0
    PG11     ------> ETH_TX_EN
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PC4     ------> ETH_RXD0
    PA2     ------> ETH_MDIO
    PC5     ------> ETH_RXD1
    PA7     ------> ETH_CRS_DV 
    */

    LL_GPIO_InitStruct.Pin = (RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin);
    LL_GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    LL_GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    LL_GPIO_InitStruct.Alternate = LL_GPIO_AF_11;
    LL_GPIO_Init(GPIOC, &LL_GPIO_InitStruct);

    LL_GPIO_InitStruct.Pin = (RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin);
    LL_GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    LL_GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    LL_GPIO_InitStruct.Alternate = LL_GPIO_AF_11;
    LL_GPIO_Init(GPIOA, &LL_GPIO_InitStruct);
    
    LL_GPIO_InitStruct.Pin = (RMII_TXD1_Pin);
    LL_GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    LL_GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    LL_GPIO_InitStruct.Alternate = LL_GPIO_AF_11;
    LL_GPIO_Init(RMII_TXD1_GPIO_Port, &LL_GPIO_InitStruct);
    
    // LL_GPIO_InitStruct.Pin = (LL_GPIO_PIN_14 | LL_GPIO_PIN_13 | LL_GPIO_PIN_11);
    LL_GPIO_InitStruct.Pin = (RMII_TX_EN_Pin|RMII_TXD0_Pin);
    LL_GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    LL_GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    LL_GPIO_InitStruct.Alternate = LL_GPIO_AF_11;
    LL_GPIO_Init(GPIOG, &LL_GPIO_InitStruct);

    NVIC_SetPriority(ETH_IRQn, PRIORITY_ETH_INT);
    NVIC_EnableIRQ(ETH_IRQn);
    //HAL_NVIC_SetPriority(ETH_IRQn, 5, 0);
    //HAL_NVIC_EnableIRQ(ETH_IRQn);

    /* USER CODE BEGIN ETH_MspInit 1 */

    /* USER CODE END ETH_MspInit 1 */
  }
}

/**
* @brief ETH MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param heth: ETH handle pointer
* @retval None
*/
void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  if(heth->Instance==ETH)
  {
  /* USER CODE BEGIN ETH_MspDeInit 0 */

  /* USER CODE END ETH_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ETH_CLK_DISABLE();
  
    /**ETH GPIO Configuration    
    PG14     ------> ETH_TXD1
    PG13     ------> ETH_TXD0
    PG11     ------> ETH_TX_EN
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PC4     ------> ETH_RXD0
    PA2     ------> ETH_MDIO
    PC5     ------> ETH_RXD1
    PA7     ------> ETH_CRS_DV 
    */
   // HAL_GPIO_DeInit(GPIOG, GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_11);

   // HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);

   // HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);

  /* USER CODE BEGIN ETH_MspDeInit 1 */

  /* USER CODE END ETH_MspDeInit 1 */
  }

}
