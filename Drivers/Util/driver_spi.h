/* Generic SPI Driver Interface
 * -----------------------------
 *
 * Provides a common interface for different SPI
 * drivers to allow them to be used as a generic
 * handler.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 10/01/2024 | Creation of driver.
 */

#ifndef DRIVER_SPI_H_
#define DRIVER_SPI_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>
#include <Util/bit_helpers.h>

#include "Util/error.h"

// MISO direction for bidirectional masters
typedef enum {
    SPI_MISO_OUT = 0,
    SPI_MISO_IN  = 1,
} SpiMISODirection;

// Clock Phase/Polarity
typedef enum {
    SPI_CPOL_LOW  = 0,   // Clock idles low
    SPI_CPOL_HIGH = 1    // Clock idles high
} SpiSCLKPolarity;

typedef enum {
    SPI_CPHA_MID   = 0,  // First clock edge in middle of bit
    SPI_CPHA_START = 1   // First clock edge at start of bit
} SpiSCLKPhase;

// Transfer type
typedef enum {
    SPI_TYPE_WRITEONLY = 0, // Write a value, ignore read result
    SPI_TYPE_READWRITE = 1  // Write a value, store read value in FIFO.
} SpiTransferType;

// IO Function Templates
typedef HpsErr_t    (*SpiWriteFunc_t)    (void* ctx, uint32_t laneMask, uint32_t* data, SpiTransferType type);
typedef HpsErr_t    (*SpiReadFunc_t)     (void* ctx, uint32_t laneMask, uint32_t* data);
typedef HpsErr_t    (*SpiSlaveSelFunc_t) (void* ctx, bool autoSlaveSel, uint32_t slaveMask);
typedef HpsErrExt_t (*SpiStatusFunc_t)   (void* ctx, uint32_t laneMask);
typedef HpsErr_t    (*SpiDirectionFunc_t)(void* ctx, SpiMISODirection dir);
typedef HpsErr_t    (*SpiDataWidthFunc_t)(void* ctx, unsigned int dataWidth);
typedef HpsErr_t    (*SpiAbortFunc_t)    (void* ctx);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Number of available lanes. Max 31
    unsigned int laneCount;
    // Transfer Function Pointers
    SpiWriteFunc_t    write;
    SpiReadFunc_t     read;
    SpiSlaveSelFunc_t slaveSelect;
    SpiAbortFunc_t    abort;
    // Status Functions
    SpiStatusFunc_t    writeReady;
    SpiStatusFunc_t    readReady;
    // Config Functions
    SpiDirectionFunc_t setDirection;
    SpiDataWidthFunc_t setDataWidth;
} SpiCtx_t, *PSpiCtx_t;

// Check if driver initialised
static inline bool SPI_isInitialised(PSpiCtx_t spi) {
    if (!spi) return false;
    return DriverContextCheckInit(spi->ctx);
}

// Set the direction of the MISO line
// - If the SPI is configured for 3-wire mode with a bidirectional data
//   line, this sets the MISO direction for all lanes.
// - Negative indicates error
static inline HpsErr_t SPI_setDirection(PSpiCtx_t spi, SpiMISODirection dir) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->setDirection) return (dir == SPI_MISO_OUT) ? ERR_NOSUPPORT : ERR_SUCCESS;
    return spi->setDirection(spi->ctx, dir);
}

// Set the SPI data width
// - If supported, allows changing of the current SPI data width.
// - Negative indicates error
static inline HpsErr_t SPI_setDataWidth(PSpiCtx_t spi, unsigned int width) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->setDataWidth) return ERR_NOSUPPORT;
    return spi->setDataWidth(spi->ctx, width);
}

// Configure slave selects
// - Auto slave select mode allows the hardware to control when the slave select signals are asserted
//   for example asserting for each word. The manual
// - Manual mode will immediately select the value of the slave selects such that those slaves with
//   bits set in the mask will be selected (polarity of slave-select is determined by hardware).
static inline HpsErr_t SPI_slaveSelect(PSpiCtx_t spi, bool autoSlaveSel, uint32_t slaveMask) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->slaveSelect) return ERR_NOSUPPORT;
    return spi->slaveSelect(spi->ctx, autoSlaveSel, slaveMask);
}

// Check if ready for write
// - Return ERR_BUSY if not ready for write.
// - Otherwise return ERR_SUCCESS or optionally available transmit space
// - Lane mask indicates which lanes to check. Max 31 lanes - MSB is ignored.
// - Negative indicates error
static inline HpsErrExt_t SPI_writeReady(PSpiCtx_t spi, uint32_t laneMask) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->writeReady) return ERR_NOSUPPORT;
    laneMask &= INT32_MAX;
    if (findHighestBit(laneMask) >= spi->laneCount) return ERR_BEYONDEND;
    return spi->writeReady(spi->ctx, laneMask);
}

// Write data to specified lanes
// - There must be one set of data words for each lane selected in the mask. Max 31 lanes - MSB is ignored.
//   - e.g. if the mask is 0x05, there must be two words, one for lane 0, and the second for lane 2.
//   - If the controller has only one lane, set the mask to 0x1.
// - The number of words per lane depends on the SPI configuration (e.g. long data bursts, or single word)
// - Transfer is write-only, read value will be ignored.
// - Negative indicates error.
static inline HpsErr_t SPI_write(PSpiCtx_t spi, uint32_t laneMask, uint32_t* data, SpiTransferType type) {
    if (!laneMask) return ERR_SUCCESS;
    if (!spi || !data) return ERR_NULLPTR;
    if (!spi->write) return ERR_NOSUPPORT;
    laneMask &= INT32_MAX;
    if (findHighestBit(laneMask) >= spi->laneCount) return ERR_BEYONDEND;
    return spi->write(spi->ctx, laneMask, data, type);
}

// Abort any running transfer immediately
static inline HpsErrExt_t SPI_abort(PSpiCtx_t spi) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->abort) return ERR_NOSUPPORT;
    return spi->abort(spi->ctx);
}

// Check if read has data ready
// - Returns number of words of data available
// - Lane mask indicates which lanes to check. Max 31 lanes - MSB is ignored.
// - If clearFlag is true, the status flag should be cleared
// - Negative indicates error
static inline HpsErrExt_t SPI_readReady(PSpiCtx_t spi, uint32_t laneMask) {
    if (!spi) return ERR_NULLPTR;
    if (!spi->readReady) return ERR_NOSUPPORT;
    laneMask &= INT32_MAX;
    if (findHighestBit(laneMask) >= spi->laneCount) return ERR_BEYONDEND;
    return spi->readReady(spi->ctx, laneMask);
}

// Read data from specified lanes
// - There must be one set of data word for each lane selected in the mask. Max 31 lanes - MSB is ignored.
//   - e.g. if the mask is 0x05, there must be two words, one for lane 0, and the second for lane 2.
//   - If the controller has only one lane, set the mask to 0x1.
// - The number of words per lane depends on the SPI configuration (e.g. long data bursts, or single word)
// - Transfer is read-only, write value will be ignored.
// - Negative indicates error.
static inline HpsErr_t SPI_read(PSpiCtx_t spi, uint32_t laneMask, uint32_t* data) {
    if (!laneMask) return ERR_SUCCESS;
    if (!spi || !data) return ERR_NULLPTR;
    if (!spi->read) return ERR_NOSUPPORT;
    laneMask &= INT32_MAX;
    if (findHighestBit(laneMask) >= spi->laneCount) return ERR_BEYONDEND;
    return spi->read(spi->ctx, laneMask, data);
}

#endif /* DRIVER_SPI_H_ */
