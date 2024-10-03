/*
 * HPS SPI Driver
 * --------------
 *
 * Driver for the HPS embedded SPI controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-----------------------------------
 * 14/01/2024 | Creation of driver
 *
 */

#include "HPS_SPI.h"

#include "HPS_Watchdog/HPS_Watchdog.h"
#include "Util/bit_helpers.h"
#include "Util/macros.h"

#include <math.h>

/*
 * Registers
 */

// Registers in SPI controller
#define HPS_SPI_REG_CONTROL  (0x00/sizeof(unsigned int))
#define HPS_SPI_REG_FRAMES   (0x04/sizeof(unsigned int))
#define HPS_SPI_REG_ENABLE   (0x08/sizeof(unsigned int))
#define HPS_SPI_REG_MWCTRL   (0x0C/sizeof(unsigned int))
#define HPS_SPI_REG_SLVSEL   (0x10/sizeof(unsigned int))
#define HPS_SPI_REG_BAUDRT   (0x14/sizeof(unsigned int))
#define HPS_SPI_REG_TXFILL   (0x20/sizeof(unsigned int))
#define HPS_SPI_REG_RXFILL   (0x24/sizeof(unsigned int))
#define HPS_SPI_REG_STATUS   (0x28/sizeof(unsigned int))
#define HPS_SPI_REG_CLRTXOVF (0x38/sizeof(unsigned int))
#define HPS_SPI_REG_CLRRXOVF (0x3C/sizeof(unsigned int))
#define HPS_SPI_REG_CLRRXUNF (0x40/sizeof(unsigned int))
#define HPS_SPI_REG_CLRIRQS  (0x48/sizeof(unsigned int))
#define HPS_SPI_REG_DATAREG  (0x60/sizeof(unsigned int))
#define HPS_SPI_REG_RXDLY    (0xF0/sizeof(unsigned int))

// Control flags
#define HPS_SPI_CONTROL_WIDTH    0
#define HPS_SPI_CONTROL_FORMAT   4
#define HPS_SPI_CONTROL_CPHA     6
#define HPS_SPI_CONTROL_CPOL     7
#define HPS_SPI_CONTROL_XFERMODE 8
#define HPS_SPI_CONTROL_MWFRAME  12

#define HPS_SPI_CONTROL_WIDTH_MASK    0xFU
#define HPS_SPI_CONTROL_FORMAT_MASK   0x3U
#define HPS_SPI_CONTROL_XFERMODE_MASK 0x3U
#define HPS_SPI_CONTROL_MWFRAME_MASK  0xFU

// Enable flags
#define HPS_SPI_ENABLE_SPIEN 0

// Slave selects
#define HPS_SPI_SS_MASK 0xFU

// Baud rates
#define HPS_SPI_BAUDDIV_MAX 0xFFFF

// Status flags
#define HPS_SPI_STATUS_BUSY    0
#define HPS_SPI_STATUS_TXSPACE 1
#define HPS_SPI_STATUS_TXEMPTY 2
#define HPS_SPI_STATUS_RXAVAIL 3
#define HPS_SPI_STATUS_RXFULL  4
#define HPS_SPI_STATUS_COLLISN 6

// MW Control flags
#define HPS_SPI_MWCTRL_SEQUENTIAL 0
#define HPS_SPI_MWCTRL_DIRECTION  1
#define HPS_SPI_MWCTRL_HANDSHAKE  2


/*
 * Internal Functions
 */

//Check if busy, and if not disable the SPI
// - if rxFill is true, will return RX fill level on success. Otherwise returns ERR_SUCCESS.
static HpsErr_t _HPS_SPI_checkBusy(PHPSSPICtx_t ctx) {
    //Can only update if SPI is disabled (i.e. not busy)
    if (ctx->base[HPS_SPI_REG_ENABLE] & _BV(HPS_SPI_ENABLE_SPIEN)) {
        //Busy if a transfer is still running
        if (ctx->base[HPS_SPI_REG_STATUS] & _BV(HPS_SPI_STATUS_BUSY)) {
            return ERR_BUSY;
        }
        //Busy also if there is anything in the receive FIFO
        if (ctx->base[HPS_SPI_REG_RXFILL]) {
            return ERR_BUSY;
        }
        //Disable the SPI as there are no outstanding transfers
        ctx->base[HPS_SPI_REG_ENABLE] = 0x0;
    }
    return ERR_SUCCESS;
}

