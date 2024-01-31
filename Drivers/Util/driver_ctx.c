/* Generic Driver Context
 * ----------------------
 *
 * Provides common driver context logic for drivers using
 * context pointers and functions returning Util/error Code.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 29/12/2023 | Creation of driver.
 */

#include "driver_ctx.h"

#define DRV_MAGIC_HEADER_WORD  0xF00DCAFE

#define DRV_ValidHeader(ctx) ((ctx)->__magic == DRV_MAGIC_HEADER_WORD)

// Allocate context
HpsErr_t DRV_allocateContext(unsigned int drvSize, PDrvCtx_t* pCtx, ContextCleanupFunc_t destroy) {
    // Must have a return pointer
    if (!pCtx) return ERR_NULLPTR;
    *pCtx = NULL;
    // Allocate the main context
    PDrvCtx_t ctx = (PDrvCtx_t)calloc(1, drvSize);
    if (!ctx) return ERR_ALLOCFAIL;
    // Allocated
    ctx->__magic = DRV_MAGIC_HEADER_WORD;
    ctx->__size = drvSize;
    ctx->__destroy = destroy;
    *pCtx = ctx;
    return ERR_SUCCESS;
}

// Cleanup a context
HpsErr_t DRV_freeContext(PDrvCtx_t* pCtx) {
    // Must have a return pointer
    if (!pCtx) return ERR_NULLPTR;
    PDrvCtx_t ctx = *pCtx;
    // Success if already null.
    if (!ctx) return ERR_SUCCESS;
    // Error if invalid
    if (!DRV_ValidHeader(ctx)) return ERR_BADDEVICE;
    // Call user cleanup if we have one
    if (ctx->__destroy) ctx->__destroy(ctx);
    // Be free driver context
    ctx->initialised = false;
    ctx->__magic = 0x0;
    free(ctx);
    *pCtx = NULL;
    return ERR_SUCCESS;
}

// Check if the driver context is initialised
bool DRV_isInitialised(PDrvCtx_t ctx) {
    return (ctx && DRV_ValidHeader(ctx) && ctx->initialised);
}

// Get the driver context, validating that it is initialised
HpsErr_t DRV_checkContext(PDrvCtx_t ctx) {
    if (!ctx) return ERR_NULLPTR;
    if (!DRV_ValidHeader(ctx)) return ERR_BADDEVICE;
    if (!ctx->initialised) return ERR_NOINIT;
    return ERR_SUCCESS;
}

