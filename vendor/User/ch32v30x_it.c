/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_it.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main Interrupt Service Routines.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/
#include "ch32v30x_it.h"
#include <stdint.h>

// void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
// void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
// void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
// void NMI_Handler(void) __attribute__((naked()));
// void HardFault_Handler(void) __attribute__((naked()));
// void USART1_IRQHandler(void) __attribute__((naked()));
void NMI_Handler(void) __attribute__((interrupt("machine")));
void HardFault_Handler(void) __attribute__((interrupt("machine")));
void USART1_IRQHandler(void) __attribute__((interrupt("machine")));

volatile char USART_rxbuf[USART_RXBUF_SIZE];
volatile uint8_t USART_rxbuf_offset = 0;
volatile uint8_t USART_rxbuf_size = 0;

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
}

void USART1_IRQHandler(void)
{
  if(USART_GetITStatus(USART1, USART_IT_RXNE)) {
      char c;

      USART_ClearITPendingBit(USART1, USART_IT_RXNE);
      c = USART_ReceiveData(USART1);
      USART_rxbuf[USART_rxbuf_offset + USART_rxbuf_size] = c;
      ++USART_rxbuf_size;
      
      while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
      USART_SendData(USART1, c);
  }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}


