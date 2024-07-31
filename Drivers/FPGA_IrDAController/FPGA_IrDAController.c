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

#include "FPGA_IrDAController.h"

/*
 *
 * CSR Map
 * -------------------
 * The following bit mapping is used for the CSR:
 *
 *  Address | Direction | Bits  | Meaning
 * ---------+-----------+-------+-------------------------------------------------------------------------------------------
 *     0    |   Rd/Wr   | 15: 0 | Data FIFOs - You **must** perform at least a 16bit access on txData and rxData for FIFO
 *          |    Wr     |  8: 0 | txData - Write to this byte to load next byte of data into TX FIFO
 *          |    Rd     |  8: 0 | rxData - Read from this to fetch next byte of data from RX FIFO
 *          |    Rd     |     9 | rxDataParityError - Whether or not the byte just read had a parity error
 *          |    Rd     |    15 | rxDataValid - Whether or not the byte just was valid
 *          |    Rd     | 23:16 | rxAvailable - The number of bytes in the RX FIFO left to be read
 *     1    |   Rd/Wr   |     0 | irqEnable(rxAvailable) - IRQ Enable Mask, when set to 1, irq. can cause an interrupt.
 *          |   Rd/Wr   |     1 | irqEnable(txEmpty) - IRQ Enable Mask, when set to 1, irq. can cause an interrupt.
 *          |   Rd/Wr   |     8 | irqAsserted(rxAvailable) - RX data available. High when detected. Write a 1 to clear.
 *          |   Rd/Wr   |     9 | irqAsserted(txEmpty) - TX buffer empty. High when detected. Write a 1 to clear.
 *          |    Rd     | 23:16 | txSpace - Space left in FIFO for TX data bytes
 *          |   Rd/Wr   |    24 | clearTXFIFO - Set high to reset the TX FIFO. This does *not* self-clear. Powers up high
 *          |   Rd/Wr   |    25 | clearRXFIFO - Set high to reset the RX FIFO. This does *not* self-clear. Powers up high
 */

/*
 * Registers
 */

// CSR Registers
#define FPGAIRDA_REG_TXFIFO(base)     (*(((volatile uint16_t*)(base)) + (0x00 / sizeof(uint16_t))))
#define FPGAIRDA_REG_RXFIFO(base)     (*(((volatile uint16_t*)(base)) + (0x00 / sizeof(uint16_t))))
#define FPGAIRDA_REG_RXAVAIL(base)    (*(((volatile uint8_t *)(base)) + (0x02 / sizeof(uint8_t ))))
#define FPGAIRDA_REG_IRQMASK(base)    (*(((volatile uint8_t *)(base)) + (0x04 / sizeof(uint8_t ))))
#define FPGAIRDA_REG_IRQFLAGS(base)   (*(((volatile uint8_t *)(base)) + (0x05 / sizeof(uint8_t ))))
#define FPGAIRDA_REG_TXSPACE(base)    (*(((volatile uint8_t *)(base)) + (0x06 / sizeof(uint8_t ))))
#define FPGAIRDA_REG_CLEARFIFO(base)  (*(((volatile uint8_t *)(base)) + (0x07 / sizeof(uint8_t ))))

// Register bits
#define FPGAIRDA_FIFO_DATA_MASK     0xFF // Both Tx and Rx FIFOs
#define FPGAIRDA_FIFO_DATA_OFFS     0
#define FPGAIRDA_RXFIFO_PARITY_MASK 0x1U
#define FPGAIRDA_RXFIFO_PARITY_OFFS 9
#define FPGAIRDA_RXFIFO_VALID_MASK  0x1U
#define FPGAIRDA_RXFIFO_VALID_OFFS  15
#define FPGAIRDA_CLEARFIFO_TX_MASK  0x1U
#define FPGAIRDA_CLEARFIFO_TX_OFFS  0
#define FPGAIRDA_CLEARFIFO_RX_MASK  0x1U
#define FPGAIRDA_CLEARFIFO_RX_OFFS  1
#define FPGAIRDA_FLAGS_IRQ_MASK     FPGA_IrDA_IRQ_ALL
#define FPGAIRDA_FLAGS_IRQ_OFFS     0

