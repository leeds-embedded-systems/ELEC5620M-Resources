/*
 * Cyclone V & Arria 10 HPS GPIO Controller
 * ---------------------------------------------
 *
 * Driver for controlling HPS GPIO pins.
 *
 * For Cyclone V, The GPIO banks are 29bit wide, so in
 * the pin bit masks, bits 0-28 only are used.
 *
 * For Arria 10, The GPIO banks are 24bit wide, so in
 * the pin bit masks, bits 0-23 only are used.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 30/12/2023 | Creation of driver
 *
 */

#include "HPS_GPIO.h"

#include "HPS_IRQ/HPS_IRQ.h"
#include "Util/bit_helpers.h"

#define GPIO_OUTPUT     ( 0x0 / sizeof(unsigned int))
#define GPIO_DIRECTION  ( 0x4 / sizeof(unsigned int))
#define GPIO_INPUT      (0x50 / sizeof(unsigned int))

#define GPIO_DEBOUNCE   (0x48 / sizeof(unsigned int))

#define GPIO_INTR_EN    (0x30 / sizeof(unsigned int))
#define GPIO_INTR_MASK  (0x34 / sizeof(unsigned int))
#define GPIO_INTR_LEVEL (0x38 / sizeof(unsigned int))
#define GPIO_INTR_POL   (0x3C / sizeof(unsigned int))
#define GPIO_INTR_FLAGS (0x40 / sizeof(unsigned int))
#define GPIO_INTR_CLEAR (0x4C / sizeof(unsigned int))

/*
 * Internal Functions
 */

//Set interrupt config
// - Sets interrupt configuration for masked pins to config.
// - Only pins with their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
static unsigned int _HPS_GPIO_setConfigFlag(unsigned int cur, unsigned int mask, unsigned int set) {
    if (set) {
        cur = (cur |  mask);
    }
    else {
        cur = (cur & ~mask);
    }
    return cur;
}

static void _HPS_GPIO_cleanup(PHPSGPIOCtx_t ctx) {
    //Disable interrupts and set default output state
    if (ctx->base) {
        ctx->base[GPIO_INTR_EN  ] = 0x0;
        ctx->base[GPIO_DIRECTION] = ctx->initDir;
        ctx->base[GPIO_OUTPUT   ] = ctx->initPort;
    }
}

/*
 * User Facing APIs
 */

//Initialise HPS GPIO Driver
// - Provide the base address of the GPIO controller
// - dir is the default direction for GPIO pins
// - port is the default output value for GPIO pins
// - polarity sets whether a given pin is inverted (bit mask).
HpsErr_t HPS_GPIO_initialise(void* base, unsigned int dir, unsigned int port, unsigned int polarity, PHPSGPIOCtx_t* pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_HPS_GPIO_cleanup);
    if (IS_ERROR(status)) return status;
    //Save base address pointers
    PHPSGPIOCtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->initDir = dir;
    ctx->initPort = port;
    ctx->polarity = polarity;
    //Configure initial GPIO settings
    ctx->base[GPIO_INTR_EN  ] = 0x0;  // No interrupts
    ctx->base[GPIO_DIRECTION] = 0x0;  // Switch all to inputs temporarily
    ctx->base[GPIO_OUTPUT   ] = port ^ polarity; // Configure default output settings
    ctx->base[GPIO_DIRECTION] = dir;  // Configure default direction settings
    //Populate the GPIO structure
    ctx->gpio.ctx = ctx;
    ctx->gpio.getDirection = (GpioReadFunc_t)&HPS_GPIO_getDirection;
    ctx->gpio.setDirection = (GpioWriteFunc_t)&HPS_GPIO_setDirection;
    ctx->gpio.getOutput = (GpioReadFunc_t)&HPS_GPIO_getOutput;
    ctx->gpio.setOutput = (GpioWriteFunc_t)&HPS_GPIO_setOutput;
    ctx->gpio.toggleOutput = (GpioToggleFunc_t)&HPS_GPIO_toggleOutput;
    ctx->gpio.getInput = (GpioReadFunc_t)&HPS_GPIO_getInput;
    //And initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

//Check if driver initialised
// - Returns true if driver previously initialised
bool HPS_GPIO_isInitialised(PHPSGPIOCtx_t ctx) {
    return DriverContextCheckInit(ctx);
}

//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setDirection(PHPSGPIOCtx_t ctx, unsigned int dir, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //R-M-W direction
    unsigned int curVal = ctx->base[GPIO_DIRECTION];
    ctx->base[GPIO_DIRECTION] = ((dir & mask) | (curVal & ~mask));
    return ERR_SUCCESS;
}