//Set number of transfers and shift register width based on data width
static inline HpsErr_t _HPS_SPI_checkWidth(unsigned int val, unsigned int minVal, unsigned int maxVal) {
    if (val < minVal) return ERR_TOOSMALL;
    if (val > maxVal) return ERR_TOOBIG;
    return ERR_SUCCESS;
}

//Calculate the necessary clock divider
// - returns false if can't find suitable rate
static inline bool _HPS_SPI_calcDivider(PHPSSPICtx_t ctx, unsigned int clockFreq) {
    //Divider is periphery clock over clock frequency, rounded up (to ensure we don't exceed clkFreq)
    unsigned int divider = CEIL_DIV(ctx->periphClk, clockFreq);
    //We also must be a multiple of 2, so round up if needed.
    if (divider % 2) divider++;
    //Ensure we don't exceed limits
    if (!divider) return false;
    if (divider > HPS_SPI_BAUDDIV_MAX) return false;
    //Save divider value
    ctx->config.clkDiv = divider;
    return true;
}

//Set number of transfers and shift register width based on data width
static HpsErr_t _HPS_SPI_setDataWidth(PHPSSPICtx_t ctx, unsigned int dataWidth) {
    //Width must be in range
    HpsErr_t status = _HPS_SPI_checkWidth(dataWidth, HPS_SPI_WIDTH_MIN, HPS_SPI_WIDTH_TOTAL_MAX);
    if (ERR_IS_ERROR(status)) return status;
    //If bigger than shift length, use multiple transfers.
    unsigned int xfers;
    unsigned int shiftWidth;
    //Check if shift size is larger then SPI capabilities
    if (dataWidth > HPS_SPI_WIDTH_SHIFT_MAX) {
        // Calculate minimum number of transfers to make small enough
        unsigned int minXfers = CEIL_DIV(dataWidth, HPS_SPI_WIDTH_SHIFT_MAX);
        // Find next smallest factor >= minimum transfers
        shiftWidth = 1;
        for (xfers = minXfers; xfers <= dataWidth; xfers++) {
            // Ensure width is large enough for our SPI controller
            shiftWidth = dataWidth / xfers;
            if (shiftWidth < HPS_SPI_WIDTH_MIN) return ERR_NOSUPPORT;
            // If xfers is a factor of data width we have found our best option.
            // We will always find a match as shiftWidth will go down to 1 (if supported)
            if (!(dataWidth % xfers)) {
                break;
            }
        }
    } else {
        xfers = 1;
        shiftWidth = dataWidth;
    }
    //Save results
    ctx->config.totalWidth = dataWidth;
    ctx->config.width = shiftWidth;
    ctx->config.xfers = xfers;
    return ERR_SUCCESS;
}

//Set Microwire control width
static HpsErr_t _HPS_SPI_setMwCtrlWidth(PHPSSPICtx_t ctx, unsigned int mwCtrlWidth) {
    // If not in MW mode, value is don't care. Set to min.
    if (ctx->config.format != HPS_SPI_FORMAT_MICROWIRE) {
        ctx->config.mw.ctrlWidth = HPS_SPI_MW_CNTRL_MIN;
        return ERR_SUCCESS;
    }
    //Check for invalid control width
    HpsErr_t status = _HPS_SPI_checkWidth(mwCtrlWidth, HPS_SPI_MW_CNTRL_MIN, HPS_SPI_MW_CNTRL_MAX);
    //Save if valid
    if (ERR_IS_SUCCESS(status)) {
        ctx->config.mw.ctrlWidth = mwCtrlWidth;
    }
    return status;
}

