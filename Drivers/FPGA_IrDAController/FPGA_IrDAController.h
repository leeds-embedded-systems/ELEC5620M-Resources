/*
 * FPGA IrDA Controller Driver
 * ---------------------------
 *
 * Driver for interfacing with an Altera
 * CSR compatible IrDA controller.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 18/02/2024 | Creation of driver.
 *
 */

#ifndef FPGA_IRDACONTROLLER_H_
#define FPGA_IRDACONTROLLER_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// IrDA Driver
#include "Util/driver_ctx.h"
#include "Util/driver_uart.h"

#include "Util/ct_assert.h"
#include "Util/macros.h"
#include "Util/bit_helpers.h"

typedef enum PACKED {
    FPGA_IrDA_IRQ_NONE       = 0,
    FPGA_IrDA_IRQ_TXEMPTY  = _BV(0),
    FPGA_IrDA_IRQ_RXAVAIL  = _BV(1),
    FPGA_IrDA_IRQ_ALL      = (FPGA_IrDA_IRQ_RXAVAIL | FPGA_IrDA_IRQ_TXEMPTY)
} FPGAIrDAIrqSources;

typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    UartCtx_t uart;
    volatile unsigned int* base;
    bool txRunning;
} FPGAIrDACtx_t, *PFPGAIrDACtx_t;


// Initialise FPGA IrDA driver
//  - base is a pointer to the IrDA CSR peripheral
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t FPGA_IrDA_initialise(void* base, PFPGAIrDACtx_t* pCtx);

// Check if driver initialised
//  - Returns true if driver context previously initialised
bool FPGA_IrDA_isInitialised(PFPGAIrDACtx_t ctx);

// Enable or Disable IrDA interrupt(s)
// - Will enable or disable based on the enable input.
// - Only interrupts with mask bit set will be changed.
HpsErr_t FPGA_IrDA_setInterruptEnable(PFPGAIrDACtx_t ctx, FPGAIrDAIrqSources enable, FPGAIrDAIrqSources mask);

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
HpsErr_t FPGA_IrDA_getInterruptFlags(PFPGAIrDACtx_t ctx, FPGAIrDAIrqSources mask, bool clear);

// Clear IrDA FIFOs
//  - Clears either the TX, RX or both FIFOs.
//  - Data will be deleted. Any pending TX will not be sent.
HpsErr_t FPGA_IrDA_clearDataFifos(PFPGAIrDACtx_t ctx, bool clearTx, bool clearRx);

// Check if there is space for TX data
//  - Returns ERR_NOSPACE if there is no space.
//  - If non-null, returns the amount of empty space in the TX FIFO in *space
HpsErr_t FPGA_IrDA_writeSpace(PFPGAIrDACtx_t ctx, unsigned int* space);

// Perform a IrDA write
//  - A write transfer will be performed
//  - Length is size of data in words.
//  - In 9-bit data mode, data[] is cast to a uint16_t internally.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If the length of the data is not 0<=length<=16, the function will return -1 and write is not performed
//  - If there is not enough space in the FIFO, as many bytes as possible are sent.
//  - The return value indicates number sent.
HpsErr_t FPGA_IrDA_write(PFPGAIrDACtx_t ctx, const uint8_t data[], uint8_t length);

// Check if there is available Rx data
//  - Returns ERR_ISEMPTY if there is nothing available.
//  - If non-null, returns the amount available in the RX FIFO in *available
HpsErr_t FPGA_IrDA_available(PFPGAIrDACtx_t ctx, unsigned int* available);

// Read a word with status info
//  - Reads a single word from the IrDA FIFO
//  - Error information is included in the return struct
UartRxData_t FPGA_IrDA_readWord(PFPGAIrDACtx_t ctx);

// Read from the IrDA FIFO
//  - This will attempt to read multiple bytes from the IrDA FIFO.
//  - Length is size of data in words.
//  - In 9-bit data mode, data[] is cast to a uint16_t internally.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If successful, will return number of bytes read
//  - If the return value is negative, an error occurred in one of the words
HpsErr_t FPGA_IrDA_read(PFPGAIrDACtx_t ctx, uint8_t data[], uint8_t length);

#endif /* FPGA_IRDACONTROLLER_H_ */