/*
 * Internal Functions
 */

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
static HpsErr_t _FPGA_IrDA_getInterruptFlags(PFPGAIrDACtx_t ctx, FPGAIrDAIrqSources mask, bool clear) {
    // Read in the flags
    FPGAIrDAIrqSources flags = MaskExtract(FPGAIRDA_REG_IRQFLAGS(ctx->base), FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
    // Check if tx is finished whenever flags are read
    if (flags & FPGA_IrDA_IRQ_TXEMPTY) {
        ctx->txRunning = false;
    }
    // Keep only flags we care about
    flags = flags & mask;
    // Clear if required
    if (clear) FPGAIRDA_REG_IRQFLAGS(ctx->base) = MaskCreate(flags, FPGAIRDA_FLAGS_IRQ_OFFS);
    // Return the flags
    return (HpsErr_t)flags;
}

static unsigned int _FPGA_IrDA_writeSpace(PFPGAIrDACtx_t ctx) {
    return FPGAIRDA_REG_TXSPACE(ctx->base);
}

static HpsErr_t _FPGA_IrDA_write(PFPGAIrDACtx_t ctx, const uint8_t* data, uint8_t length) {
    int written = 0;
    if (!length) return 0;
    // Ensure the TX empty flag if clear so we can detect end of run
    // after our write.
    _FPGA_IrDA_getInterruptFlags(ctx, FPGA_IrDA_IRQ_TXEMPTY, true);
    // Mark as running if we are writing a non-zero amount
    ctx->txRunning = true;
    // Start the new transfer
    while(length--) {
        if (_FPGA_IrDA_writeSpace(ctx) == 0) {
            //End write if FIFO ran out of space.
            break;
        }
        uint16_t wrVal;
        if (ctx->uart.is9bit) {
            wrVal = *(uint16_t*)data;
            data += sizeof(uint16_t);
        } else {
            wrVal = (uint16_t)*data++;
        }
        FPGAIRDA_REG_TXFIFO(ctx->base) = MaskInsert(wrVal, FPGAIRDA_FIFO_DATA_MASK, FPGAIRDA_FIFO_DATA_OFFS);
        written++;
    }
    return written;
}

static unsigned int _FPGA_IrDA_available(PFPGAIrDACtx_t ctx) {
    return FPGAIRDA_REG_RXAVAIL(ctx->base);
}

static void _FPGA_IrDA_readWord(PFPGAIrDACtx_t ctx, UartRxData_t* data) {
    // Check if we have anything to read
    uint16_t rxReg = FPGAIRDA_REG_RXFIFO(ctx->base);
    data->valid        = MaskExtract(rxReg, FPGAIRDA_RXFIFO_VALID_MASK, FPGAIRDA_RXFIFO_VALID_OFFS);
    data->partityError = MaskExtract(rxReg, FPGAIRDA_RXFIFO_PARITY_MASK, FPGAIRDA_RXFIFO_PARITY_OFFS);
    data->frameError   = false;
    data->rxData       = MaskExtract(rxReg, FPGAIRDA_FIFO_DATA_MASK, FPGAIRDA_FIFO_DATA_OFFS);
}

static HpsErr_t _FPGA_IrDA_read(PFPGAIrDACtx_t ctx, uint8_t* data, uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Try to read length worth of data
    int numRead = 0;
    bool parityErr = false;
    while(length--) {
        UartRxData_t readData;
        _FPGA_IrDA_readWord(ctx, &readData);
        if (!readData.valid) {
            //End read if FIFO ran out of data.
            break;
        }
        // Check for error flags
        parityErr = parityErr || readData.partityError;
        // Read into the buffer with correct word size
        if (ctx->uart.is9bit){
            *(uint16_t*)data = readData.rxData;
            data += sizeof(uint16_t);
        } else {
            *data++ = readData.rxData;
        }
        // One more read
        numRead++;
    }
    return (parityErr ? ERR_CHECKSUM : numRead);
}

static void _FPGA_IrDA_cleanup(PFPGAIrDACtx_t ctx) {
    //Disable interrupts and reset FIFOs
    if (ctx->base) {
        FPGAIRDA_REG_IRQMASK(ctx->base)   = 0;
        FPGAIRDA_REG_IRQFLAGS(ctx->base)  = MaskCreate(FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
        FPGAIRDA_REG_CLEARFIFO(ctx->base) = MaskCreate(FPGAIRDA_CLEARFIFO_RX_MASK, FPGAIRDA_CLEARFIFO_RX_OFFS) |
                                            MaskCreate(FPGAIRDA_CLEARFIFO_TX_MASK, FPGAIRDA_CLEARFIFO_TX_OFFS) ;
    }
}

// APIs used for generic IrDA interface
static HpsErr_t _FPGA_IrDA_txFifoSpace(PFPGAIrDACtx_t ctx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Return FIFO space
    return (HpsErr_t)_FPGA_IrDA_writeSpace(ctx);
}

static HpsErr_t _FPGA_IrDA_rxFifoAvailable(PFPGAIrDACtx_t ctx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Return FIFO availability
    return (HpsErr_t)_FPGA_IrDA_available(ctx);
}

static HpsErr_t _FPGA_IrDA_txIdle(PFPGAIrDACtx_t ctx, bool clearFlag) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Check the Tx empty IRQ (clearing flag if requested). This will also update txRunning flag
    _FPGA_IrDA_getInterruptFlags(ctx, FPGA_IrDA_IRQ_TXEMPTY, clearFlag);
    // Return whether TX is running
    return !ctx->txRunning;
}

static HpsErr_t _FPGA_IrDA_rxReady(PFPGAIrDACtx_t ctx, bool clearFlag) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Ready whenever there is data available
    return _FPGA_IrDA_available(ctx) > 0;
}