//Set config register. Requires SPI be disabled
static void _HPS_SPI_configureFormat(PHPSSPICtx_t ctx) {
    //Prepare config value
    unsigned int config = 0;
    //Set transfer parameters. Default to TxRx mode
    config |= ((ctx->config.width-1) & HPS_SPI_CONTROL_WIDTH_MASK) << HPS_SPI_CONTROL_WIDTH;
    config |= (ctx->config.format & HPS_SPI_CONTROL_FORMAT_MASK) << HPS_SPI_CONTROL_FORMAT;
    config |= (ctx->config.cpol == SPI_CPOL_HIGH ) ? _BV(HPS_SPI_CONTROL_CPOL) : 0;
    config |= (ctx->config.cpha == SPI_CPHA_START) ? _BV(HPS_SPI_CONTROL_CPHA) : 0;
    config |= (ctx->config.xferMode & HPS_SPI_CONTROL_XFERMODE_MASK) << HPS_SPI_CONTROL_XFERMODE;
    //Only use mwCtrlWidth if in microwire mode
    if (ctx->config.format == HPS_SPI_FORMAT_MICROWIRE) {
        config |= ((ctx->config.mw.ctrlWidth-1) & HPS_SPI_CONTROL_WIDTH_MASK) << HPS_SPI_CONTROL_WIDTH;
        //Also set MW control register
        ctx->base[HPS_SPI_REG_MWCTRL] = (ctx->config.mw.seqTransfer  << HPS_SPI_MWCTRL_SEQUENTIAL) |
                                        (ctx->config.mw.txMode       << HPS_SPI_MWCTRL_DIRECTION ) |
                                        (ctx->config.mw.useHandshake << HPS_SPI_MWCTRL_HANDSHAKE );
    }
    //Set baud rate
    ctx->base[HPS_SPI_REG_BAUDRT] = ctx->config.clkDiv;
    //Update control register
    ctx->base[HPS_SPI_REG_CONTROL] = config;
    // Set the selected slaves
    ctx->base[HPS_SPI_REG_SLVSEL] = ctx->config.selectedSlaves;
}

static void _HPS_SPI_cleanup(PHPSSPICtx_t ctx) {
    //Disable the SPI controller.
    if (ctx->base) {
        ctx->base[HPS_SPI_REG_ENABLE] = 0x0;
    }
}

/*
 * User Facing APIs
 */

