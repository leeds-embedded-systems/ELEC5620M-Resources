/*
 * HPS UART Driver
 * ---------------
 *
 * Driver for the HPS embedded UART controller
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

#include "HPS_UART.h"

#include "Util/bit_helpers.h"
#include "Util/macros.h"

/*
 * Registers
 */

#define HPS_UART_L4_SP_CLK_DIVISOR 16

// Internal IRQ ID define. Will be mapped to flags define as needed.
typedef enum {
    HPS_UART_IRQID_MODEMSTAT    = 0,
    HPS_UART_IRQID_NOINTRPENDNG = 1,
    HPS_UART_IRQID_THREMPTY     = 2,
    HPS_UART_IRQID_RXDATAAVAIL  = 4,
    HPS_UART_IRQID_RXLINESTAT   = 6,
    HPS_UART_IRQID_BUSYDETECT   = 7,
    HPS_UART_IRQID_CHARTIMEOUT  = 12,
    HPS_UART_IRQID_MASK = 0xF,
} HPSUARTInterruptId;

// Registers in UART controller
#define HPS_UART_REG_RBRTHRDLL (0x00/sizeof(unsigned int))
#define HPS_UART_REG_IERDLH    (0x04/sizeof(unsigned int))
#define HPS_UART_REG_IRQID     (0x08/sizeof(unsigned int))
#define HPS_UART_REG_FIFOCTRL  (0x08/sizeof(unsigned int))
#define HPS_UART_REG_LINECTRL  (0x0C/sizeof(unsigned int))
#define HPS_UART_REG_MODEMCTRL (0x10/sizeof(unsigned int))
#define HPS_UART_REG_LINESTAT  (0x14/sizeof(unsigned int))
#define HPS_UART_REG_MODEMSTAT (0x18/sizeof(unsigned int))
#define HPS_UART_REG_DMABURST  (0x30/sizeof(unsigned int))  // 16 consecutive 32-bit registers (0x30-0x6F) to allow DMA controller burst access.
                                                            // All regs access the head of the Rx FIFO (or buffer) on read, and Tx FIFO on write.
#define HPS_UART_REG_STATUS    (0x7C/sizeof(unsigned int))
#define HPS_UART_REG_TXFILL    (0x80/sizeof(unsigned int))
#define HPS_UART_REG_RXFILL    (0x84/sizeof(unsigned int))
#define HPS_UART_REG_SOFTRST   (0x88/sizeof(unsigned int))
#define HPS_UART_REG_RTSCTRL   (0x8C/sizeof(unsigned int))
#define HPS_UART_REG_BREAKCTRL (0x90/sizeof(unsigned int))
#define HPS_UART_REG_DMAMODE   (0x94/sizeof(unsigned int))
#define HPS_UART_REG_FIFOENBL  (0x98/sizeof(unsigned int))
#define HPS_UART_REG_RXTRIG    (0x9C/sizeof(unsigned int))
#define HPS_UART_REG_TXETRIG   (0xA0/sizeof(unsigned int))
#define HPS_UART_REG_HALTTX    (0xA4/sizeof(unsigned int))
#define HPS_UART_REG_DMASACK   (0xA8/sizeof(unsigned int))
#define HPS_UART_REG_PARAMS    (0xF4/sizeof(unsigned int)) // Read-Only. See below.
// 0xF4 provides capabilities of the controller. These are fixed silicon parameters. 
// For the A10/CV SoC we target, the following are set:
//  - FIFO_MODE               = 128bytes (8    )
//  - DMA_EXTRA               = Enabled  (TRUE )
//  - UART_ADD_ENCODED_PARAMS = Enabled  (TRUE )
//  - SHADOW                  = Enabled  (TRUE )
//  - FIFO_STAT               = Enabled  (TRUE )
//  - FIFO_ACCESS             = Enabled  (TRUE )
//  - ADDITIONAL_FEATURES     = Enabled  (TRUE )
//  - SIR_LP_MODE             = Disabled (FALSE)
//  - SIR_MODE                = Disabled (FALSE)
//  - THRE_MODE               = Enabled  (TRUE )
//  - ACFE_MODE               = Enabled  (TRUE )
//  - APB_DATA_WIDTH          = 32bit    (2    )

// RBR/THR/DLL Fields
//  - DLL when DLAB (LineCtl[7]) == 1 (Divisor Low Byte)
#define HPS_UART_RBRTHRDLL_DLL_MASK   0xFF
#define HPS_UART_RBRTHRDLL_DLL_OFFS   0
//  - Else:
//     - RBR on read  (Rx data in)
//     - THR on write (Tx data out)
#define HPS_UART_RBRTHRDLL_RX_MASK    0xFF // Rx in register (read-only)
#define HPS_UART_RBRTHRDLL_RX_OFFS    0
#define HPS_UART_RBRTHRDLL_TX_MASK    0xFF // Tx out register (write-only)
#define HPS_UART_RBRTHRDLL_TX_OFFS    0

