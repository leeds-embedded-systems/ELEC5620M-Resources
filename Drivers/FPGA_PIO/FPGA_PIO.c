/*
 * Nios PIO Controller Driver
 * ---------------------------
 *
 * Driver for writing to generic PIO controller
 * core (e.g. avmm_pio_hw).
 *
 * The PIO controller has a single data registers
 * shared between input and output. If there is an
 * input register, then we cannot read the state of
 * the output register. This means that R-M-W of
 * the output is only possible if the PIO is either
 * output-only, or has the optional bit set/clear
 * capability.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 30/12/2023 | Creation of driver.
 *
 */

#include "FPGA_PIO.h"

#include "HPS_IRQ/HPS_IRQ.h"
#include "Util/bit_helpers.h"

/*
 * Internal Functions
 */

static void _FPGA_PIO_cleanup(PFPGAPIOCtx_t ctx) {
    //Disable interrupts and reset default output states
    if (ctx->csr) {
        ctx->csr->base.interruptmask = 0x0;
        if (ctx->pioType & FPGA_PIO_DIRECTION_OUT) {
            ctx->csr->base.data = ctx->initPort;
        }
        if (ctx->pioType == FPGA_PIO_DIRECTION_BIDIR) {
            ctx->csr->base.direction = ctx->initDir;
        }
    }
}

static HpsErr_t _FPGA_PIO_setDirection(PFPGAPIOCtx_t ctx, unsigned int dir, unsigned int mask) {
    //R-M-W direction
    unsigned int curVal = ctx->csr->base.direction;
    ctx->csr->base.direction = ((dir & mask) | (curVal & ~mask));
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getDirection(PFPGAPIOCtx_t ctx, unsigned int* dir, unsigned int mask) {
    *dir = ctx->csr->base.direction & mask;
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_setOutput(PFPGAPIOCtx_t ctx, unsigned int port, unsigned int mask){
    //Masking required
    if (ctx->gpio.getOutput) {
        // Can read output register, so do R-M-W
        unsigned int curVal = ctx->csr->base.data;
        ctx->csr->base.data = ((port & mask) | (curVal & ~mask));
    } else {
        //Configure output
        if (mask == UINT32_MAX) {
            //No masking required, just write port
            ctx->csr->base.data = port;
        } else {
            // Can't read register. Can only do R-M-W if we have bitset support
            if (!ctx->hasBitset) return ERR_NOSUPPORT;
            // Clear zero bits and set one bits.
            ctx->csr->outclear = (~port & mask);
            ctx->csr->outset = (port & mask);
        }
    }
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_toggleOutput(PFPGAPIOCtx_t ctx, unsigned int mask) {
    //Toggle outputs
    ctx->csr->base.data = (ctx->csr->base.data ^ mask);
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getOutput(PFPGAPIOCtx_t ctx, unsigned int* port, unsigned int mask) {
    //Get output
    *port = ctx->csr->base.data & mask;
    return ERR_SUCCESS;
}


static HpsErr_t _FPGA_PIO_getInput(PFPGAPIOCtx_t ctx, unsigned int* in, unsigned int mask) {
    //Get input
    if (ctx->splitData) {
        *in = ctx->csr->base.read & mask;
    } else {
        *in = ctx->csr->base.data & mask;
    }
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_getInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int* flags, unsigned int mask, bool autoClear) {
    //Read the flags
    unsigned int edgeCapt = ctx->csr->base.edgecapture & mask;
    //Clear the flags if requested
    if (ctx->hasEdge && edgeCapt && autoClear) {
        ctx->csr->base.edgecapture = edgeCapt;
    }
    //Return the flags if there is somewhere to return them to.
    if (flags) *flags = edgeCapt;
    return ERR_SUCCESS;
}

static HpsErr_t _FPGA_PIO_clearInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int mask) {
    ctx->csr->base.edgecapture = mask;
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
HpsErr_t FPGA_PIO_initialise(void* base, FPGAPIODirectionType pioType, bool splitData, bool hasBitset, bool hasEdge, bool hasIrq, unsigned int dir, unsigned int port, PFPGAPIOCtx_t* pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(FPGAPIOCSR_t))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_FPGA_PIO_cleanup);
    if (IS_ERROR(status)) return status;
    //Save base address pointers
    PFPGAPIOCtx_t ctx = *pCtx;
    ctx->csr = (PFPGAPIOExtCSR_t)base;
    ctx->pioType = pioType;
    ctx->splitData = splitData;
    ctx->hasBitset = hasBitset;
    ctx->hasEdge = hasEdge;
    ctx->hasIrq = hasIrq;
    ctx->initDir = dir;
    ctx->initPort = port;
    //Ensure no interrupts are enabled.
    ctx->csr->base.interruptmask = 0x0;
    ctx->csr->base.edgecapture = UINT32_MAX;
    //Populate the GPIO structure
    ctx->gpio.ctx = ctx;
    if (pioType == FPGA_PIO_DIRECTION_BIDIR) {
        ctx->gpio.getDirection = (GpioReadFunc_t)&_FPGA_PIO_getDirection;
        ctx->gpio.setDirection = (GpioWriteFunc_t)&_FPGA_PIO_setDirection;
        ctx->csr->base.direction = dir;
    }
    if (pioType & FPGA_PIO_DIRECTION_OUT) {
        ctx->gpio.setOutput = (GpioWriteFunc_t)&_FPGA_PIO_setOutput;
        ctx->csr->base.data = port;
    }
    if (pioType & FPGA_PIO_DIRECTION_IN) {
        ctx->gpio.getInput = (GpioReadFunc_t)&_FPGA_PIO_getInput;
        if (splitData) {
            // When we have an input, can only read output value if in split data mode.
            ctx->gpio.getOutput = (GpioReadFunc_t)&_FPGA_PIO_getOutput;
            ctx->gpio.toggleOutput = (GpioToggleFunc_t)&_FPGA_PIO_toggleOutput;
        }
    } else {
        // Can get/toggle output if we don't have input
        ctx->gpio.getOutput = (GpioReadFunc_t)&_FPGA_PIO_getOutput;
        ctx->gpio.toggleOutput = (GpioToggleFunc_t)&_FPGA_PIO_toggleOutput;
    }
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_PIO_isInitialised(PFPGAPIOCtx_t ctx) {
    return DriverContextCheckInit(ctx);
}


//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t FPGA_PIO_setDirection(PFPGAPIOCtx_t ctx, unsigned int dir, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (ctx->pioType != FPGA_PIO_DIRECTION_BIDIR) return ERR_NOSUPPORT;
    //R-M-W direction
    return _FPGA_PIO_setDirection(ctx, dir, mask);
}

//Get direction
// - Returns the current direction of masked pins to *dir
HpsErr_t FPGA_PIO_getDirection(PFPGAPIOCtx_t ctx, unsigned int* dir, unsigned int mask) {
    if (!dir) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (ctx->pioType != FPGA_PIO_DIRECTION_BIDIR) return ERR_NOSUPPORT;
    //Get direction
    return _FPGA_PIO_getDirection(ctx, dir, mask);
}

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t FPGA_PIO_setOutput(PFPGAPIOCtx_t ctx, unsigned int port, unsigned int mask){
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!(ctx->pioType & FPGA_PIO_DIRECTION_OUT)) return ERR_NOSUPPORT;
    //Configure output
    return _FPGA_PIO_setOutput(ctx, port, mask);
}

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t FPGA_PIO_toggleOutput(PFPGAPIOCtx_t ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!ctx->gpio.toggleOutput) return ERR_NOSUPPORT;
    //Toggle outputs
    return _FPGA_PIO_toggleOutput(ctx, mask);
}