//Initialise HPS SPI Controller
// - periphClk is the peripheral clock rate in Hz (e.g. 100E6 = 100MHz for DE1-SoC)
// - format selects the protocol used; use MOTOROLA if unsure
// - chpa/cpol set clock polarity and phase
// - dataWidth sets number of bits in data transfers
// - mwCtrlWidth is only used if format is MICROWIRE
// - Returns 0 if successful.
HpsErr_t HPS_SPI_initialise(void* base, unsigned int periphClk, HPSSPIFormat format, PHPSSPICtx_t* pCtx) {
    HpsErr_t status;
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    status = DriverContextAllocateWithCleanup(pCtx, &_HPS_SPI_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    PHPSSPICtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->periphClk = periphClk;
    //Initialise SPI common context
    ctx->spi.ctx = ctx;
    ctx->spi.laneCount    = 1;
    ctx->spi.writeReady   = (SpiStatusFunc_t   )&HPS_SPI_writeReady;
    ctx->spi.write        = (SpiWriteFunc_t    )&HPS_SPI_writeData;
    ctx->spi.readReady    = (SpiStatusFunc_t   )&HPS_SPI_readReady;
    ctx->spi.read         = (SpiReadFunc_t     )&HPS_SPI_readData;
    ctx->spi.abort        = (SpiAbortFunc_t    )&HPS_SPI_abort;
    ctx->spi.setDirection = (SpiDirectionFunc_t)&HPS_SPI_setMicrowireDirection;
    ctx->spi.slaveSelect  = (SpiSlaveSelFunc_t )&HPS_SPI_slaveSelect;
    ctx->spi.setClockMode = (SpiClockModeFunc_t)&HPS_SPI_setClockMode;
    ctx->spi.setDataWidth = (SpiDataWidthFunc_t)&HPS_SPI_setDataWidth;
    //Ensure SPI disabled to begin with. We enable during transfer only
    ctx->base[HPS_SPI_REG_ENABLE] = 0;
    ctx->base[HPS_SPI_REG_RXDLY ] = 2;
    //And set default configuration
    ctx->config.format = format;
    ctx->config.cpha = SPI_CPHA_START;
    ctx->config.cpol = SPI_CPOL_LOW;
    ctx->config.xferMode = HPS_SPI_XFERMODE_TXRX;
    ctx->config.clkDiv = 0; //Clock disabled by default
    ctx->config.mw.ctrlWidth = 0;
    //Validate data and MW control widths
    status = _HPS_SPI_setDataWidth(ctx, 8);
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status =_HPS_SPI_setMwCtrlWidth(ctx, 0);
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    //And initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

//Check if driver initialised
// - Returns true if driver context is initialised
bool HPS_SPI_isInitialised(PHPSSPICtx_t ctx){
    return DriverContextCheckInit(ctx);
}

//Set data and clock format
// - chpa/cpol set clock polarity and phase
// - clkFreq is the max target rate for the SPI clock.
//   - Will find the closest value that is slower than clkRate to use.
//   - If can't achieve a slow enough value, will error.
// - dataWidth sets number of bits in data transfers.
//    - Widths > SHIFT_MAX will be performed as multiple transfers
//    - Only multiples of 1, 2, 3, or 5 transfers is supported. This works for all widths up to 16, and all non-prime data widths up to 32.
// - mwCtrlWidth is only used if format is MICROWIRE
HpsErr_t HPS_SPI_configureFormat(PHPSSPICtx_t ctx, SpiSCLKPolarity cpol, SpiSCLKPhase cpha, unsigned int clkFreq, unsigned int dataWidth, unsigned int mwCtrlWidth) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Can't be busy
    if (ERR_IS_BUSY(_HPS_SPI_checkBusy(ctx))) return ERR_BUSY;
    //Calculate clock divider
    if (!_HPS_SPI_calcDivider(ctx, clkFreq)) return ERR_NOSUPPORT;
    //Set widths
    status = _HPS_SPI_setDataWidth(ctx, dataWidth);
    if (ERR_IS_ERROR(status)) return status;
    status = _HPS_SPI_setMwCtrlWidth(ctx, mwCtrlWidth);
    if (ERR_IS_ERROR(status)) return status;
    //Save new parameters
    ctx->config.cpha = cpha;
    ctx->config.cpol = cpol;
    ctx->config.mw.ctrlWidth = mwCtrlWidth;
    return ERR_SUCCESS;
}

// Set the SPI clock parameters
// - Change the phase/polarity width without changing other formats
HpsErr_t HPS_SPI_setClockMode(PHPSSPICtx_t ctx, SpiSCLKPolarity cpol, SpiSCLKPhase cpha) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Save new parameters.
    ctx->config.cpol = cpol;
    ctx->config.cpha = cpha;
    return ERR_SUCCESS;
}

//Change the SPI data width
// - Change the data width without changing other formats
HpsErr_t HPS_SPI_setDataWidth(PHPSSPICtx_t ctx, unsigned int dataWidth) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    return _HPS_SPI_setDataWidth(ctx, dataWidth);
}

//Check if ready to write
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - returns ERR_SUCCESS if ready to start a new write (i.e. not busy)
HpsErr_t HPS_SPI_writeReady(PHPSSPICtx_t ctx, uint32_t laneMask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Return whether busy
    return _HPS_SPI_checkBusy(ctx);
}

//Perform a single word transfer
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - If write only, will not store read result to FIFO.
HpsErr_t HPS_SPI_writeData(PHPSSPICtx_t ctx, uint32_t laneMask, uint32_t* data, SpiTransferType type) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!laneMask) return ERR_SUCCESS;
    if (!data) return ERR_NULLPTR;
    //Can't be busy
    if (ERR_IS_BUSY(_HPS_SPI_checkBusy(ctx))) return ERR_BUSY;
    //Update config register for this transfer
    ctx->config.xferMode = (type == SPI_TYPE_READWRITE) ? HPS_SPI_XFERMODE_TXRX : HPS_SPI_XFERMODE_TXONLY;
    _HPS_SPI_configureFormat(ctx);
    //Enable SPI controller
    ctx->base[HPS_SPI_REG_ENABLE] = _BV(HPS_SPI_ENABLE_SPIEN);
    //Split the data word into the number of width-bit transfers
    //being performed.
    uint64_t dataOut = ((uint64_t)*data++);
    if (ctx->config.totalWidth > 32) {
        dataOut |= ((uint64_t)*data) << 32ULL;
    }
    unsigned int xferMask = UINTN_MAX(ctx->config.width);
    unsigned int xfers = ctx->config.xfers;
    unsigned int xferData[xfers];
    for (unsigned int xferIdx = 0; xferIdx < xfers; xferIdx++) {
        xferData[xferIdx] = dataOut & xferMask;
        dataOut >>= ctx->config.width;
    }
    //Now load the required number of transfers into the write FIFO.
    //We send the split data in reverse order as our transmission is
    //MSB first.
    while (xfers--) {
        ctx->base[HPS_SPI_REG_DATAREG] = xferData[xfers];
    }
    return ERR_SUCCESS;
}