// IER/DLH Fields
//  - DLH when DLAB (LineCtl[7]) == 1 (Divisor High Byte)
#define HPS_UART_IERDLH_DLH_MASK      0xFF
#define HPS_UART_IERDLH_DLH_OFFS      0
//  - IER otherwise (Interrupt enable)
#define HPS_UART_IERDLH_ERBFI_MASK    0x1  // Rx available and character timeout IRQ En
#define HPS_UART_IERDLH_ERBFI_OFFS    0
#define HPS_UART_IERDLH_ETBEI_MASK    0x1  // Tx buffer empty IRQ En
#define HPS_UART_IERDLH_ETBEI_OFFS    1
#define HPS_UART_IERDLH_ELSI_MASK     0x1  // Rx line status IRQ En
#define HPS_UART_IERDLH_ELSI_OFFS     2
#define HPS_UART_IERDLH_EDSSI_MASK    0x1  // Modem status IRQ En
#define HPS_UART_IERDLH_EDSSI_OFFS    3
#define HPS_UART_IERDLH_PTIME_MASK    0x1  // Threshold mode IRQ En
#define HPS_UART_IERDLH_PTIME_OFFS    7

// IRQ ID Fields
//  - Read Only
#define HPS_UART_IRQID_ID_MASK        HPS_UART_IRQID_MASK  // Current IRQ ID
#define HPS_UART_IRQID_ID_OFFS        0
#define HPS_UART_IRQID_FIFOEN_MASK    0x3  // Indicates whether FIFOs are enabled
#define HPS_UART_IRQID_FIFOEN_OFFS    6

// FIFO Control Fields
//  - Write Only
#define HPS_UART_FIFOCTRL_FIFOE_MASK  0x1  // FIFO enable
#define HPS_UART_FIFOCTRL_FIFOE_OFFS  0
#define HPS_UART_FIFOCTRL_RFIFOR_MASK 0x1  // Receive FIFO reset (self-clear)
#define HPS_UART_FIFOCTRL_RFIFOR_OFFS 1
#define HPS_UART_FIFOCTRL_XFIFOR_MASK 0x1  // Transmit FIFO reset (self-clear)
#define HPS_UART_FIFOCTRL_XFIFOR_OFFS 2
#define HPS_UART_FIFOCTRL_DMAM_MASK   0x1  // DMA mode
#define HPS_UART_FIFOCTRL_DMAM_OFFS   3
#define HPS_UART_FIFOCTRL_TET_MASK    0x3  // Tx empty trigger
#define HPS_UART_FIFOCTRL_TET_OFFS    4
#define HPS_UART_FIFOCTRL_RT_MASK     0x3  // Rx threshold trigger
#define HPS_UART_FIFOCTRL_RT_OFFS     6

// Line Control Fields
#define HPS_UART_LINECTRL_DLS_MASK    0x3  // Data Length
#define HPS_UART_LINECTRL_DLS_OFFS    0
#define HPS_UART_LINECTRL_STOP_MASK   0x1  // Number of stop bits
#define HPS_UART_LINECTRL_STOP_OFFS   2
#define HPS_UART_LINECTRL_PEN_MASK    0x1  // Enable Parity
#define HPS_UART_LINECTRL_PEN_OFFS    3
#define HPS_UART_LINECTRL_EPS_MASK    0x1  // Even Parity Select. Used only if parity enabled
#define HPS_UART_LINECTRL_EPS_OFFS    4
#define HPS_UART_LINECTRL_SP_MASK     0x1  // Stick parity (Altera don't seem to know what this does...)
#define HPS_UART_LINECTRL_SP_OFFS     5
#define HPS_UART_LINECTRL_BREAK_MASK  0x1  // Assert the SIR line low to signal a break condition
#define HPS_UART_LINECTRL_BREAK_OFFS  6
#define HPS_UART_LINECTRL_DLAB_MASK   0x1  // Divisor Access Bit (enables access to {DLH,DLL})
#define HPS_UART_LINECTRL_DLAB_OFFS   7

// Modem Control Fields
#define HPS_UART_MODEMCTRL_DTR_MASK   0x1  // Write 1 to assert DTRn signal low.
#define HPS_UART_MODEMCTRL_DTR_OFFS   0
#define HPS_UART_MODEMCTRL_RTS_MASK   0x1  // Write 1 to assert RTSn signal low.
#define HPS_UART_MODEMCTRL_RTS_OFFS   1
#define HPS_UART_MODEMCTRL_OUT1_MASK  0x1  // Write 1 to assert OUT1n signal low.
#define HPS_UART_MODEMCTRL_OUT1_OFFS  2
#define HPS_UART_MODEMCTRL_OUT2_MASK  0x1  // Write 1 to assert OUT2n signal low.
#define HPS_UART_MODEMCTRL_OUT2_OFFS  3
#define HPS_UART_MODEMCTRL_LPBK_MASK  0x1  // Loopback mode enable
#define HPS_UART_MODEMCTRL_LPBK_OFFS  4
#define HPS_UART_MODEMCTRL_AFCE_MASK  0x1  // Automatic flow control enable
#define HPS_UART_MODEMCTRL_AFCE_OFFS  5
#define HPS_UART_MODEMCTRL_SIR_MASK   0x1  // Enable IrDA SIR compatible mode
#define HPS_UART_MODEMCTRL_SIR_OFFS   6

