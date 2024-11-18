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

#ifndef HPS_SPI_H_
#define HPS_SPI_H_

#include <stdint.h>
#include <stdbool.h>

#include "Util/driver_spi.h"

// SPI Formats
typedef enum {
    HPS_SPI_FORMAT_MOTOROLA  = 0x0,
    HPS_SPI_FORMAT_TEXAS_SSP = 0x1,
    HPS_SPI_FORMAT_MICROWIRE = 0x2
} HPSSPIFormat;

// SPI Data Widths
enum {
    HPS_SPI_WIDTH_MIN       = 4,  // Minimum data width
    HPS_SPI_WIDTH_SHIFT_MAX = 16, // Max width of the hardware SPI. Any widths from min up to this value are supported
    HPS_SPI_WIDTH_TOTAL_MAX = 64  // Max width of a transfer. Widths > SHIFT_MAX will be performed as multiple transfers
};

// SPI Micro-wire control widths
enum {
    HPS_SPI_MW_CNTRL_MIN = 1,
    HPS_SPI_MW_CNTRL_MAX = 16
};

// Transfer Mode (internal)
typedef enum {
    HPS_SPI_XFERMODE_TXRX   = 0x0,
    HPS_SPI_XFERMODE_TXONLY = 0x1,
    HPS_SPI_XFERMODE_RXONLY = 0x2
} HPSSPIXferMode;

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    unsigned int periphClk;
    // SPI common interface
    SpiCtx_t spi;
    // Transfer Config
    struct {
        // General
        unsigned int    totalWidth;
        unsigned int    width;
        unsigned int    xfers;
        HPSSPIFormat    format;
        SpiSCLKPolarity cpol;
        SpiSCLKPhase    cpha;
        unsigned int    selectedSlaves;
        HPSSPIXferMode  xferMode;
        unsigned int    clkDiv;
        // Microwire
        struct {
            unsigned int ctrlWidth;
            bool         seqTransfer;
            bool         useHandshake;
            bool         txMode;
        } mw;
    } config;
} HPSSPICtx_t;

//Initialise HPS SPI Controller
// - periphClk is the peripheral clock rate in Hz (e.g. 100E6 = 100MHz for DE1-SoC)
// - format selects the protocol used; use MOTOROLA if unsure
// - Returns 0 if successful.
HpsErr_t HPS_SPI_initialise(void* base, unsigned int periphClk, HPSSPIFormat format, HPSSPICtx_t** pCtx);

//Check if driver initialised
// - Returns true if driver context is initialised
bool HPS_SPI_isInitialised(HPSSPICtx_t* ctx);

//Set data and clock format
// - chpa/cpol set clock polarity and phase
// - clkFreq is the max target rate for the SPI clock.
//   - Will find the closest value that is slower than clkRate to use.
//   - If can't achieve a slow enough value, will error.
// - dataWidth sets number of bits in data transfers.
//    - Widths > SHIFT_MAX will be performed as multiple transfers
//    - Only multiples of 1, 2, 3, or 5 transfers is supported. This works for all widths up to 16, and all non-prime data widths up to 32.
// - mwCtrlWidth is only used if format is MICROWIRE
HpsErr_t HPS_SPI_configureFormat(HPSSPICtx_t* ctx, SpiSCLKPolarity cpol, SpiSCLKPhase cpha, unsigned int clkFreq, unsigned int dataWidth, unsigned int mwCtrlWidth);

// Set the SPI clock parameters
// - Change the phase/polarity width without changing other formats
HpsErr_t HPS_SPI_setClockMode(HPSSPICtx_t* ctx, SpiSCLKPolarity cpol, SpiSCLKPhase cpha);

//Change the SPI data width
// - Change the data width without changing other formats
HpsErr_t HPS_SPI_setDataWidth(HPSSPICtx_t* ctx, unsigned int dataWidth);

//Check if ready to write
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - returns ERR_SUCCESS if ready to start a new write (i.e. not busy)
HpsErr_t HPS_SPI_writeReady(HPSSPICtx_t* ctx, uint32_t laneMask);

//Perform a single word transfer
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - For data widths between 33 and 64, provide two 32-bit per lane.
// - If write only, will not store read result to FIFO.
HpsErr_t HPS_SPI_writeData(HPSSPICtx_t* ctx, uint32_t laneMask, uint32_t* data, SpiTransferType type);

//Check if there is any data in the read FIFO
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - Returns the number of available words on success.
HpsErr_t HPS_SPI_readReady(HPSSPICtx_t* ctx, uint32_t laneMask);

//Read a word from the read FIFO
// - Only one lane. Ignores bits higher than 0 in lane mask.
// - For data widths between 33 and 64, provide two 32-bit per lane.
HpsErr_t HPS_SPI_readData(HPSSPICtx_t* ctx, uint32_t laneMask, uint32_t* data);

//Configure the slave select pins
// - Only auto slave select mode supported
// - Maximum of 4 slave selects, only bits 0-3 are used.
HpsErr_t HPS_SPI_slaveSelect(HPSSPICtx_t* ctx, bool autoSlaveSel, uint32_t selectedSlaves);

//Set the microwire transfer direction.
// - This is only used in microwire mode to select the direction of a microwire transfer
HpsErr_t HPS_SPI_setMicrowireDirection(HPSSPICtx_t* ctx, SpiMISODirection dir);

//Set the microwire control mode
// - Configure whether the next transfers will be sequential or non-sequential mode
// - Configure whether to use handshaking interface
HpsErr_t HPS_SPI_setMicrowireControl(HPSSPICtx_t* ctx, bool seqTransfer, bool useHandshake);

//Abort the current transfer
// - Immediately disables the SPI controller. This may cause partial transfers on the bus.
HpsErr_t HPS_SPI_abort(HPSSPICtx_t* ctx);

#endif /* HPS_SPI_H_ */
