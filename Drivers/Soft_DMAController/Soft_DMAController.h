/*
 * Software DMA Controller Driver
 * ------------------------------
 * 
 * Driver for a software DMA controller. Transfers
 * are performed by the CPU, essentially using the
 * memcpy function under the hood.
 * 
 * The driver allows performing CPU transfers using
 * the generic DMA interface in cases where a hardware
 * DMA controller is not available.
 * 
 * During init, the maximum chunk size is specified,
 * which sets the maximum amount of data the driver will
 * copy each time the API is polled by calling the
 * completed() API. Transfers shorter than this chunk
 * size will complete immediately when startTransfer()
 * API.
 * 
 * Selecting this chunk size allows performing a large
 * copy whilst performing other tasks by periodically
 * returning from the transfer routines. This can also
 * help with watchdog handling.
 * 
 * If the chunk size is small, the API can also be used
 * to perform a transfer using e.g. a timer interrupt
 * to transfer each chunk.
 * 
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+--------------------------------------------
 * 27/08/2025 | Add word size argument to copy function API
 * 02/10/2024 | Creation of driver.
 *
 */

#ifndef SOFT_DMACONTROLLER_H_
#define SOFT_DMACONTROLLER_H_

#include "Util/driver_ctx.h"
#include "Util/driver_dma.h"
#include "Util/bit_helpers.h"
#include "Util/macros.h"

typedef enum {
    SOFT_DMA_WORDSIZE_8BIT  = sizeof(uint8_t ),
    SOFT_DMA_WORDSIZE_16BIT = sizeof(uint16_t),
    SOFT_DMA_WORDSIZE_32BIT = sizeof(uint32_t),
    SOFT_DMA_WORDSIZE_64BIT = sizeof(uint64_t),
} SoftDMAWordSize;

// Memory Copy Function Pointer
//  - dest is a pointer to the start of the destination memory
//  - src  is a pointer to the start of the source memory
//  - len  is length of copy in bytes
//  - word is the size of a single word
//  - ctx  is an optional user parameter (copyFuncCtx)
//  - Source/Destination/Length are guaranteed to be aligned to 'word' bytes and non-zero
//  - Return NULL if there was an error, or 'dest' otherwise.
typedef void* (*SoftDmaMemcpyFunc_t)(void* dest, void* src, size_t len, const size_t word, void* ctx);

typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    SoftDMAWordSize wordSize;
    unsigned int chunkSize;
    SoftDmaMemcpyFunc_t copyFunc;
    void* copyFuncCtx;
    DmaCtx_t dma;
    //State
    bool transferQueued;
    bool transferRunning;
    //Current transfer
    uintptr_t source;
    uintptr_t dest;
    size_t   length;
} SoftDmaCtx_t;

// Initialise DMA Controller Driver
//  - wordSize is the width of the DMA word in bytes.
//  - chunkSize is the size of each chunk in wordSize words transferred when polling.
//     - This can be overridden later by calling the Soft_DMA_setChunkSize() API.
//  - copyFunc/copyFuncCtx are an optional pointer to a custom memory copy function.
//     - Function will be called as copyFunc(destPtr, srcPtr, lengthInBytes, copyFuncCtx)
//     - The source, destination, and length in bytes are guaranteed to be a multiple of the wordSize.
//     - If copyFunc is NULL, a default copy function will be used.
//     - copyFuncCtx is user defined, can be NULL if not required.
//     - This allows overriding of how the copy works in cases where device specific
//       behaviour is required. For example copying to a FIFO pointer, you could make a
//       memcpy function that doesn;t increment the destination address.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t Soft_DMA_initialise(SoftDMAWordSize wordSize, unsigned int chunkSize, SoftDmaMemcpyFunc_t copyFunc, void* copyFuncCtx, SoftDmaCtx_t** pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool Soft_DMA_isInitialised(SoftDmaCtx_t* ctx);

// Set the size of the polled DMA chunk
//  - Size is specified in units of words (wordSize set during initialisation)
//  - Returns ERR_SUCCESS if updated successfully
//  - Returns ERR_BUSY if a DMA transfer is already running (can't change size during run).
HpsErr_t Soft_DMA_setChunkSize(SoftDmaCtx_t* ctx, unsigned int chunkSize);

// Configure a DMA transfer
//  - If a transfer has already been queued but is not running it will be cancelled.
//  - If the controller is already running, will return ERR_BUSY.
//  - If autoStart is true, will call Soft_DMA_startTransfer() if transfer is setup
//    successfully
//  - Source buffer pointed to by xfer must remain valid throughout the transfer.
//  - Can optionally override chunkSize by setting: `xfer->params = (void*)chunkSize;`
//  - If autoStart completes immediately (length smaller than chunkSize) the API will
//    return ERR_SKIPPED to indicate that it is complete and there is no need to call
//    Soft_DMA_completed().
HpsErr_t Soft_DMA_setupTransfer(SoftDmaCtx_t* ctx, DmaChunk_t* xfer, bool autoStart);

// Start the previously configured transfer
//  - If controller is busy and not able to start, returns ERR_BUSY.
//  - Will return ERR_NOTFOUND if no transfer queued.
//  - If completes immediately (length smaller than chunkSize) the API will
//    return ERR_SKIPPED to indicate that it is complete and there is no
//    need to call Soft_DMA_completed().
HpsErr_t Soft_DMA_startTransfer(SoftDmaCtx_t* ctx);

// Check if the DMA controller is busy
//  - Will return ERR_BUSY if the DMA controller is busy.
HpsErr_t Soft_DMA_busy(SoftDmaCtx_t* ctx);

// Check if the DMA controller completed
//  - If not all chunks have been copied yet, calling this will first copy another chunk.
//  - Calling this function will return TRUE if the DMA has just completed
//  - It will return ERR_BUSY if (a) the transfer has not completed, or (b) the completion has already been acknowledged
//  - If ERR_SUCCESS is returned, the done flag will be cleared automatically acknowledging the completion.
HpsErr_t Soft_DMA_completed(SoftDmaCtx_t* ctx);

// Issue an abort request to the DMA controller
//  - DMA_ABORT_NONE does nothing.
//  - Both DMA_ABORT_SAFE and DMA_ABORT_FORCE clear any remaining chunks and effectively stops the transfer immediately
//  - Returns ERR_ABORTED to indicate abort completed immediately.
HpsErr_t Soft_DMA_abort(SoftDmaCtx_t* ctx, DmaAbortType abort);

#endif /* SOFT_DMACONTROLLER_H_ */