// Line Status Fields
#define HPS_UART_LINESTAT_DR_MASK     0x3  // Data Ready (receive buffer or FIFO contains at least one character)
#define HPS_UART_LINESTAT_DR_OFFS     0
#define HPS_UART_LINESTAT_OE_MASK     0x1  // Receive buffer overrun error
#define HPS_UART_LINESTAT_OE_OFFS     1
#define HPS_UART_LINESTAT_PE_MASK     0x1  // Parity error in receive word since last check of LINESTAT
#define HPS_UART_LINESTAT_PE_OFFS     2
#define HPS_UART_LINESTAT_FE_MASK     0x1  // Framing error in receive word since last check of LINESTAT
#define HPS_UART_LINESTAT_FE_OFFS     3
#define HPS_UART_LINESTAT_BI_MASK     0x1  // Break interrupt detected (Rx line held low for >1 packet)
#define HPS_UART_LINESTAT_BI_OFFS     4
#define HPS_UART_LINESTAT_THRE_MASK   0x1  // If threshold mode enabled (IERDLH[7]), indicates Tx FIFO Full. Else indicates Tx FIFO or holding register empty.
#define HPS_UART_LINESTAT_THRE_OFFS   5
#define HPS_UART_LINESTAT_TEMT_MASK   0x1  // Transmitter empty bit. Holding register/FIFO empty, and nothing being transmitted
#define HPS_UART_LINESTAT_TEMT_OFFS   6
#define HPS_UART_LINESTAT_RFE_MASK    0x1  // Receiver FIFO error. If FIFO moode enabled, indicates at least on entry in FIFO has FE/PE/BI
#define HPS_UART_LINESTAT_RFE_OFFS    7
// All line status fields used as IRQ sources
#define HPS_UART_LINESTAT_ALL_MASK    HPS_UART_IRQ_ALL
#define HPS_UART_LINESTAT_ALL_OFFS    0

// Modem Control Fields
#define HPS_UART_MODEMSTAT_DCTS_MASK  0x1  // Indicates CTSn has changed since last MSR read
#define HPS_UART_MODEMSTAT_DCTS_OFFS  0
#define HPS_UART_MODEMSTAT_DDSR_MASK  0x1  // Indicates DSRn has changed since last MSR read
#define HPS_UART_MODEMSTAT_DDSR_OFFS  1
#define HPS_UART_MODEMSTAT_TERI_MASK  0x1  // Ring indicator has deasserted since last MSR read. Oh, and it's not teri's, it's mine.
#define HPS_UART_MODEMSTAT_TERI_OFFS  2
#define HPS_UART_MODEMSTAT_DDCD_MASK  0x1  // Indicates DCDn has changed since last MSR read
#define HPS_UART_MODEMSTAT_DDCD_OFFS  3
#define HPS_UART_MODEMSTAT_CTS_MASK   0x1  // Indicates CTSn (Clear To Send) is asserted low
#define HPS_UART_MODEMSTAT_CTS_OFFS   4
#define HPS_UART_MODEMSTAT_DSR_MASK   0x1  // Indicates DSRn (Data Set Ready) is asserted low
#define HPS_UART_MODEMSTAT_DSR_OFFS   5
#define HPS_UART_MODEMSTAT_RI_MASK    0x1  // Indicates RIn (Ring Indicator) is asserted low
#define HPS_UART_MODEMSTAT_RI_OFFS    6
#define HPS_UART_MODEMSTAT_DCD_MASK   0x1  // Indicates DCDn (Data Carrier Detect) is asserted low
#define HPS_UART_MODEMSTAT_DCD_OFFS   7

// DMA Burst Access/TxRx Shadow Register
//  - RBR on read  (Rx data in)
//  - THR on write (Tx data out)
#define HPS_UART_DMABURST_RX_MASK    0xFF // Rx in register (read-only)
#define HPS_UART_DMABURST_RX_OFFS    0
#define HPS_UART_DMABURST_TX_MASK    0xFF // Tx out register (write-only)
#define HPS_UART_DMABURST_TX_OFFS    0