//Get direction
// - Returns the current direction of masked pins to *dir
HpsErr_t HPS_GPIO_getDirection(PHPSGPIOCtx_t ctx, unsigned int* dir, unsigned int mask) {
    if (!dir) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Get direction
    *dir = ctx->base[GPIO_DIRECTION];
    return ERR_SUCCESS;
}

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setOutput(PHPSGPIOCtx_t ctx, unsigned int port, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //R-M-W output
    port ^= ctx->polarity;
    unsigned int curVal = ctx->base[GPIO_OUTPUT];
    ctx->base[GPIO_OUTPUT] = ((port & mask) | (curVal & ~mask));
    return ERR_SUCCESS;
}

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_toggleOutput(PHPSGPIOCtx_t ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Toggle outputs
    ctx->base[GPIO_OUTPUT] = (ctx->base[GPIO_OUTPUT] ^ mask);
    return ERR_SUCCESS;
}

//Get output value
// - Returns the current value of the masked output pins to *port
HpsErr_t HPS_GPIO_getOutput(PHPSGPIOCtx_t ctx, unsigned int* port, unsigned int mask) {
    if (!port) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Get output
    unsigned int _port = ctx->base[GPIO_OUTPUT];
    *port = _port ^ ctx->polarity;
    return ERR_SUCCESS;
}

//Get input value
// - Returns the current value of the masked input pins to *in.
HpsErr_t HPS_GPIO_getInput(PHPSGPIOCtx_t ctx,  unsigned int* in, unsigned int mask) {
    if (!in) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Get input
    unsigned int _in = ctx->base[GPIO_INPUT];
    *in = _in ^ ctx->polarity;
    return ERR_SUCCESS;
}

HpsErr_t HPS_GPIO_setInterruptConfig(PHPSGPIOCtx_t ctx, GPIOIRQPolarity config, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Before changing anything we need to mask global interrupts temporarily
    HpsErr_t irqStatus = HPS_IRQ_globalEnable(false);
    //Disable individual interrupts while we make changes
    unsigned int enBits = ctx->base[GPIO_INTR_EN];
    ctx->base[GPIO_INTR_EN] = 0;
    //Update type flags
    ctx->base[GPIO_INTR_POL  ] = _HPS_GPIO_setConfigFlag(ctx->base[GPIO_INTR_POL  ], mask, config & GPIO_IRQ_POLR_FLAG);
    ctx->base[GPIO_INTR_LEVEL] = _HPS_GPIO_setConfigFlag(ctx->base[GPIO_INTR_LEVEL], mask, config & GPIO_IRQ_EDGE_FLAG);
    //Configure which are enabled
    ctx->base[GPIO_INTR_EN] = _HPS_GPIO_setConfigFlag(enBits, mask, config & GPIO_IRQ_ENBL_FLAG);
    //Re-enable interrupts if they were enabled
    HPS_IRQ_globalEnable(IS_SUCCESS(irqStatus));
    return ERR_SUCCESS;
}

//Get interrupt config
// - Returns the current interrupt configuration for the specified pin
HpsErrExt_t HPS_GPIO_getInterruptConfig(PHPSGPIOCtx_t ctx, unsigned int pin) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Read the various config registers
    unsigned int mask = _BV(pin);
    if (ctx->base[GPIO_INTR_EN] & mask) {
        HpsErrExt_t config = GPIO_IRQ_ENBL_FLAG;
        if (ctx->base[GPIO_INTR_LEVEL] & mask) config |= GPIO_IRQ_EDGE_FLAG;
        if (ctx->base[GPIO_INTR_POL  ] & mask) config |= GPIO_IRQ_POLR_FLAG;
        return config;
    }
    return GPIO_IRQ_DISABLED;
}

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
HpsErr_t HPS_GPIO_getInterruptFlags(PHPSGPIOCtx_t ctx,  unsigned int* flags) {
    if (!flags) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Read the flags
    *flags = ctx->base[GPIO_INTR_FLAGS];
    return ERR_SUCCESS;
}

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
HpsErr_t HPS_GPIO_clearInterruptFlags(PHPSGPIOCtx_t ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Clear the flags
    ctx->base[GPIO_INTR_CLEAR] = mask;
    return ERR_SUCCESS;
}

//Enable or disable debouncing of inputs
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setDebounce(PHPSGPIOCtx_t ctx, unsigned int bounce, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //R-M-W debounce
    unsigned int curVal = ctx->base[GPIO_DEBOUNCE];
    ctx->base[GPIO_DEBOUNCE] = ((bounce & mask) | (curVal & ~mask));
    return ERR_SUCCESS;
}