//Get output value
// - Returns the current value of the masked output pins to *port
HpsErr_t FPGA_PIO_getOutput(PFPGAPIOCtx_t ctx, unsigned int* port, unsigned int mask) {
    if (!port) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!ctx->gpio.getOutput) return ERR_NOSUPPORT;
    //Get output
    return _FPGA_PIO_getOutput(ctx, port, mask);
}

//Get input value
// - Returns the current value of the masked input pins to *in.
HpsErr_t FPGA_PIO_getInput(PFPGAPIOCtx_t ctx, unsigned int* in, unsigned int mask) {
    if (!in) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!ctx->gpio.getInput) return ERR_NOSUPPORT;
    //Get input
    return _FPGA_PIO_getInput(ctx, in, mask);
}

//Set interrupt mask
// - Enable masked pins to generate an interrupt to the processor.
static HpsErr_t _FPGA_PIO_setInterruptEnable(PFPGAPIOCtx_t ctx, unsigned int flags, unsigned int mask) {
    //Before changing anything we need to mask global interrupts temporarily
    HpsErr_t irqStatus = HPS_IRQ_globalEnable(false);
    //Modify the enable flags
    unsigned int enBits = ctx->csr->base.interruptmask;
    ctx->csr->base.interruptmask = ((flags & mask) | (enBits & ~mask));
    //Re-enable interrupts if they were enabled
    HPS_IRQ_globalEnable(IS_SUCCESS(irqStatus));
    return ERR_SUCCESS;
}
HpsErr_t FPGA_PIO_setInterruptEnable(PFPGAPIOCtx_t ctx, unsigned int flags, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!ctx->hasIrq) return ERR_NOSUPPORT;
    return _FPGA_PIO_setInterruptEnable(ctx, flags, mask);
}

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
// - The returned flags are unmasked so even pins for which interrupt
//   has not been enabled may return true in the flags.
HpsErr_t FPGA_PIO_getInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int* flags, unsigned int mask, bool autoClear) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Read the flags
    return _FPGA_PIO_getInterruptFlags(ctx, flags, mask, autoClear);
}

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
HpsErr_t FPGA_PIO_clearInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    if (!ctx->hasEdge) return ERR_NOSUPPORT;
    return _FPGA_PIO_clearInterruptFlags(ctx, mask);
}
