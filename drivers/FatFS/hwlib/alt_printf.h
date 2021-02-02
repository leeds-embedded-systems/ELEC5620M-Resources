/******************************************************************************
*
* Copyright 2014 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

/*
 * $Id: //acds/rel/15.0/embedded/ip/hps/altera_hps/hwlib/include/alt_printf.h#1 $
 */

#ifndef __ALT_PRINTF_H__
#define __ALT_PRINTF_H__

#include <stdio.h>
#include <stdlib.h>

/*!
 * Specifies that the ALT_PRINTF() uses the host printf() function as the
 * underlying mechanism.
 */
#ifndef ALT_PRINTF_TARGET_HOST
#define ALT_PRINTF_TARGET_HOST                  (0)
#endif

/*!
 * Specifies that ALT_PRINTF() uses creates the formatted string and sends the
 * output to the UART.
 */
#ifndef ALT_PRINTF_TARGET_UART
#define ALT_PRINTF_TARGET_UART                  (1)
#endif

/*!
 * When targetting the UART, this parameter specifies the largest message that
 * can be output to the UART with a single call to ALT_PRINTF().
 */
#ifndef ALT_PRINTF_UART_MAX_LEN
#define ALT_PRINTF_UART_MAX_LEN                 (80)
#endif

/*!
 * When targetting the UART, this parameter specifies the UART device to use as
 * the output target. Only SoCFPGA UART0 and UART1 can be selected.
 */
#ifndef ALT_PRINTF_UART_SERIAL_PORT
#define ALT_PRINTF_UART_SERIAL_PORT             (ALT_16550_DEVICE_SOCFPGA_UART0)
#endif

/*!
 * When targetting the UART, this parameter specifies the UART baud rate.
 */
#ifndef ALT_PRINTF_UART_BAUD_RATE
#define ALT_PRINTF_UART_BAUD_RATE               (115200)
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#if defined (ALT_PRINTF_TARGET_HOST)

#define ALT_PRINTF printf

#elif defined (ALT_PRINTF_TARGET_UART)

extern char log_buf[];
ALT_STATUS_CODE alt_log_printf(char * str);
#define ALT_PRINTF(...)  do { snprintf(log_buf, ALT_PRINTF_UART_MAX_LEN-1, __VA_ARGS__); alt_log_printf(log_buf); } while (0)

#else

#define ALT_PRINTF

#endif


#ifdef __cplusplus
}
#endif

#endif /* __ALT_PRINTF_H__ */
