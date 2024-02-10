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

typedef struct {
    uint32_t data;
    union {
        uint32_t direction;
        uint32_t read;
    };
    uint32_t interruptmask;
    uint32_t edgecapture;
} FPGAPIOCSR_t, *PFPGAPIOCSR_t;
ct_assert(FPGAPIOCSR_t, (4 * sizeof(uint32_t)));

typedef struct {
    FPGAPIOCSR_t base;
    uint32_t  outset;
    uint32_t  outclear;
} FPGAPIOExtCSR_t, *PFPGAPIOExtCSR_t;
ct_assert(FPGAPIOExtCSR_t, 6 * sizeof(uint32_t));

typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile PFPGAPIOExtCSR_t csr;
    FPGAPIODirectionType pioType;
    bool splitData;
    bool hasBitset;
    bool hasEdge;
    bool hasIrq;
    unsigned int initPort;
    unsigned int initDir;
    GpioCtx_t gpio;
} FPGAPIOCtx_t, *PFPGAPIOCtx_t;

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
HpsErr_t FPGA_PIO_initialise(void* base, FPGAPIODirectionType pioType, bool splitData, bool hasBitset, bool hasEdge, bool hasIrq, unsigned int dir, unsigned int port, PFPGAPIOCtx_t* pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_PIO_isInitialised(PFPGAPIOCtx_t ctx);

//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_setDirection(PFPGAPIOCtx_t ctx, unsigned int dir, unsigned int mask);

//Get direction
// - Returns the current direction of masked pins to *dir
// - Only supported if pio type is FPGA_PIO_DIRECTION_BIDIR
HpsErr_t FPGA_PIO_getDirection(PFPGAPIOCtx_t ctx, unsigned int *dir, unsigned int mask);

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Only supported if pio type has FPGA_PIO_DIRECTION_OUT capability.
// - Masking not supported if has FPGA_PIO_DIRECTION_IN capability, unless hasBitset is true.
HpsErr_t FPGA_PIO_setOutput(PFPGAPIOCtx_t ctx, unsigned int port, unsigned int mask);

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
// - Not supported if pio type has FPGA_PIO_DIRECTION_IN capability.
HpsErr_t FPGA_PIO_toggleOutput(PFPGAPIOCtx_t ctx, unsigned int mask);

//Get output value
// - Returns the current value of the masked output pins to *port
// - Not supported if pio type has FPGA_PIO_DIRECTION_IN capability.
HpsErr_t FPGA_PIO_getOutput(PFPGAPIOCtx_t ctx, unsigned int* port, unsigned int mask);

//Get input value
// - Returns the current value of the masked input pins to *in.
// - Only supported if pio type has FPGA_PIO_DIRECTION_IN capability.
HpsErr_t FPGA_PIO_getInput(PFPGAPIOCtx_t ctx, unsigned int* in, unsigned int mask);

//Set interrupt mask
// - Configure whether masked pins to generate an interrupt to the processor.
// - Any masked pin with a 1 bit in the flags will generate IRQ.
HpsErr_t FPGA_PIO_setInterruptEnable(PFPGAPIOCtx_t ctx, unsigned int flags, unsigned int mask);

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
// - Interrupt is edge sensitive if hasEdge, otherwise leven sensitive
// - The returned flags are unmasked so even pins for which interrupt
//   has not been enabled may return true in the flags.
// - If autoClear true, then interrupt flags will be cleared on read.
//   - clearing is only possible if we have edge detector capability
HpsErr_t FPGA_PIO_getInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int* flags, unsigned int mask, bool autoClear);

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
// - Only possible if has edge detector
HpsErr_t FPGA_PIO_clearInterruptFlags(PFPGAPIOCtx_t ctx, unsigned int mask);

#endif /* FPGA_PIO_H_ */

