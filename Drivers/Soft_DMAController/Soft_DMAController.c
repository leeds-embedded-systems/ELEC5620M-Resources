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

#include "Soft_DMAController.h"
#include "Util/irq.h"

/*
 * Default Copy Functions
 */

#define SOFT_DMA_MEMCPY_DEF(type) \
static void* _Soft_DMA_memcpy_##type(void* dest, void* src, size_t length, const size_t word, void* ctx_) { \
    volatile type* outPtr = dest;                                       \
    volatile type* inPtr = src;                                         \
    size_t nWords = length / sizeof(type);                              \
    while (nWords--) {                                                  \
        *outPtr++ = *inPtr++;                                           \
    }                                                                   \
    return dest;                                                        \
}

SOFT_DMA_MEMCPY_DEF(uint8_t )
SOFT_DMA_MEMCPY_DEF(uint16_t)
SOFT_DMA_MEMCPY_DEF(uint32_t)
SOFT_DMA_MEMCPY_DEF(uint64_t)


/*
 * Internal Functions
 */

static HpsErr_t _Soft_DMA_setChunkSize(SoftDmaCtx_t* ctx, unsigned int chunkSize) {
    // Validate chunk size
    if (chunkSize == 0) return ERR_TOOSMALL;             // Can't be zero
    // Store the new size
    ctx->chunkSize = chunkSize;
    return ERR_SUCCESS;
}

// Send a chunk
static HpsErr_t _Soft_DMA_transferChunk(SoftDmaCtx_t* ctx) {
    //Must have a transfer
    if (!ctx->transferQueued) return ERR_NOTFOUND;
    if (!ctx->transferRunning) return ERR_SUCCESS;
    //Calculate chunk size. Smaller of max size and remaining length.
    size_t copyLen = (size_t)ctx->chunkSize * (size_t)ctx->wordSize;
    if (copyLen > ctx->length) copyLen = ctx->length;
    if (copyLen) {
        //If remaining length is non-zero, perform the copy.
        if (!ctx->copyFunc((void*)ctx->dest, (void*)ctx->source, copyLen, ctx->wordSize, ctx->copyFuncCtx)) {
            //Null return means copy failed.
            return ERR_IOFAIL;
        }
        //Update transfer
        ctx->dest   += copyLen;
        ctx->source += copyLen;
        ctx->length -= copyLen;
    }
    //If more to do, then we are still busy.
    if (ctx->length) return ERR_BUSY;
    //Otherwise we are done
    ctx->transferRunning = false;
    ctx->transferQueued = false;
    return ERR_SUCCESS;
}

// Start a transfer
static HpsErr_t _Soft_DMA_startTransfer(SoftDmaCtx_t* ctx) {
    // Check if accepting more
    if (ctx->transferRunning) return ERR_BUSY; // Can't start until previous done
    // Start the transfer.
    ctx->transferRunning = (ctx->length > 0);
    HpsErr_t status = _Soft_DMA_transferChunk(ctx);
    // If successful, then transfer has already completed, so return skipped, otherwise
    // return status code
    return ERR_IS_SUCCESS(status) ? ERR_SKIPPED : status;
}

/*
 * User Facing APIs
 */

// Initialise DMA Controller Driver
//  - wordSize is the width of the DMA word in bytes.
//  - chunkSize is the size of each chunk in wordSize words transferred when polling.
//     - This can be overridden later by calling the Soft_DMA_setChunkSize() API.
//  - copyFunc/copyFuncCtx are an optional pointer to a custom memory copy function.
//     - Function will be called as copyFunc(destPtr, srcPtr, lengthInBytes, copyFuncCtx)
//     - The source, destination, and length in bytes are guaranteed to be a multiple of the wordSize.
//     - If copyFunc is NULL, a default copy function will be used.
//     - This allows overriding of how the copy works in cases where device specific
//       behaviour is required. For example copying to a FIFO pointer, you could make a
//       memcpy function that doesn;t increment the destination address.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t Soft_DMA_initialise(SoftDMAWordSize wordSize, unsigned int chunkSize, SoftDmaMemcpyFunc_t copyFunc, void* copyFuncCtx, SoftDmaCtx_t** pCtx) {
    //Word and chunk size must be non-zero
    if (!wordSize || !chunkSize) return ERR_TOOSMALL;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocate(pCtx);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    SoftDmaCtx_t* ctx = *pCtx;
    ctx->dma.ctx = ctx;
    ctx->dma.setupTransfer = (DmaXferFunc_t)&Soft_DMA_setupTransfer;
    ctx->dma.startTransfer = (DmaXferStartFunc_t)&Soft_DMA_startTransfer;
    ctx->dma.abortTransfer = (DmaAbortFunc_t)&Soft_DMA_abort;
    ctx->dma.transferBusy = (DmaStatusFunc_t)&Soft_DMA_busy;
    ctx->dma.transferDone = (DmaStatusFunc_t)&Soft_DMA_completed;
    //Validate and set the word size. Also pick appropriate default copy function
    SoftDmaMemcpyFunc_t defaultCopyFunc;
    switch (wordSize) {
        case SOFT_DMA_WORDSIZE_8BIT:
            defaultCopyFunc = (SoftDmaMemcpyFunc_t)&_Soft_DMA_memcpy_uint8_t;
            break;
        case SOFT_DMA_WORDSIZE_16BIT:
            defaultCopyFunc = (SoftDmaMemcpyFunc_t)&_Soft_DMA_memcpy_uint16_t;
            break;
        case SOFT_DMA_WORDSIZE_32BIT:
            defaultCopyFunc = (SoftDmaMemcpyFunc_t)&_Soft_DMA_memcpy_uint32_t;
            break;
        case SOFT_DMA_WORDSIZE_64BIT:
            defaultCopyFunc = (SoftDmaMemcpyFunc_t)&_Soft_DMA_memcpy_uint64_t;
            break;
        default:
            return DriverContextInitFail(pCtx, ERR_NOSUPPORT);
    }
    ctx->wordSize = wordSize;
    //Save chunk size
    status = _Soft_DMA_setChunkSize(ctx, chunkSize);
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    //Save user copy function function if provided, or use default
    if (copyFunc) {
        ctx->copyFunc = copyFunc;
        ctx->copyFuncCtx = copyFuncCtx;
    } else {
        ctx->copyFunc = defaultCopyFunc;
        ctx->copyFuncCtx = ctx;
    }
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool Soft_DMA_isInitialised(SoftDmaCtx_t* ctx) {
    return DriverContextCheckInit(ctx);
}

