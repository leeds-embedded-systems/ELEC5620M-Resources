/* Generic DMA Driver Interface
 * ----------------------------
 *
 * Provides a common interface for different DMA
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
 * 15/02/2024 | Creation of driver.
 */

#ifndef DRIVER_DMA_H_
#define DRIVER_DMA_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

//Transfer description
typedef struct {
    uint64_t readAddr;
    uint64_t writeAddr;
    uint64_t length;
    bool     isLast;
    void*    params;  // Optional driver specific parameters
} DmaChunk_t, *PDmaChunk_t;

//Abort types
typedef enum {
    DMA_ABORT_NONE,
    DMA_ABORT_SAFE,
    DMA_ABORT_FORCE
} DmaAbortType;

// IO Function Templates
typedef HpsErr_t (*DmaXferFunc_t)     (void* ctx, PDmaChunk_t xfer, bool autoStart);
typedef HpsErr_t (*DmaXferStartFunc_t)(void* ctx);
typedef HpsErr_t (*DmaXferSpaceFunc_t)(void* ctx, unsigned int* space);
typedef HpsErr_t (*DmaAbortFunc_t)    (void* ctx, DmaAbortType abort);
typedef HpsErr_t (*DmaStatusFunc_t)   (void* ctx);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Perform a transfer
    DmaXferSpaceFunc_t transferSpace;
    DmaXferFunc_t      setupTransfer;
    DmaXferStartFunc_t startTransfer;
    DmaAbortFunc_t     abortTransfer;
    // Status Functions
    DmaStatusFunc_t transferBusy;
    DmaStatusFunc_t transferDone;
    DmaStatusFunc_t transferAborted;
} DmaCtx_t, *PDmaCtx_t;

// Check if driver initialised
static inline bool DMA_isInitialised(PDmaCtx_t dma) {
    if (!dma) return false;
    return DriverContextCheckInit(dma->ctx);
}

// Check if there is space to perform a transfer
// - Returns the amount of space available for transfers
// - Returns ERR_NOSPACE if the space is 0.
static inline HpsErr_t DMA_transferSpace(PDmaCtx_t dma, unsigned int* space) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->transferSpace) return ERR_NOSUPPORT;
    return dma->transferSpace(dma->ctx, space);
}

// Configure a DMA transfer from the DmaChunk structure
// - Can optionally request the transfer be started immediately
// - Returns ERR_BUSY if not enough space to start a new Xfer
static inline HpsErr_t DMA_setupTransfer(PDmaCtx_t dma, PDmaChunk_t xfer, bool autoStart) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->setupTransfer) return ERR_NOSUPPORT;
    return dma->setupTransfer(dma->ctx, xfer, autoStart);
}

// Perform the previously configured transfer
// - Returns ERR_BUSY if not enough space to start a new Xfer
static inline HpsErr_t DMA_startTransfer(PDmaCtx_t dma) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->startTransfer) return ERR_NOSUPPORT;
    return dma->startTransfer(dma->ctx);
}

// Abort the current transfer
// - Allows setting or clearing an abort flag.
// - Check transferAborted to find out when abort is done. 
static inline HpsErr_t DMA_abortTransfer(PDmaCtx_t dma, DmaAbortType abort) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->abortTransfer) return ERR_NOSUPPORT;
    return dma->abortTransfer(dma->ctx, abort);
}

// Check if the controller is busy
// - Returns ERR_BUSY if a transfer is in progress.
// - Non-stateful. Can be used to check at any time.
static inline HpsErr_t DMA_transferBusy(PDmaCtx_t dma) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->transferBusy) return ERR_NOSUPPORT;
    return dma->transferBusy(dma->ctx);
}

// Check if the last transfer is done
// - Returns ERR_SUCCESS once done.
// - Uses self clearing flag, so should can only be used to check
//   that a previously started transfer has just finished.
static inline HpsErr_t DMA_transferDone(PDmaCtx_t dma) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->transferDone) return ERR_NOSUPPORT;
    return dma->transferDone(dma->ctx);
}

// Check if the abort is complete
// - Returns ERR_SUCCESS once done.
// - Uses self clearing flag, so should can only be used to check
//   that a previously started abort request has just finished.
static inline HpsErr_t DMA_transferAborted(PDmaCtx_t dma) {
    if (!dma) return ERR_NULLPTR;
    if (!dma->transferAborted) return ERR_NOSUPPORT;
    return dma->transferAborted(dma->ctx);
}

#endif /* DRIVER_UART_H_ */
