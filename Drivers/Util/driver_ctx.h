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

#ifndef DRIVER_CTX_H_
#define DRIVER_CTX_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "Util/error.h"

/*
 * Example driver
 *

    // Context implementation:

    // Driver context
    typedef struct {
        // Context Header
        DrvCtx_t header;
        // Context Body
        volatile unsigned int* base;
        ... any fields you want
    } MyDriverCtx_t;

    #define MY_DATA_REG (0/sizeof(unsigned int))
    #define MY_IRQ_REG  (4/sizeof(unsigned int))


    // Example driver cleanup

    // Cleanup function called when driver destroyed.
    //  - Disables any hardware and interrupts.
    //  - Free any allocated memory
    void _MY_cleanup(MyDriverCtx_t* ctx) {
        if (ctx->base) {
            // Disable any interrupts
            ctx->base[MY_IRQ_REG] = 0;
        }
    }

    // Example driver initialisation routine

    // Initialise My driver
    //  - base is a pointer to my hardwar
    //  - Returns Util/error Code
    //  - Returns context pointer to *ctx
    HpsErr_t MY_initialise(void* base, MyDriverCtx_t** pCtx) {
        //Ensure user pointers valid
        if (!base) return ERR_NULLPTR;
        if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
        //Allocate the driver context, validating return value.
        HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_MY_cleanup);
        if (ERR_IS_ERROR(status)) return status;
        //Save base address pointers
        MyDriverCtx_t* ctx = *pCtx;
        ctx->base = (unsigned int*)base;

        ... Any other initialisation "stuff" ...

        ... Maybe something went wrong? If so init failure
        status = MyFunc(ctx);    
        if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);

        //Initialised
        DriverContextSetInit(ctx);
        return status;
    }

    // Check if driver initialised
    //  - Returns true if driver previously initialised
    bool MY_isInitialised(MyDriverCtx_t* ctx) {
        return DriverContextCheckInit(ctx);
    }

    // Example driver API

    // Read my value
    // - Reads a certain value from my hardware and returns it via *val
    HpsErr_t MY_queueIsInterruptable(MyDriverCtx_t* ctx, unsigned int *val) {
        if (!val) return ERR_NULLPTR;
        //Ensure context valid and initialised
        HpsErr_t status = DriverContextValidate(ctx);
        if (ERR_IS_ERROR(status)) return status;
        //Read the value
        *val = ctx->base[MY_DATA_REG];
        return ERR_SUCCESS;
    }

 *
 */


/*
 * Driver context header
 */

// Cleanup function template
typedef void (*ContextCleanupFunc_t)(void* ctx);

// This must be the first field in any driver context structure
typedef struct {
    // Magic word
    unsigned int __magic;
    // Size of the context
    unsigned int __size;
    // Cleanup function called when context is about to be freed.
    ContextCleanupFunc_t __destroy;
    // Whether initialised
    bool initialised;
} DrvCtx_t;


/*
 * Internal Functions. Use Macros Below
 */
// Allocate context
// - Remember to set initialised flag once driver specific context is initialised
// - Returns success or error code
HpsErr_t DRV_allocateContext(unsigned int drvSize, DrvCtx_t** pCtx, ContextCleanupFunc_t destroy);

// Cleanup a context
// - Will set *pCtx to null once freed.
// - Returns success or error code
HpsErr_t DRV_freeContext(DrvCtx_t** pCtx);

// Cleanum context with status
// - Same as DRV_freeContext, except returns
//   provided status code.
static inline HpsErr_t DRV_freeContextWithStatus(DrvCtx_t** pCtx, HpsErr_t retVal) {
    DRV_freeContext(pCtx);
    return retVal;
}

// Check if the driver context is initialised
bool DRV_isInitialised(DrvCtx_t* ctx);

// Checks that a driver context is valid
// - Returns success or error code
HpsErr_t DRV_checkContext(DrvCtx_t* ctx);


/*
 * User Macros for Driver Functions
 */

// Allocate a context pointer
// - Returns HpsErr_t
#define DriverContextAllocate(pCtx) \
    DRV_allocateContext(sizeof(**(pCtx)),(DrvCtx_t**)(pCtx),NULL)

// Allocate a context pointer with cleanup function
// - Returns HpsErr_t
#define DriverContextAllocateWithCleanup(pCtx, cleanupFunc) \
    DRV_allocateContext(sizeof(**(pCtx)),(DrvCtx_t**)(pCtx),(ContextCleanupFunc_t)(cleanupFunc))

// Free a context pointer
// - Call during initialisation function if there is an
//   error after the context has been allocated.
// - Call when finished with a driver context to clean it up.
// - Returns HpsErr_t
#define DriverContextFree(pCtx) \
    DRV_freeContext((DrvCtx_t**)(pCtx))

// Driver initialise failed
// - Call after Allocate but before SetInit if something
//   goes wrong in the constructor.
// - This will clean up the allocated context then return
//   the provided return value
#define DriverContextInitFail(pCtx, retVal) \
    DRV_freeContextWithStatus((DrvCtx_t**)(pCtx), retVal)

// Mark driver context as initialised.
// - Call once at end of driver initialisation function
#define DriverContextSetInit(ctx) \
    ((DrvCtx_t*)(ctx))->initialised = true

// Check if context initialised
//  - returns bool
#define DriverContextCheckInit(ctx) \
    DRV_isInitialised((DrvCtx_t*)(ctx))

// Ensure driver context is valid
// - Returns HpsErr_t
#define DriverContextValidate(ctx) \
    DRV_checkContext((DrvCtx_t*)(ctx))

#endif /* DRIVER_CTX_H */