// Status flags
#define HPS_UART_STATUS_BUSY    0
#define HPS_UART_STATUS_TXSPACE 1
#define HPS_UART_STATUS_TXEMPTY 2
#define HPS_UART_STATUS_RXAVAIL 3
#define HPS_UART_STATUS_RXFULL  4

// Software reset flags
//  - Write Only. All flags self-clear.
#define HPS_UART_SOFTRST_UART_MASK    0x1  // Software reset for whole UART block.
#define HPS_UART_SOFTRST_UART_OFFS    0
#define HPS_UART_SOFTRST_RFR_MASK     0x1  // Receive FIFO reset (shadow of FIFOCTRL[1] bit)
#define HPS_UART_SOFTRST_RFR_OFFS     1
#define HPS_UART_SOFTRST_XFR_MASK     0x1  // Transmit FIFO reset (shadow of FIFOCTRL[2] bit)
#define HPS_UART_SOFTRST_XFR_OFFS     2

// Shadow request to send
#define HPS_UART_RTSCTRL_MASK         0x1  // Write 1 to assert RTSn signal low (shadow of MODEMCTRL[1]).
#define HPS_UART_RTSCTRL_OFFS         0

// Shadow break request
#define HPS_UART_BREAKCTRL_MASK       0x1  // Write 1 to for RX signal low (shadow of LINECTRL[6]).
#define HPS_UART_BREAKCTRL_OFFS       0

// Shadow DMA mode
#define HPS_UART_DMAMODE_MASK         0x1  // Control the DMA signalling mode (shadow of FIFOCTRL[3])
#define HPS_UART_DMAMODE_OFFS         0

// Shadow FIFO enable
#define HPS_UART_FIFOENBL_MASK        0x1  // Write 1 to enable FIFOs (shadow of FIFOCTRL[0]).
#define HPS_UART_FIFOENBL_OFFS        0

// Shadow Receive Trigger Threshold
#define HPS_UART_RXTRIG_MASK          0x3  // Rx threshold trigger (shadow of FIFOCTRL[7:6])
#define HPS_UART_RXTRIG_OFFS          0

// Shadow Tx Empty Trigger Threshold
#define HPS_UART_TXETRIG_MASK         0x3  // Tx empty trigger (shadow of FIFOCTRL[5:4])
#define HPS_UART_TXETRIG_OFFS         0

// Halt transmit FIFO processing
#define HPS_UART_HALTTX_MASK          0x1  // When asserted, transmit FIFO is not processed. Allows filling FIFO before sending.
#define HPS_UART_HALTTX_OFFS          0

// DMA Soft Acknowledge
#define HPS_UART_HALTTX_MASK          0x1  // Deasserts the DMA request signals manually (e.g. if DMA disables channel). Self clear.
#define HPS_UART_HALTTX_OFFS          0

// Component parameters
#define HPS_UART_PARAMS_APBWID_MASK   0x3
#define HPS_UART_PARAMS_APBWID_OFFS   0
#define HPS_UART_PARAMS_AFCE_MASK     0x1
#define HPS_UART_PARAMS_AFCE_OFFS     4
#define HPS_UART_PARAMS_THRE_MASK     0x1
#define HPS_UART_PARAMS_THRE_OFFS     5
#define HPS_UART_PARAMS_SIR_MASK      0x1
#define HPS_UART_PARAMS_SIR_OFFS      6
#define HPS_UART_PARAMS_SIRLP_MASK    0x1
#define HPS_UART_PARAMS_SIRLP_OFFS    7
#define HPS_UART_PARAMS_ADDITNL_MASK  0x1
#define HPS_UART_PARAMS_ADDITNL_OFFS  8
#define HPS_UART_PARAMS_FIFOACC_MASK  0x1
#define HPS_UART_PARAMS_FIFOACC_OFFS  9
#define HPS_UART_PARAMS_FIFOSTAT_MASK 0x1
#define HPS_UART_PARAMS_FIFOSTAT_OFFS 10
#define HPS_UART_PARAMS_SHADOW_MASK   0x1
#define HPS_UART_PARAMS_SHADOW_OFFS   11
#define HPS_UART_PARAMS_ENCPARAM_MASK 0x1
#define HPS_UART_PARAMS_ENCPARAM_OFFS 12
#define HPS_UART_PARAMS_DMAEXTRA_MASK 0x1
#define HPS_UART_PARAMS_DMAEXTRA_OFFS 13
#define HPS_UART_PARAMS_FIFOMODE_MASK 0xFF
#define HPS_UART_PARAMS_FIFOMODE_OFFS 16
#define HPS_UART_FIFOMODE_TO_SIZE(mode) ((mode) * 16)