// Set the size of the polled DMA chunk
//  - Size is specified in units of words (wordSize set during initialisation)
//  - Returns ERR_SUCCESS if updated successfully
//  - Returns ERR_BUSY if a DMA transfer is already running (can't change size during run).
HpsErr_t Soft_DMA_setChunkSize(SoftDmaCtx_t* ctx, unsigned int chunkSize) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Can't be running
    if (ctx->transferRunning) return ERR_BUSY;
    //Save chunk size
    return _Soft_DMA_setChunkSize(ctx, chunkSize);
}

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
HpsErr_t Soft_DMA_setupTransfer(SoftDmaCtx_t* ctx, DmaChunk_t* xfer, bool autoStart) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Can't be running
    if (ctx->transferRunning) return ERR_BUSY;
    //Ensure transfer requests are aligned to DMA word size
    if (!xfer->writeAddr || !xfer->readAddr) return ERR_NULLPTR;
    if (!addressIsAligned64b(xfer->readAddr,  ctx->wordSize)) return ERR_ALIGNMENT;
    if (!addressIsAligned64b(xfer->writeAddr, ctx->wordSize)) return ERR_ALIGNMENT;
    if (!addressIsAligned64b(xfer->length,    ctx->wordSize)) return ERR_ALIGNMENT;
    //Ensure source and destination don't overlap
    if ((xfer->writeAddr - xfer->readAddr) < ctx->length) return ERR_NOSUPPORT;
    //Can't support 64-bit transfers
    if (xfer->readAddr  > UINTPTR_MAX) return ERR_TOOBIG;
    if (xfer->writeAddr > UINTPTR_MAX) return ERR_TOOBIG;
    if (xfer->length    > UINT32_MAX ) return ERR_TOOBIG;
    //Ensure the transfer doesn't wrap through NULL
    if (xfer->readAddr  + xfer->length > (1ULL << 32)) return ERR_BEYONDEND;
    if (xfer->writeAddr + xfer->length > (1ULL << 32)) return ERR_BEYONDEND;
    //Check if chunk size provided
    if (xfer->params) {
        status = _Soft_DMA_setChunkSize(ctx, (unsigned int)xfer->params);
        if (ERR_IS_ERROR(status)) return status;
    }
    //Set transfer parameters
    ctx->source = (uintptr_t)xfer->readAddr;
    ctx->dest   = (uintptr_t)xfer->writeAddr;
    ctx->length = (size_t)(xfer->length);
    ctx->transferQueued = true;
    //If not auto starting, then done
    if (!autoStart) return ERR_SUCCESS;
    //Otherwise start the transfer
    return _Soft_DMA_startTransfer(ctx);
}

// Start the previously configured transfer
//  - If controller is busy and not able to start, returns ERR_BUSY.
//  - Will return ERR_NOTFOUND if no transfer queued.
//  - If completes immediately (length smaller than chunkSize) the API will
//    return ERR_SKIPPED to indicate that it is complete and there is no
//    need to call Soft_DMA_completed().
HpsErr_t Soft_DMA_startTransfer(SoftDmaCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Start the transfer (skips if we don't have one)
    return _Soft_DMA_startTransfer(ctx);
}

// Check if the DMA controller is busy
//  - Will return ERR_BUSY if the DMA controller is busy.
HpsErr_t Soft_DMA_busy(SoftDmaCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if busy
    return ctx->transferRunning ? ERR_BUSY : ERR_SUCCESS;
}

// Check if the DMA controller completed
//  - If not all chunks have been copied yet, calling this will first copy another chunk.
//  - Calling this function will return TRUE if the DMA has just completed
//  - It will return ERR_BUSY if (a) the transfer has not completed, or (b) the completion has already been acknowledged
//  - If ERR_SUCCESS is returned, the done flag will be cleared automatically acknowledging the completion.
HpsErr_t Soft_DMA_completed(SoftDmaCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if running
    if (!ctx->transferRunning) return ERR_BUSY; // Already acknowledged
    //Send the next chunk
    return _Soft_DMA_transferChunk(ctx);
}

// Issue an abort request to the DMA controller
//  - DMA_ABORT_NONE does nothing.
//  - Both DMA_ABORT_SAFE and DMA_ABORT_FORCE clear any remaining chunks and effectively stops the transfer immediately
//  - Returns ERR_ABORTED to indicate abort completed immediately.
HpsErr_t Soft_DMA_abort(SoftDmaCtx_t* ctx, DmaAbortType abort) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (abort == DMA_ABORT_NONE) return ERR_SKIPPED;
    //Stopped immediately.
    ctx->transferQueued = false;
    ctx->transferRunning = false;
    return ERR_ABORTED;
}

