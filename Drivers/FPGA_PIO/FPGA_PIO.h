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

#ifndef FPGA_PIO_H_
#define FPGA_PIO_H_

#include "Util/ct_assert.h"
#include "Util/bit_helpers.h"
#include "Util/macros.h"
#include "Util/driver_ctx.h"
#include "Util/driver_gpio.h"

typedef enum {
    FPGA_PIO_DIRECTION_IN    = _BV(0),
    FPGA_PIO_DIRECTION_OUT   = _BV(1),
    FPGA_PIO_DIRECTION_BOTH  =          FPGA_PIO_DIRECTION_IN | FPGA_PIO_DIRECTION_OUT,
    FPGA_PIO_DIRECTION_BIDIR = _BV(2) | FPGA_PIO_DIRECTION_IN | FPGA_PIO_DIRECTION_OUT,
} FPGAPIODirectionType;
// Convert system header CMACROs to direction type
#define __FPGA_PIO_DIRECTION_001  FPGA_PIO_DIRECTION_BIDIR
#define __FPGA_PIO_DIRECTION_110  FPGA_PIO_DIRECTION_BOTH
#define __FPGA_PIO_DIRECTION_100  FPGA_PIO_DIRECTION_IN
#define __FPGA_PIO_DIRECTION_010  FPGA_PIO_DIRECTION_OUT
#define FPGA_PIO_DIRECTION_TYPE(in,out,tri) CONCAT(__FPGA_PIO_DIRECTION_,in,out,tri)

typedef enum {
    FPGA_PIO_IRQ_NONE = 0, // Must be 0
    FPGA_PIO_IRQ_LEVEL,
    FPGA_PIO_IRQ_EDGE
} FPGAPIOIrqType;
// Convert system header CMACRO to PIO irq type
#define FPGA_PIO_IRQ_TYPE(x) CONCAT(FPGA_PIO_IRQ_,x)

typedef enum {
    FPGA_PIO_EDGE_NONE    = 0, // Must be 0
    FPGA_PIO_EDGE_RISING  = _BV(0),
    FPGA_PIO_EDGE_FALLING = _BV(1),
    FPGA_PIO_EDGE_ANY     = FPGA_PIO_EDGE_RISING | FPGA_PIO_EDGE_FALLING
} FPGAPIOEdgeCaptureType;
// Convert system header CMACRO to PIO edge type
#define FPGA_PIO_EDGE_TYPE(x) CONCAT(FPGA_PIO_EDGE_,x)

typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile unsigned int* base;
    FPGAPIODirectionType pioType;
    bool splitData;
    bool hasBitset;
    bool hasEdge;
    bool hasIrq;
    bool usePortCache;
    unsigned int initPort;
    unsigned int initDir;
    unsigned int outPort;
    GpioCtx_t gpio;
} FPGAPIOCtx_t;

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
HpsErr_t FPGA_PIO_initialise(void* base, FPGAPIODirectionType pioType, bool splitData, bool hasBitset, bool hasEdge, bool hasIrq, unsigned int dir, unsigned int port, FPGAPIOCtx_t** pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_PIO_isInitialised(FPGAPIOCtx_t* ctx);

//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_setDirection(FPGAPIOCtx_t* ctx, unsigned int dir, unsigned int mask);

//Get direction
// - Returns the current direction of masked pins to *dir
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_getDirection(FPGAPIOCtx_t* ctx, unsigned int *dir, unsigned int mask);

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_setOutput(FPGAPIOCtx_t* ctx, unsigned int port, unsigned int mask);

//Set output bits
// - If bit-set feature is supported, directly sets the masked bits
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_bitsetOutput(FPGAPIOCtx_t* ctx, unsigned int mask);

//Clear output bits
// - If bit-set feature is supported, directly clears the masked bits
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_bitclearOutput(FPGAPIOCtx_t* ctx, unsigned int mask);

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_toggleOutput(FPGAPIOCtx_t* ctx, unsigned int mask);

//Get output value
// - Returns the current value of the masked output pins to *port
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
HpsErr_t FPGA_PIO_getOutput(FPGAPIOCtx_t* ctx, unsigned int* port, unsigned int mask);

//Get input value
// - Returns the current value of the masked input pins to *in.
// - Only supported if pio type has FPGA_PIO_DIRECTION_IN capability.
HpsErr_t FPGA_PIO_getInput(FPGAPIOCtx_t* ctx, unsigned int* in, unsigned int mask);

//Set interrupt mask
// - Configure whether masked pins to generate an interrupt to the processor.
// - Any masked pin with a 1 bit in the flags will generate IRQ.
HpsErr_t FPGA_PIO_setInterruptEnable(FPGAPIOCtx_t* ctx, unsigned int flags, unsigned int mask);

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
// - Interrupt is edge sensitive if hasEdge, otherwise leven sensitive
// - The returned flags are unmasked so even pins for which interrupt
//   has not been enabled may return true in the flags.
// - If autoClear true, then interrupt flags will be cleared on read.
//   - clearing is only possible if we have edge detector capability
HpsErr_t FPGA_PIO_getInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int* flags, unsigned int mask, bool autoClear);

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
// - Only possible if has edge detector
HpsErr_t FPGA_PIO_clearInterruptFlags(FPGAPIOCtx_t* ctx, unsigned int mask);

#endif /* FPGA_PIO_H_ */