/*
 * Internal Functions
 */

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
static HpsErr_t _HPS_UART_getInterruptFlags(PHPSUARTCtx_t ctx, HPSUARTIrqSources mask, bool clear) {
    // Read the line status register. Reads to this clear the flags automatically, so we keep a copy
    // of them.
    ctx->irqFlags |= ctx->base[HPS_UART_REG_LINESTAT];
    // Check if tx is empty whenever flags are read
    if (MaskCheck(ctx->irqFlags, HPS_UART_LINESTAT_TEMT_MASK, HPS_UART_LINESTAT_TEMT_OFFS)) {
        ctx->txRunning = false;
    }
    // Check IRQ ID
    HPSUARTInterruptId irqId = MaskExtract(ctx->base[HPS_UART_REG_IRQID], HPS_UART_IRQID_ID_MASK, HPS_UART_IRQID_ID_OFFS);
    if (irqId == HPS_UART_IRQID_MODEMSTAT) {
        // If modem IRQ, update flags and read modemstat to clear the interrupt.
        ctx->irqFlags |= HPS_UART_IRQ_MODEM;
        if (clear) {
            ctx->modemStat = ctx->base[HPS_UART_REG_MODEMSTAT];
        }
    }
    // Read in the flags
    HPSUARTIrqSources flags = MaskExtract(ctx->irqFlags, HPS_UART_LINESTAT_ALL_MASK, HPS_UART_LINESTAT_ALL_OFFS);
    // Keep only flags we care about
    flags = flags & mask;
    // Clear if required
    if (clear) ctx->irqFlags = MaskClear(ctx->irqFlags, flags, HPS_UART_LINESTAT_ALL_OFFS);
    // Return the flags
    return (HpsErr_t)flags;
}

static unsigned int _HPS_UART_writeSpace(PHPSUARTCtx_t ctx) {
    return ctx->fifoSize - ctx->base[HPS_UART_REG_TXFILL];
}

static HpsErr_t _HPS_UART_write(PHPSUARTCtx_t ctx, const uint8_t* data, uint8_t length) {
    int written = 0;
    if (!length) return 0;
    // Ensure the TX empty flag if clear so we can detect end of run
    // after our write.
    _HPS_UART_getInterruptFlags(ctx, HPS_UART_IRQ_TXEMPTY, true);
    // Mark as running if we are writing a non-zero amount
    ctx->txRunning = true;
    // Start the new transfer
    while(length--) {
        if (_HPS_UART_writeSpace(ctx) == 0) {
            //End write if FIFO ran out of space.
            break;
        }
        ctx->base[HPS_UART_REG_DMABURST] = MaskInsert(*data++, HPS_UART_DMABURST_TX_MASK, HPS_UART_DMABURST_TX_OFFS);
        written++;
    }
    return written;
}

static unsigned int _HPS_UART_available(PHPSUARTCtx_t ctx) {
    return ctx->base[HPS_UART_REG_RXFILL];
}

static void _HPS_UART_readWord(PHPSUARTCtx_t ctx, UartRxData_t* data) {
    // Check if we have anything to read
    data->valid = (_HPS_UART_available(ctx) > 0);
    // If we do, decode the data
    if (data->valid) {
        data->partityError = _HPS_UART_getInterruptFlags(ctx, HPS_UART_IRQ_PARITY , true);
        data->frameError   = _HPS_UART_getInterruptFlags(ctx, HPS_UART_IRQ_FRAMING, true);
        data->rxData = MaskExtract(ctx->base[HPS_UART_REG_DMABURST], HPS_UART_DMABURST_RX_MASK, HPS_UART_DMABURST_RX_OFFS);
    }
}

static HpsErr_t _HPS_UART_read(PHPSUARTCtx_t ctx, uint8_t* data, uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Try to read length worth of data
    int numRead = 0;
    bool parityErr = false;
    bool frameErr = false;
    while(length--) {
        UartRxData_t readData;
        _HPS_UART_readWord(ctx, &readData);
        if (!readData.valid) {
            //End read if FIFO ran out of data.
            break;
        }
        // Check for error flags
        parityErr = parityErr || readData.partityError;
        frameErr  = frameErr  || readData.frameError;
        // Read into the buffer
        *data++ = readData.rxData;
        // One more read
        numRead++;
    }
    return (parityErr ? ERR_CHECKSUM : (frameErr ? ERR_CORRUPT : numRead));
}

static void _HPS_UART_cleanup(PHPSUARTCtx_t ctx) {
    //Disable interrupts and reset FIFOs
    if (ctx->base) {
        //Disabling FIFOs clears them.
        ctx->base[HPS_UART_REG_FIFOCTRL] = 0;
    }
}

/*
 * Wrapper APIs used for generic UART interface
 */
static HpsErr_t _HPS_UART_txFifoSpace(PHPSUARTCtx_t ctx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Return FIFO space
    return (HpsErr_t)_HPS_UART_writeSpace(ctx);
}

static HpsErr_t _HPS_UART_rxFifoAvailable(PHPSUARTCtx_t ctx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Return FIFO availability
    return (HpsErr_t)_HPS_UART_available(ctx);
}

