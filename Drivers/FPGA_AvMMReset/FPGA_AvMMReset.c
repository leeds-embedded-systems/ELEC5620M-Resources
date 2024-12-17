/*
 * FPGA Avalon-MM Reset Controller
 * -------------------------------
 *
 * APIs for controlling the Av-MM Reset PIO core.
 * 
 * Provides a simple API to assert or release reset.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 13/12/2024 | Creation of driver.
 *
 */

#include "FPGA_AvMMReset.h"

#include "Util/bit_helpers.h"

/*
 * Register Map
 */
#define AVMM_REG_RESET   (0 / sizeof(unsigned int))

#define AVMM_RESET_MASK  0x1U
#define AVMM_RESET_OFFS  0

/*
 * Internal Functions
 */

static void _FPGA_Reset_cleanup(FPGAAvMMResetCtx_t* ctx) {
    //Disable interrupts and reset default output states
    if (ctx->base) {
        ctx->base[AVMM_REG_RESET] = MaskInsert(!ctx->defaultAssert, AVMM_RESET_MASK, AVMM_RESET_OFFS);
    }
}

/*
 * User Facing APIs
 */

// Initialise Av-MM Reset Driver
//  - base is a pointer to the reset control CSR
//  - defaultAssert is the state restored on cleanup.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t FPGA_Reset_initialise(void* base, bool defaultAssert, FPGAAvMMResetCtx_t** pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_FPGA_Reset_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    FPGAAvMMResetCtx_t* ctx = *pCtx;
    ctx->base = (unsigned int *)base;
    ctx->defaultAssert = defaultAssert;
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_Reset_isInitialised(FPGAAvMMResetCtx_t* ctx) {
    return DriverContextCheckInit(ctx);
}

//Set reset state
// - If assertReset is true, will apply reset. Otherwise releases reset.
HpsErr_t FPGA_Reset_configureReset(FPGAAvMMResetCtx_t* ctx, bool assertReset) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Set source
    ctx->base[AVMM_REG_RESET] = MaskInsert(!assertReset, AVMM_RESET_MASK, AVMM_RESET_OFFS);
    return ERR_SUCCESS;
}

//Check if reset is asserted
// - Returns ERR_TRUE if in reset, else ERR_FALSE.
HpsErr_t FPGA_Reset_isAsserted(FPGAAvMMResetCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Get source
    return MaskCheck(ctx->base[AVMM_REG_RESET], AVMM_RESET_MASK, AVMM_RESET_OFFS) ? ERR_FALSE : ERR_TRUE;
}
