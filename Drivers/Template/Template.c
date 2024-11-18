/*
 * Template
 * ------------------------
 *
 * Template Driver
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 24/01/2024 | Creation of driver.
 *
 */

#include "Template.h"

#include "Util/bit_helpers.h"

/*
 * Registers
 */

// CSR Registers
#define TEMPLATE_REG_MYREG(base)    (*(((volatile uint32_t*)(base)) + (0x00 / sizeof(uint32_t))))

// Register bits
#define TEMPLATE_MYREG_ENABLE_MASK    0x1U
#define TEMPLATE_MYREG_ENABLE_OFFS    0

/*
 * Internal Functions
 */

//Cleanup
static void _Template_cleanup(TemplateCtx_t* ctx) {
    if (ctx->base) {
        TEMPLATE_REG_MYREG(ctx->base) = MaskClear(TEMPLATE_REG_MYREG(ctx->base), TEMPLATE_MYREG_ENABLE_MASK, TEMPLATE_MYREG_ENABLE_OFFS);
    }
}

/*
 * User Facing APIs
 */

// Initialise the Template Driver
//  - base is a pointer to the template thingy
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t Template_initialise(void* base, TemplateCtx_t** pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_Template_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    TemplateCtx_t* ctx = *pCtx;
    ctx->base = (unsigned int *)base;
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool Template_isInitialised(TemplateCtx_t* ctx) {
    return DriverContextCheckInit(ctx);
}

// Template API
//  - Does stuff.
HpsErr_t Template_api(TemplateCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Set source
    TEMPLATE_REG_MYREG(ctx->base) = MaskSet(TEMPLATE_REG_MYREG(ctx->base), TEMPLATE_MYREG_ENABLE_MASK, TEMPLATE_MYREG_ENABLE_OFFS);
    return ERR_SUCCESS;
}