static HpsErr_t _HPS_UART_txIdle(PHPSUARTCtx_t ctx, bool clearFlag) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Check the Tx empty IRQ (clearing flag if requested). This will also update txRunning flag
    _HPS_UART_getInterruptFlags(ctx, HPS_UART_IRQ_TXEMPTY, clearFlag);
    // Return whether TX is running
    return !ctx->txRunning;
}

static HpsErr_t _HPS_UART_rxReady(PHPSUARTCtx_t ctx, bool clearFlag) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Ready whenever there is data available
    return _HPS_UART_available(ctx) > 0;
}


/*
 * User Facing APIs
 */

// Initialise HPS UART driver
//  - base is a pointer to the UART peripheral
//  - periphClk is the peripheral clock rate in Hz (e.g. 100E6 = 100MHz for DE1-SoC)
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t HPS_UART_initialise(void* base, unsigned int periphClk, PHPSUARTCtx_t* pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_HPS_UART_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    PHPSUARTCtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->baudClk = periphClk / HPS_UART_L4_SP_CLK_DIVISOR;
    ctx->fifoSize = HPS_UART_FIFOMODE_TO_SIZE(MaskExtract(ctx->base[HPS_UART_REG_PARAMS],HPS_UART_PARAMS_FIFOMODE_MASK,HPS_UART_PARAMS_FIFOMODE_OFFS));
    //Setup UART driver
    ctx->uart.ctx = ctx;
    ctx->uart.is9bit = false; // Never 9-bit, not supported.
    ctx->uart.transmit        = (UartTxFunc_t       )HPS_UART_write;
    ctx->uart.receive         = (UartRxFunc_t       )HPS_UART_read;
    ctx->uart.txIdle          = (UartStatusFunc_t   )_HPS_UART_txIdle;
    ctx->uart.rxReady         = (UartStatusFunc_t   )_HPS_UART_rxReady;
    ctx->uart.txFifoSpace     = (UartFifoSpaceFunc_t)_HPS_UART_txFifoSpace;
    ctx->uart.rxFifoAvailable = (UartFifoSpaceFunc_t)_HPS_UART_rxFifoAvailable;
    ctx->uart.clearFifos      = (UartFifoClearFunc_t)HPS_UART_clearDataFifos;
    //Do a full reset of the UART controller
    ctx->base[HPS_UART_REG_SOFTRST] = MaskCreate(HPS_UART_SOFTRST_XFR_MASK,  HPS_UART_SOFTRST_XFR_OFFS ) |
                                      MaskCreate(HPS_UART_SOFTRST_RFR_MASK,  HPS_UART_SOFTRST_RFR_OFFS ) |
                                      MaskCreate(HPS_UART_SOFTRST_UART_MASK, HPS_UART_SOFTRST_UART_OFFS);
    //Wait a moment for it to come back up
    volatile int dly = 1000;
    while(dly--);
    //Reset IRQ enable
    ctx->base[HPS_UART_REG_LINECTRL] = 0;
    ctx->base[HPS_UART_REG_IERDLH  ] = 0;
    //Initialise the FIFOs
    ctx->base[HPS_UART_REG_FIFOCTRL] = MaskInsert(HPS_UART_TXTHRESH_EMPTY, HPS_UART_FIFOCTRL_TET_MASK, HPS_UART_FIFOCTRL_TET_OFFS) |
                                       MaskInsert(HPS_UART_RXTHRESH_CHAR1, HPS_UART_FIFOCTRL_RT_MASK,  HPS_UART_FIFOCTRL_RT_OFFS ) |
                                       MaskCreate(HPS_UART_FIFOCTRL_FIFOE_MASK, HPS_UART_FIFOCTRL_FIFOE_OFFS); // FIFO enabled.
    ctx->base[HPS_UART_REG_MODEMCTRL] = 0;
    //Clear IRQ flags
    (ctx->base[HPS_UART_REG_LINESTAT ]); // Reading clears flags in LSR.
    (ctx->base[HPS_UART_REG_MODEMSTAT]); // Reading clears flags in MSR.
    ctx->irqFlags = HPS_UART_IRQ_NONE;
    ctx->modemStat = 0;
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver context previously initialised
bool HPS_UART_isInitialised(PHPSUARTCtx_t ctx) {
    return DriverContextCheckInit(ctx);
}


// Enable or Disable UART interrupt(s)
// - Will enable or disable based on the enable input.
// - Only interrupts with mask bit set will be changed.
HpsErr_t HPS_UART_setInterruptEnable(PHPSUARTCtx_t ctx, HPSUARTIrqSources enable, HPSUARTIrqSources mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (enable & HPS_UART_IRQ_RXFIFO) return ERR_NOSUPPORT;
    // Clear any flags for IRQs we are changing
    _HPS_UART_getInterruptFlags(ctx, mask, true);
    // And modify the enable mask
    unsigned int irqEn = ctx->base[HPS_UART_REG_IERDLH];
    if (mask & HPS_UART_IRQ_RXAVAIL) irqEn = MaskModify(irqEn, !!(enable & HPS_UART_IRQ_RXAVAIL), HPS_UART_IERDLH_ERBFI_MASK, HPS_UART_IERDLH_ERBFI_OFFS);
    if (mask & HPS_UART_IRQ_TXEMPTY) irqEn = MaskModify(irqEn, !!(enable & HPS_UART_IRQ_TXEMPTY), HPS_UART_IERDLH_ETBEI_MASK, HPS_UART_IERDLH_ETBEI_OFFS);
    if (mask & HPS_UART_IRQ_ERRORS ) irqEn = MaskModify(irqEn, !!(enable & HPS_UART_IRQ_ERRORS ), HPS_UART_IERDLH_ELSI_MASK,  HPS_UART_IERDLH_ELSI_OFFS );
    if (mask & HPS_UART_IRQ_MODEM  ) irqEn = MaskModify(irqEn, !!(enable & HPS_UART_IRQ_MODEM  ), HPS_UART_IERDLH_EDSSI_MASK, HPS_UART_IERDLH_EDSSI_OFFS);
    ctx->base[HPS_UART_REG_IERDLH] = irqEn;
    return ERR_SUCCESS;
}

// Check interrupt flags
// - Returns whether each of the the selected interrupt flags is asserted
// - If clear is true, will also clear the flags.
HpsErr_t HPS_UART_getInterruptFlags(PHPSUARTCtx_t ctx, HPSUARTIrqSources mask, bool clear) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Access the flags
    return _HPS_UART_getInterruptFlags(ctx, mask, clear);
}