/*
 * User Facing APIs
 */

// Initialise FPGA IrDA driver
//  - csr is a pointer to the IrDA CSR peripheral
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t FPGA_IrDA_initialise(void* csr, PFPGAIrDACtx_t* pCtx) {
    //Ensure user pointers valid
    if (!csr) return ERR_NULLPTR;
    if (!pointerIsAligned(csr, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_FPGA_IrDA_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    PFPGAIrDACtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)csr;
    //Setup IrDA driver
    ctx->uart.ctx = ctx;
    ctx->uart.is9bit = false;
    ctx->uart.transmit        = (UartTxFunc_t       )FPGA_IrDA_write;
    ctx->uart.receive         = (UartRxFunc_t       )FPGA_IrDA_read;
    ctx->uart.txIdle          = (UartStatusFunc_t   )_FPGA_IrDA_txIdle;
    ctx->uart.rxReady         = (UartStatusFunc_t   )_FPGA_IrDA_rxReady;
    ctx->uart.txFifoSpace     = (UartFifoSpaceFunc_t)_FPGA_IrDA_txFifoSpace;
    ctx->uart.rxFifoAvailable = (UartFifoSpaceFunc_t)_FPGA_IrDA_rxFifoAvailable;
    ctx->uart.clearFifos      = (UartFifoClearFunc_t)FPGA_IrDA_clearDataFifos;
    //Disable and clear any IRQ flags, and release the FIFOs
    FPGAIRDA_REG_IRQMASK(ctx->base)   = 0;
    FPGAIRDA_REG_IRQFLAGS(ctx->base)  = MaskCreate(FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
    FPGAIRDA_REG_CLEARFIFO(ctx->base) = 0;
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver context previously initialised
bool FPGA_IrDA_isInitialised(PFPGAIrDACtx_t ctx) {
    return DriverContextCheckInit(ctx);
}


// Enable or Disable IrDA interrupt(s)
// - Will enable or disable based on the enable input.
// - Only interrupts with mask bit set will be changed.
HpsErr_t FPGA_IrDA_setInterruptEnable(PFPGAIrDACtx_t ctx, FPGAIrDAIrqSources enable, FPGAIrDAIrqSources mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Clear any flags for IRQs we are changing
    FPGAIRDA_REG_IRQFLAGS(ctx->base) = MaskInsert(mask, FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
    // And modify the enable mask
    FPGAIrDAIrqSources curFlags = (FPGAIrDAIrqSources)MaskExtract(FPGAIRDA_REG_IRQMASK(ctx->base), FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
    FPGAIRDA_REG_IRQMASK(ctx->base) = MaskInsert((enable & mask) | (curFlags & ~mask), FPGAIRDA_FLAGS_IRQ_MASK, FPGAIRDA_FLAGS_IRQ_OFFS);
    return ERR_SUCCESS;
}

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
HpsErr_t FPGA_IrDA_getInterruptFlags(PFPGAIrDACtx_t ctx, FPGAIrDAIrqSources mask, bool clear) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Access the flags
    return _FPGA_IrDA_getInterruptFlags(ctx, mask, clear);
}

// Clear IrDA FIFOs
//  - Clears either the TX, RX or both FIFOs.
//  - Data will be deleted. Any pending TX will not be sent.
HpsErr_t FPGA_IrDA_clearDataFifos(PFPGAIrDACtx_t ctx, bool clearTx, bool clearRx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Assert the FIFO reset flags
    FPGAIRDA_REG_CLEARFIFO(ctx->base) = MaskInsert(clearTx, FPGAIRDA_CLEARFIFO_TX_MASK, FPGAIRDA_CLEARFIFO_TX_OFFS) |
                                        MaskInsert(clearRx, FPGAIRDA_CLEARFIFO_RX_MASK, FPGAIRDA_CLEARFIFO_RX_OFFS);
    // Then clear them again
    FPGAIRDA_REG_CLEARFIFO(ctx->base) = 0;
    return ERR_SUCCESS;
}

// Check if there is space for TX data
//  - Returns ERR_NOSPACE if there is no space.
//  - If non-null, returns the amount of empty space in the TX FIFO in *space
HpsErr_t FPGA_IrDA_writeSpace(PFPGAIrDACtx_t ctx, unsigned int* space) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Check the space
    unsigned int _space = _FPGA_IrDA_writeSpace(ctx);
    if (space) *space = _space;
    return _space ? ERR_SUCCESS : ERR_NOSPACE;
}

// Perform a IrDA write
//  - A write transfer will be performed
//  - Length is size of data in words.
//  - In 9-bit data mode, data[] is cast to a uint16_t internally.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If the length of the data is not 0<=length<=16, the function will return -1 and write is not performed
//  - If there is not enough space in the FIFO, as many bytes as possible are sent.
//  - The return value indicates number sent.
HpsErr_t FPGA_IrDA_write(PFPGAIrDACtx_t ctx, const uint8_t data[], uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // And read
    return _FPGA_IrDA_write(ctx, data, length);
}

// Check if there is available Rx data
//  - Returns ERR_ISEMPTY if there is nothing available.
//  - If non-null, returns the amount available in the RX FIFO in *available
HpsErr_t FPGA_IrDA_available(PFPGAIrDACtx_t ctx, unsigned int* available) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Read the FIFO available level
    unsigned int _available = _FPGA_IrDA_available(ctx);
    if (available) *available = _available;
    return _available ? ERR_SUCCESS : ERR_ISEMPTY;
}

// Read from the IrDA FIFO
//  - Reads a single word from the IrDA FIFO
//  - Error information is included in the return struct
UartRxData_t FPGA_IrDA_readWord(PFPGAIrDACtx_t ctx) {
    UartRxData_t data = {0};
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_SUCCESS(status)) {
        // And read
        _FPGA_IrDA_readWord(ctx, &data);
    }
    return data;
}

// Read multiple from IrDA FIFO
//  - This will attempt to read multiple bytes from the IrDA FIFO.
//  - Length is size of data in words.
//  - In 9-bit data mode, data[] is cast to a uint16_t internally.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If successful, will return number of bytes read
//  - If the return value is negative, an error occurred in one of the words
HpsErr_t FPGA_IrDA_read(PFPGAIrDACtx_t ctx, uint8_t data[], uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // And read
    return _FPGA_IrDA_read(ctx, data, length);
}
