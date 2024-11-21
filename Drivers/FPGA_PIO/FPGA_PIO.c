/*
 * FPGA PIO Controller Driver
 * ---------------------------
 *
 * Driver for writing to generic PIO controller
 * core (e.g. avmm_pio_hw).
 *
 * The PIO controller has a single data registers
 * shared between input and output. If there is an
 * input register, then we cannot read the state of
 * the output register directly.
 * 
 * To combat this, a cached value of the output
 * register has been added to the driver context. If
 * it is not possible to read the output register
 * directly, the cached value is used in any R-M-W
 * operations.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-----------------------------------------
 * 21/11/2024 | Add output cache to allow getOutput in all modes
 * 21/02/2024 | Conversion from struct to array indexing
 * 30/12/2023 | Creation of driver.
 *
 */

#include "FPGA_PIO.h"

#include "Util/irq.h"
#include "Util/bit_helpers.h"

#include "FPGA_PIORegs.h"

/*
 * Internal Functions
 */

static void _FPGA_PIO_cleanup(FPGAPIOCtx_t* ctx) {
    //Disable interrupts and reset default output states
    if (ctx->base) {
        ctx->base[GPIO_INTR_MASK] = 0x0;
        if (ctx->pioType & FPGA_PIO_DIRECTION_OUT) {
            ctx->base[GPIO_OUTPUT] = ctx->initPort;
        }
        if (ctx->pioType == FPGA_PIO_DIRECTION_BIDIR) {
            ctx->base[GPIO_DIRECTION] = ctx->initDir;
        }
    }
}