// Set the UART Baud Rate
// - Returns the achieved baud rate
HpsErr_t HPS_UART_setBaudRate(PHPSUARTCtx_t ctx, unsigned int baudRate) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // The following formula is used to calculate the Baud divisors:
    //
    //                      [Prescaler Timer Frequency]
    //    [Baud Divisor] = -----------------------------
    //                            16 x [Baud Rate]
    unsigned int divisor;
    if (baudRate == UART_BAUD_MIN) {
        divisor = UINT16_MAX;
    } else if (baudRate == UART_BAUD_MAX) {
        divisor = 1;
    } else {
        divisor = (ctx->baudClk / baudRate);
        if (divisor > UINT16_MAX) {
            divisor = UINT16_MAX;
        }
    }
    // Enable changing divisor
    ctx->base[HPS_UART_REG_LINECTRL ] = MaskSet(ctx->base[HPS_UART_REG_LINECTRL],HPS_UART_LINECTRL_DLAB_MASK,HPS_UART_LINECTRL_DLAB_OFFS);
    // Set lower and upper divisor bytes
    ctx->base[HPS_UART_REG_RBRTHRDLL] = MaskInsert(divisor,      HPS_UART_RBRTHRDLL_DLL_MASK, HPS_UART_RBRTHRDLL_DLL_OFFS);
    ctx->base[HPS_UART_REG_IERDLH   ] = MaskInsert(divisor >> 8, HPS_UART_IERDLH_DLH_MASK,    HPS_UART_IERDLH_DLH_OFFS   );
    // Disable changing divisor (restore access to TxRx data)
    ctx->base[HPS_UART_REG_LINECTRL ] = MaskClear(ctx->base[HPS_UART_REG_LINECTRL],HPS_UART_LINECTRL_DLAB_MASK,HPS_UART_LINECTRL_DLAB_OFFS);
    // Convert back to baud rate to return to user
    baudRate = (ctx->baudClk / divisor);
    if (baudRate > INT32_MAX) baudRate = INT32_MAX;
    return (HpsErr_t) baudRate;
}

// Check UART Operation mode
//  - UART core is always in full-duplex mode
HpsErr_t HPS_UART_getTransferMode(PHPSUARTCtx_t ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Always full duplex
    return (HpsErr_t)UART_FULL_DUPLEX;
}

