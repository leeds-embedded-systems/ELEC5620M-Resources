/* Generic GPIO Driver Interface
 * -----------------------------
 *
 * Provides a common interface for different GPIO
 * drivers to allow them to be used as a generic
 * handler.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 30/12/2023 | Creation of driver.
 */

#ifndef DRIVER_GPIO_H_
#define DRIVER_GPIO_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

// IO Function Templates
typedef HpsErr_t (*GpioWriteFunc_t )(void* ctx, unsigned int out, unsigned int mask);
typedef HpsErr_t (*GpioToggleFunc_t)(void* ctx, unsigned int mask);
typedef HpsErr_t (*GpioReadFunc_t  )(void* ctx, unsigned int* in, unsigned int mask);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Driver Function Pointers
    GpioWriteFunc_t  setDirection;
    GpioReadFunc_t   getDirection;
    GpioWriteFunc_t  setOutput;
    GpioReadFunc_t   getOutput;
    GpioToggleFunc_t toggleOutput;
    GpioReadFunc_t   getInput;
} GpioCtx_t, *PGpioCtx_t;

// Check if driver initialised
inline HpsErr_t GPIO_isInitialised(PGpioCtx_t gpio) {
    if (!gpio) return ERR_NULLPTR;
    return DriverContextCheckInit(gpio->ctx);
}

inline HpsErr_t GPIO_setDirection(PGpioCtx_t gpio, unsigned int dir, unsigned int mask) {
    if (!gpio) return ERR_NULLPTR;
    if (!gpio->setDirection) return ERR_NOSUPPORT;
    return gpio->setDirection(gpio->ctx,dir,mask);
}

inline HpsErr_t GPIO_getDirection(PGpioCtx_t gpio, unsigned int* dir, unsigned int mask) {
    if (!gpio || !dir) return ERR_NULLPTR;
    if (!gpio->getDirection) return ERR_NOSUPPORT;
    return gpio->getDirection(gpio->ctx,dir,mask);
}

inline HpsErr_t GPIO_setOutput(PGpioCtx_t gpio, unsigned int port, unsigned int mask) {
    if (!gpio) return ERR_NULLPTR;
    if (!gpio->setOutput) return ERR_NOSUPPORT;
    return gpio->setOutput(gpio->ctx,port,mask);
}

inline HpsErr_t GPIO_getOutput(PGpioCtx_t gpio, unsigned int* port, unsigned int mask) {
    if (!gpio || !port) return ERR_NULLPTR;
    if (!gpio->getOutput) return ERR_NOSUPPORT;
    return gpio->getOutput(gpio->ctx,port,mask);
}

inline HpsErr_t GPIO_toggleOutput(PGpioCtx_t gpio, unsigned int mask) {
    if (!gpio) return ERR_NULLPTR;
    if (!gpio->toggleOutput) return ERR_NOSUPPORT;
    return gpio->toggleOutput(gpio->ctx,mask);
}

inline HpsErr_t GPIO_getInput(PGpioCtx_t gpio, unsigned int* in, unsigned int mask) {
    if (!gpio || !in) return ERR_NULLPTR;
    if (!gpio->getInput) return ERR_NOSUPPORT;
    return gpio->getInput(gpio->ctx,in,mask);
}


#endif /* DRIVER_GPIO_H_ */
