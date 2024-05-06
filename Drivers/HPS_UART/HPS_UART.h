/*
 * HPS UART Driver
 * ---------------
 *
 * Driver for the HPS embedded UART controller.
 * 
 * Basic functionality is thus far implemented. More
 * complex features like the full modem signals and
 * DMA configuration have not yet been implemented.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 01/04/2024 | Creation of driver.
 *
 */

#ifndef HPS_UART_H_
#define HPS_UART_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// UART Driver
#include "Util/driver_ctx.h"
#include "Util/driver_uart.h"

#include "Util/ct_assert.h"
#include "Util/macros.h"
#include "Util/bit_helpers.h"

typedef enum {
    HPS_UART_RXTHRESH_CHAR1 = 0, // Single character in FIFO
    HPS_UART_RXTHRESH_QUART = 1, // Quarter full
    HPS_UART_RXTHRESH_HALF  = 2, // Half full
    HPS_UART_RXTHRESH_FULL2 = 3  // Almost full. Space for two characters
} HPSUARTRxThreshold;

typedef enum {
    HPS_UART_TXTHRESH_EMPTY = 0, // FIFO Empty
    HPS_UART_TXTHRESH_CHAR2 = 1, // Two characters in FIFO
    HPS_UART_TXTHRESH_QUART = 2, // Quarter full
    HPS_UART_TXTHRESH_HALF  = 3  // Half full
} HPSUARTTxThreshold;

typedef enum {
    HPS_UART_DATASIZE_5BIT = 0,
    HPS_UART_DATASIZE_6BIT = 1,
    HPS_UART_DATASIZE_7BIT = 2,
    HPS_UART_DATASIZE_8BIT = 3
} HPSUARTDataSize;

typedef enum {
    HPS_UART_STOPBIT_ONE = 0,
    HPS_UART_STOPBIT_TWO = 1  // 1.5 stop bits for 5bit data, or 2 stop bits otherwise
} HPSUARTStopBits;

typedef enum {
    HPS_UART_FLOWCTRL_DISABLED = 0,
    HPS_UART_FLOWCTRL_ENABLED  = 1  // Automatic flow control enable
} HPSUARTFlowCntrl;

typedef enum {
    HPS_UART_IRQ_NONE    = 0,
    // Bits match LSR register flags
    HPS_UART_IRQ_RXAVAIL = _BV(0),
    HPS_UART_IRQ_NOSPACE = _BV(1),
    HPS_UART_IRQ_PARITY  = _BV(2),
    HPS_UART_IRQ_FRAMING = _BV(3),
    HPS_UART_IRQ_BREAK   = _BV(4),
    HPS_UART_IRQ_TXEMPTY = _BV(6),
    HPS_UART_IRQ_RXFIFO  = _BV(7), // Cannot generate IRQ
    // Extra IRQ sources not in LSR
    HPS_UART_IRQ_MODEM   = _BV(8),
    // Combined Maskes
    HPS_UART_IRQ_ERRORS  = (HPS_UART_IRQ_FRAMING | HPS_UART_IRQ_PARITY  | HPS_UART_IRQ_NOSPACE | HPS_UART_IRQ_BREAK),
    HPS_UART_IRQ_FIFOS   = (HPS_UART_IRQ_RXAVAIL | HPS_UART_IRQ_TXEMPTY),
    HPS_UART_IRQ_ALL     = (HPS_UART_IRQ_FIFOS   | HPS_UART_IRQ_ERRORS  | HPS_UART_IRQ_RXFIFO  | HPS_UART_IRQ_MODEM)
} HPSUARTIrqSources;

typedef struct {
    //Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    unsigned int baudClk;
    // UART common interface
    UartCtx_t uart;
    // Config
    unsigned int fifoSize;
    unsigned int clkDiv;
    bool txRunning;
    // IRQ shadow
    HPSUARTIrqSources irqFlags;
    unsigned int modemStat;
} HPSUARTCtx_t, *PHPSUARTCtx_t;


// Initialise HPS UART driver
//  - base is a pointer to the UART peripheral
//  - periphClk is the peripheral clock rate in Hz (e.g. 100E6 = 100MHz for DE1-SoC)
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t HPS_UART_initialise(void* base, unsigned int periphClk, PHPSUARTCtx_t* pCtx);

// Check if driver initialised
//  - Returns true if driver context previously initialised
bool HPS_UART_isInitialised(PHPSUARTCtx_t ctx);

// Enable or Disable UART interrupt(s)
// - Will enable or disable based on the enable input.
// - Only interrupts with mask bit set will be changed.
HpsErr_t HPS_UART_setInterruptEnable(PHPSUARTCtx_t ctx, HPSUARTIrqSources enable, HPSUARTIrqSources mask);

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
HpsErr_t HPS_UART_getInterruptFlags(PHPSUARTCtx_t ctx, HPSUARTIrqSources mask, bool clear);

// Set the UART Baud Rate
// - Returns the achieved baud rate
HpsErr_t HPS_UART_setBaudRate(PHPSUARTCtx_t ctx, unsigned int baudRate);

// Check UART Operation mode
//  - Always returns full-duplex
HpsErr_t HPS_UART_getTransferMode(PHPSUARTCtx_t ctx);

// Set the UART Data Format
//  - Sets the data format to be used.
//  - The format can select the data width, and the parity mode.
//  - Stop bit count can be set to 1 or 1.5/2 based on enum values.
HpsErr_t HPS_UART_setDataFormat(PHPSUARTCtx_t ctx, unsigned int dataWidth, UartParity parityEn, HPSUARTStopBits stopBits, HPSUARTFlowCntrl flowctrl);

// Clear UART FIFOs
//  - Clears either the TX, RX or both FIFOs.
//  - Data will be deleted. Any pending TX will not be sent.
HpsErr_t HPS_UART_clearDataFifos(PHPSUARTCtx_t ctx, bool clearTx, bool clearRx);

// Check if there is space for TX data
//  - Returns ERR_NOSPACE if there is no space.
//  - If non-null, returns the amount of empty space in the TX FIFO in *space
HpsErr_t HPS_UART_writeSpace(PHPSUARTCtx_t ctx, unsigned int* space);

// Perform a UART write
//  - A write transfer will be performed
//  - Length is size of data in words.
//  - The data[] parameter must be an array of at least 'length' bytes.
//  - If there is not enough space in the FIFO, as many bytes as possible are sent.
//  - The return value indicates number sent.
HpsErr_t HPS_UART_write(PHPSUARTCtx_t ctx, const uint8_t data[], uint8_t length);

// Check if there is available Rx data
//  - Returns ERR_ISEMPTY if there is nothing available.
//  - If non-null, returns the amount available in the RX FIFO in *available
HpsErr_t HPS_UART_available(PHPSUARTCtx_t ctx, unsigned int* available);

// Read a word with status info
//  - Reads a single word from the UART FIFO
//  - Error information is included in the return struct
UartRxData_t HPS_UART_readWord(PHPSUARTCtx_t ctx);

// Read from the UART FIFO
//  - This will attempt to read multiple bytes from the UART FIFO.
//  - Length is size of data in words.
//  - In 9-bit data mode, data[] is cast to a uint16_t internally.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If successful, will return number of bytes read
//  - If the return value is negative, an error occurred in one of the words
HpsErr_t HPS_UART_read(PHPSUARTCtx_t ctx, uint8_t data[], uint8_t length);

#endif /* HPS_UART_H_ */