// Set the UART Data Format
//  - Sets the data format to be used.
//  - The format can select the data width, and the parity mode.
//  - There will always be 1 stop bit, but due to the design of the firmware design there will always be
//    a single extra bit of dead time between TX, which makes the core compatible with both 1 and 2 stop bits
HpsErr_t HPS_UART_setDataFormat(PHPSUARTCtx_t ctx, unsigned int dataWidth, UartParity parityEn, HPSUARTStopBits stopBits, HPSUARTFlowCntrl flowctrl) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Map common data width enum to HPS data size enum
    HPSUARTDataSize dataSize;
    switch (dataWidth) {
        case UART_5BIT: dataSize = HPS_UART_DATASIZE_5BIT; break;
        case UART_6BIT: dataSize = HPS_UART_DATASIZE_6BIT; break;
        case UART_7BIT: dataSize = HPS_UART_DATASIZE_7BIT; break;
        case UART_8BIT: dataSize = HPS_UART_DATASIZE_8BIT; break;
        case UART_9BIT: return ERR_TOOBIG;
        default: return ERR_TOOSMALL;
    }
    //Update the data parameters
    unsigned int lcrVal = ctx->base[HPS_UART_REG_LINECTRL];
    lcrVal = MaskModify(lcrVal, dataSize,                    HPS_UART_LINECTRL_DLS_MASK,  HPS_UART_LINECTRL_DLS_OFFS );
    lcrVal = MaskModify(lcrVal, stopBits,                    HPS_UART_LINECTRL_STOP_MASK, HPS_UART_LINECTRL_STOP_OFFS);
    lcrVal = MaskModify(lcrVal, parityEn != UART_NOPARITY,   HPS_UART_LINECTRL_PEN_MASK,  HPS_UART_LINECTRL_PEN_OFFS );
    lcrVal = MaskModify(lcrVal, parityEn == UART_EVENPARITY, HPS_UART_LINECTRL_EPS_MASK,  HPS_UART_LINECTRL_EPS_OFFS );
    ctx->base[HPS_UART_REG_LINECTRL] = lcrVal;
    //Configure flow control
    unsigned int mcrVal = ctx->base[HPS_UART_REG_MODEMCTRL];
    mcrVal = MaskModify(mcrVal, flowctrl, HPS_UART_MODEMCTRL_AFCE_MASK, HPS_UART_MODEMCTRL_AFCE_OFFS);
    mcrVal = MaskModify(mcrVal, flowctrl, HPS_UART_MODEMCTRL_RTS_MASK,  HPS_UART_MODEMCTRL_RTS_OFFS );
    ctx->base[HPS_UART_REG_MODEMCTRL] = mcrVal;
    return ERR_SUCCESS;
}

// Clear UART FIFOs
//  - Clears either the TX, RX or both FIFOs.
//  - Data will be deleted. Any pending TX will not be sent.
HpsErr_t HPS_UART_clearDataFifos(PHPSUARTCtx_t ctx, bool clearTx, bool clearRx) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Assert the FIFO reset flags (self-clear)
    ctx->base[HPS_UART_REG_SOFTRST] = MaskInsert(clearTx, HPS_UART_SOFTRST_XFR_MASK, HPS_UART_SOFTRST_XFR_OFFS) |
                                      MaskInsert(clearRx, HPS_UART_SOFTRST_RFR_MASK, HPS_UART_SOFTRST_RFR_OFFS);
    return ERR_SUCCESS;
}

// Check if there is space for TX data
//  - Returns ERR_NOSPACE if there is no space.
//  - If non-null, returns the amount of empty space in the TX FIFO in *space
HpsErr_t HPS_UART_writeSpace(PHPSUARTCtx_t ctx, unsigned int* space) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Check the space
    unsigned int _space = _HPS_UART_writeSpace(ctx);
    if (space) *space = _space;
    return _space ? ERR_SUCCESS : ERR_NOSPACE;
}

// Perform a UART write
//  - A write transfer will be performed
//  - Length is size of data in words.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If there is not enough space in the FIFO, as many bytes as possible are sent.
//  - The return value indicates number sent.
HpsErr_t HPS_UART_write(PHPSUARTCtx_t ctx, const uint8_t data[], uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // And write
    return _HPS_UART_write(ctx, data, length);
}

// Check if there is available Rx data
//  - Returns ERR_ISEMPTY if there is nothing available.
//  - If non-null, returns the amount available in the RX FIFO in *available
HpsErr_t HPS_UART_available(PHPSUARTCtx_t ctx, unsigned int* available) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Read the FIFO available level
    unsigned int _available = _HPS_UART_available(ctx);
    if (available) *available = _available;
    return _available ? ERR_SUCCESS : ERR_ISEMPTY;
}

// Read from the UART FIFO
//  - Reads a single word from the UART FIFO
//  - Error information is included in the return struct
UartRxData_t HPS_UART_readWord(PHPSUARTCtx_t ctx) {
    UartRxData_t data = {0};
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_SUCCESS(status)) {
        // And read
        _HPS_UART_readWord(ctx, &data);
    }
    return data;
}

// Read multiple from UART FIFO
//  - This will attempt to read multiple bytes from the UART FIFO.
//  - Length is size of data in words.
//  - The data[] parameter must be an array of at least 'length' words.
//  - If successful, will return number of bytes read
//  - If the return value is negative, an error occurred in one of the words
HpsErr_t HPS_UART_read(PHPSUARTCtx_t ctx, uint8_t data[], uint8_t length) {
    // Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // And read
    return _HPS_UART_read(ctx, data, length);
}
