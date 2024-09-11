/********************************** (C) COPYRIGHT  *******************************
* File Name          : debug.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : This file contains all the functions prototypes for UART
*                      Printf, Delay functions.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

#include "stdio.h"
#include "ch32v30x.h"

// UART Printf Definition
#define DEBUG_UART1    1
#define DEBUG_UART2    2
#define DEBUG_UART3    3

// DEBUG UATR Definition 
#define DEBUG   DEBUG_UART1

void Delay_Init(void);
void Delay_Us(uint32_t n);
void Delay_Ms(uint32_t n);
void USART_Printf_Init(uint32_t baudrate);

#endif // DEBUG_H