//Check if there is any data in the read FIFO
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - Returns the number of available words on success.
HpsErr_t HPS_SPI_readReady(PHPSSPICtx_t ctx, uint32_t laneMask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Return the number of full words available, accounting for transfers per word
    return ctx->base[HPS_SPI_REG_RXFILL] / ctx->config.xfers;
}

//Read a word from the read FIFO
// - Only one lane. Ignores bits higher than 0 in lane mask.
HpsErr_t HPS_SPI_readData(PHPSSPICtx_t ctx, uint32_t laneMask, uint32_t* data) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!laneMask) return ERR_SUCCESS;
    if (!data) return ERR_NULLPTR;
    //Ensure there are enough words in the incoming FIFO
    unsigned int xfers = ctx->config.xfers;
    if (ctx->base[HPS_SPI_REG_RXFILL] < xfers) return ERR_AGAIN;
    //Recombine the correct of transfers of data
    unsigned int xferMask = UINTN_MAX(ctx->config.width);
    uint64_t dataIn = 0;
    while (xfers--) {
        dataIn <<= ctx->config.width;
        dataIn |= (ctx->base[HPS_SPI_REG_DATAREG] & xferMask);
    }
    *data++ = (uint32_t)dataIn;
    if (ctx->config.totalWidth > 32) {
        *data = (uint32_t)(dataIn >> 32ULL);
    }
    return ERR_SUCCESS;
}

//Configure the slave select pins
// - Only auto slave select mode supported
// - Maximum of 4 slave selects, only bits 0-3 are used.
HpsErr_t HPS_SPI_slaveSelect(PHPSSPICtx_t ctx, bool autoSlaveSel, uint32_t selectedSlaves) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Error if manual slave select requested
    if (!autoSlaveSel) return ERR_NOSUPPORT;
    //Can't be busy
    if (ERR_IS_BUSY(_HPS_SPI_checkBusy(ctx))) return ERR_BUSY;
    //Save the selected slaves config
    ctx->config.selectedSlaves = selectedSlaves & HPS_SPI_SS_MASK;
    return ERR_SUCCESS;
}

//Set the microwire transfer direction.
// - This is only used in microwire mode to select the direction of a microwire transfer
HpsErr_t HPS_SPI_setMicrowireDirection(PHPSSPICtx_t ctx, SpiMISODirection dir) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (ctx->config.format != HPS_SPI_FORMAT_MICROWIRE) return ERR_WRONGMODE;
    ctx->config.mw.txMode = (dir == SPI_MISO_OUT);
    return ERR_SUCCESS;
}

//Set the microwire control mode
// - Configure whether the next transfers will be sequential or non-sequential mode
// - Configure whether to use handshaking interface
HpsErr_t HPS_SPI_setMicrowireControl(PHPSSPICtx_t ctx, bool seqTransfer, bool useHandshake) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Save parameters
    if (ctx->config.format != HPS_SPI_FORMAT_MICROWIRE) return ERR_WRONGMODE;
    ctx->config.mw.seqTransfer = seqTransfer;
    ctx->config.mw.useHandshake = useHandshake;
    return ERR_SUCCESS;
}

//Abort the current transfer
// - Immediately disables the SPI controller. This may cause partial transfers on the bus.
HpsErr_t HPS_SPI_abort(PHPSSPICtx_t ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Just disable without thought
    ctx->base[HPS_SPI_REG_ENABLE] = 0;
    return ERR_SUCCESS;
}