static HpsErr_t _FPGA_PIO_setDirection(FPGAPIOCtx_t* ctx, unsigned int dir, unsigned int mask) {
    //R-M-W direction
    unsigned int curVal = ctx->base[GPIO_DIRECTION];
    ctx->base[GPIO_DIRECTION] = ((dir & mask) | (curVal & ~mask));
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getDirection(FPGAPIOCtx_t* ctx, unsigned int* dir, unsigned int mask) {
    *dir = ctx->base[GPIO_DIRECTION] & mask;
    return ERR_SUCCESS;
}

static unsigned int _FPGA_PIO_getOutputRegValue(FPGAPIOCtx_t*ctx) {
    //Check if we have the capability to read the output port
    if (ctx->usePortCache) {
        // Rely on our cached value as we cannot read back the port register value.
        return ctx->outPort;
    } else {
        // Can read output register, so use the real current value in case it was changed by something else
        return ctx->base[GPIO_OUTPUT];
    }
}

static HpsErr_t _FPGA_PIO_setOutput(FPGAPIOCtx_t* ctx, unsigned int port, unsigned int mask) {
    //Configure output
    if (mask != UINT32_MAX) {
        //Masking required, integrate the new value with our current output register value
        //based on masked bits.
        unsigned int curVal = _FPGA_PIO_getOutputRegValue(ctx);
        port = ((port & mask) | (curVal & ~mask));
    }
    //Update the output register, and our shadow copy in case we can't read it back.
    ctx->base[GPIO_OUTPUT] = port;
    ctx->outPort           = port;
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_toggleOutput(FPGAPIOCtx_t* ctx, unsigned int mask) {
    //Toggle outputs
    ctx->base[GPIO_OUTPUT] = _FPGA_PIO_getOutputRegValue(ctx) ^ mask;
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getOutput(FPGAPIOCtx_t* ctx, unsigned int* port, unsigned int mask) {
    //Get output
    *port = _FPGA_PIO_getOutputRegValue(ctx) & mask;
    return ERR_SUCCESS;
}


static HpsErr_t _FPGA_PIO_getInput(FPGAPIOCtx_t* ctx, unsigned int* in, unsigned int mask) {
    //Get input
    if (ctx->splitData) {
        *in = ctx->base[GPIO_SPLITINPUT] & mask;
    } else {
        *in = ctx->base[GPIO_INPUT     ] & mask;
    }
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_setInterruptEnable(FPGAPIOCtx_t* ctx, unsigned int flags, unsigned int mask) {
    //Before changing anything we need to mask global interrupts temporarily
    HpsErr_t irqStatus = IRQ_globalEnable(false);
    //Modify the enable flags
    unsigned int enBits = ctx->base[GPIO_INTR_MASK];
    ctx->base[GPIO_INTR_MASK] = ((flags & mask) | (enBits & ~mask));
    //Re-enable interrupts if they were enabled
    IRQ_globalEnable(ERR_IS_SUCCESS(irqStatus));
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int* flags, unsigned int mask, bool autoClear) {
    //Read the flags
    unsigned int edgeCapt = ctx->base[GPIO_INTR_FLAGS] & mask;
    //Clear the flags if requested
    if (ctx->hasEdge && edgeCapt && autoClear) {
        ctx->base[GPIO_INTR_FLAGS] = edgeCapt;
    }
    //Return the flags if there is somewhere to return them to.
    if (flags) *flags = edgeCapt;
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_clearInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int mask) {
    ctx->base[GPIO_INTR_FLAGS] = mask;
    return ERR_SUCCESS;
}

/*
 * User Facing APIs
 */


// Initialise FPGA PIO Driver
//  - base is a pointer to the PIO csr
//  - pioType indicates whether we have inputs, outputs, both, and/or a direction pin.
//  - splitData indicates a special case where the direction register is used for reading inputs instead of the data register.
//  - hasEdge indicates whether we have an edge capture capability
//  - hasIrq indicates whether we have an interrupt capability
//  - hasBitset indicates whether this PIO uses the extended CSR
//  - dir is the default direction for GPIO pins
//  - port is the default output value for GPIO pins
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t FPGA_PIO_initialise(void* base, FPGAPIODirectionType pioType, bool splitData, bool hasBitset, bool hasEdge, bool hasIrq, unsigned int dir, unsigned int port, FPGAPIOCtx_t** pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    if ((pioType == FPGA_PIO_DIRECTION_BIDIR) && splitData) return ERR_WRONGMODE;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_FPGA_PIO_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    FPGAPIOCtx_t* ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->pioType = pioType;
    ctx->splitData = splitData;
    ctx->hasBitset = hasBitset;
    ctx->hasEdge = hasEdge;
    ctx->hasIrq = hasIrq;
    ctx->initDir = dir;
    ctx->initPort = port;
    //Ensure no interrupts are enabled.
    ctx->base[GPIO_INTR_MASK] = 0x0;
    ctx->base[GPIO_INTR_FLAGS] = UINT32_MAX;
    //Populate the GPIO structure
    ctx->gpio.ctx = ctx;
    if (pioType == FPGA_PIO_DIRECTION_BIDIR) {
        //Enable direction APIs
        ctx->gpio.getDirection = (GpioReadFunc_t)&_FPGA_PIO_getDirection;
        ctx->gpio.setDirection = (GpioWriteFunc_t)&_FPGA_PIO_setDirection;
        //Initialise the direction register
        ctx->base[GPIO_DIRECTION] = dir;
    }
    if (pioType & FPGA_PIO_DIRECTION_OUT) {
        //Enable write APIs
        ctx->gpio.getOutput    = (GpioReadFunc_t)&_FPGA_PIO_getOutput;
        ctx->gpio.setOutput    = (GpioWriteFunc_t)&_FPGA_PIO_setOutput;
        ctx->gpio.toggleOutput = (GpioToggleFunc_t)&_FPGA_PIO_toggleOutput;
        //Initialise the output register
        ctx->base[GPIO_OUTPUT] = port;
        ctx->outPort           = port;
    }
    if (pioType & FPGA_PIO_DIRECTION_IN) {
        //Enable read APIs
        ctx->gpio.getInput = (GpioReadFunc_t)&_FPGA_PIO_getInput;
    }
    if (pioType & FPGA_PIO_DIRECTION_BOTH) {
        // If we have both directions, we must use the cached port copy for reads
        // unless the hardware uses the special splitData mode.
        ctx->usePortCache = !splitData;
    }
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_PIO_isInitialised(FPGAPIOCtx_t* ctx) {
    return DriverContextCheckInit(ctx);
}


//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_setDirection(FPGAPIOCtx_t* ctx, unsigned int dir, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (ctx->pioType != FPGA_PIO_DIRECTION_BIDIR) return ERR_NOSUPPORT;
    //R-M-W direction
    return _FPGA_PIO_setDirection(ctx, dir, mask);
}

//Get direction
// - Returns the current direction of masked pins to *dir
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_getDirection(FPGAPIOCtx_t* ctx, unsigned int* dir, unsigned int mask) {
    if (!dir) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (ctx->pioType != FPGA_PIO_DIRECTION_BIDIR) return ERR_NOSUPPORT;
    //Get direction
    return _FPGA_PIO_getDirection(ctx, dir, mask);
}

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_setOutput(FPGAPIOCtx_t* ctx, unsigned int port, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Configure output
    return _FPGA_PIO_setOutput(ctx, port, mask);
}

//Set output bits
// - If bit-set feature is supported, directly sets the masked bits
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_bitsetOutput(FPGAPIOCtx_t* ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!ctx->hasBitset) return ERR_NOSUPPORT;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Configure output
    return ctx->base[GPIO_OUT_SET] = mask;
}

//Clear output bits
// - If bit-set feature is supported, directly clears the masked bits
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_bitclearOutput(FPGAPIOCtx_t* ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!ctx->hasBitset) return ERR_NOSUPPORT;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Configure output
    return ctx->base[GPIO_OUT_CLEAR] = mask;
}

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_toggleOutput(FPGAPIOCtx_t* ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Toggle outputs
    return _FPGA_PIO_toggleOutput(ctx, mask);
}

//Get output value
// - Returns the current value of the masked output pins to *port
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_getOutput(FPGAPIOCtx_t* ctx, unsigned int* port, unsigned int mask) {
    if (!port) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Get output
    return _FPGA_PIO_getOutput(ctx, port, mask);
}

//Get input value
// - Returns the current value of the masked input pins to *in.
// - Only supported if pio type has FPGA_PIO_DIRECTION_IN capability.
HpsErr_t FPGA_PIO_getInput(FPGAPIOCtx_t* ctx, unsigned int* in, unsigned int mask) {
    if (!in) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_IN)) return ERR_NOSUPPORT;
    //Get input
    return _FPGA_PIO_getInput(ctx, in, mask);
}

//Set interrupt mask
// - Enable masked pins to generate an interrupt to the processor.
HpsErr_t FPGA_PIO_setInterruptEnable(FPGAPIOCtx_t* ctx, unsigned int flags, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!ctx->hasIrq) return ERR_NOSUPPORT;
    return _FPGA_PIO_setInterruptEnable(ctx, flags, mask);
}

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
// - The returned flags are unmasked so even pins for which interrupt
//   has not been enabled may return true in the flags.
HpsErr_t FPGA_PIO_getInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int* flags, unsigned int mask, bool autoClear) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Read the flags
    return _FPGA_PIO_getInterruptFlags(ctx, flags, mask, autoClear);
}

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
HpsErr_t FPGA_PIO_clearInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (!ctx->hasEdge) return ERR_NOSUPPORT;
    return _FPGA_PIO_clearInterruptFlags(ctx, mask);
}
