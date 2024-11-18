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

#ifndef HPS_GPIO_H_
#define HPS_GPIO_H_

#include "Util/macros.h"
#include "Util/driver_ctx.h"
#include "Util/bit_helpers.h"
#include "Util/driver_gpio.h"

//Modes for interrupts
typedef enum {
    // Flags
    GPIO_IRQ_POLR_FLAG    = _BV(0),
    GPIO_IRQ_EDGE_FLAG    = _BV(1),
    GPIO_IRQ_ENBL_FLAG    = _BV(2),
    // IRQ Configs
    GPIO_IRQ_DISABLED     = 0,
    GPIO_IRQ_LEVEL_LOW    = GPIO_IRQ_ENBL_FLAG                                          ,
    GPIO_IRQ_LEVEL_HIGH   = GPIO_IRQ_ENBL_FLAG |                      GPIO_IRQ_POLR_FLAG,
    GPIO_IRQ_EDGE_FALLING = GPIO_IRQ_ENBL_FLAG | GPIO_IRQ_EDGE_FLAG                     ,
    GPIO_IRQ_EDGE_RISING  = GPIO_IRQ_ENBL_FLAG | GPIO_IRQ_EDGE_FLAG | GPIO_IRQ_POLR_FLAG
} GPIOIRQPolarity;

// Driver context
typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile unsigned int* base;
    unsigned int initPort;
    unsigned int initDir;
    unsigned int polarity;
    GpioCtx_t gpio;
} HPSGPIOCtx_t;

//Initialise HPS GPIO Driver
// - Provide the base address of the GPIO controller
// - dir is the default direction for GPIO pins
// - port is the default output value for GPIO pins
// - polarity sets whether a given pin is inverted (bit mask).
HpsErr_t HPS_GPIO_initialise(void* base, unsigned int dir, unsigned int port, unsigned int polarity, HPSGPIOCtx_t** pCtx);

//Check if driver initialised
// - Returns true if driver previously initialised
bool HPS_GPIO_isInitialised(HPSGPIOCtx_t* ctx);

//Set direction
// - Setting a bit to 1 means output, while 0 means input.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setDirection(HPSGPIOCtx_t* ctx, unsigned int dir, unsigned int mask);

//Get direction
// - Returns the current direction of masked pins to *dir
HpsErr_t HPS_GPIO_getDirection(HPSGPIOCtx_t* ctx, unsigned int *dir, unsigned int mask);

//Set output value
// - Sets or clears the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setOutput(HPSGPIOCtx_t* ctx, unsigned int port, unsigned int mask);

//Toggle output value
// - Toggles the output value of masked pins.
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be toggled.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_toggleOutput(HPSGPIOCtx_t* ctx, unsigned int mask);

//Get output value
// - Returns the current value of the masked output pins to *port
HpsErr_t HPS_GPIO_getOutput(HPSGPIOCtx_t* ctx, unsigned int* port, unsigned int mask);

//Get input value
// - Returns the current value of the masked input pins to *in.
HpsErr_t HPS_GPIO_getInput(HPSGPIOCtx_t* ctx, unsigned int* in, unsigned int mask);

//Set interrupt config
// - Sets interrupt configuration for masked pins to config.
// - Only pins with their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setInterruptConfig(HPSGPIOCtx_t* ctx, GPIOIRQPolarity config, unsigned int mask);

//Get interrupt config
// - Returns the current interrupt configuration for the specified pin
HpsErr_t HPS_GPIO_getInterruptConfig(HPSGPIOCtx_t* ctx, unsigned int pin);

//Get interrupt flags
// - Returns flags indicating which pins have generated an interrupt
HpsErr_t HPS_GPIO_getInterruptFlags(HPSGPIOCtx_t* ctx, unsigned int* flags);

//Clear interrupt flags
// - Clear interrupt flags of pins with bits set in mask.
HpsErr_t HPS_GPIO_clearInterruptFlags(HPSGPIOCtx_t* ctx, unsigned int mask);

//Enable or disable debouncing of inputs
// - Will perform read-modify-write such that only pins with
//   their mask bit set will be changed.
// - e.g. with mask of 0x00010002, pins [1] and [16] will be changed.
HpsErr_t HPS_GPIO_setDebounce(HPSGPIOCtx_t* ctx, unsigned int bounce, unsigned int mask);


#endif /* HPS_GPIO_H_ */
