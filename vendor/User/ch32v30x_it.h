/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_it.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : This file contains the headers of the interrupt handlers.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/
#ifndef __CH32V30x_IT_H
#define __CH32V30x_IT_H

#include "debug.h"
#include <stdint.h>

#define USART_RXBUF_SIZE 256

extern volatile char USART_rxbuf[USART_RXBUF_SIZE];
extern volatile uint8_t USART_rxbuf_offset;
extern volatile uint8_t USART_rxbuf_size;

#endif /* __CH32V30x_IT_H */


