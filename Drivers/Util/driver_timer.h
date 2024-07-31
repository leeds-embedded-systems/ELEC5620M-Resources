/* Generic Timer Driver Interface
 * ------------------------------
 *
 * Provides a common interface for different Timer
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
 * 09/03/2024 | Creation of driver.
 */

#ifndef DRIVER_TIMER_H_
#define DRIVER_TIMER_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

typedef enum {
    TIMER_MODE_EVENT,     // Free running. Top/load value MUST be set to UINT32_MAX (0xFFFFFFFF) for event timing
    TIMER_MODE_FREERUN,   // Free running, counts from load value to zero, then repeats
    TIMER_MODE_ONESHOT    // Counts from load value to zero then stops
} TimerMode;

// IO Function Templates
typedef HpsErr_t (*TimerEnableFunc_t    )(void* ctx, unsigned int fraction);
typedef HpsErr_t (*TimerDisableFunc_t   )(void* ctx);
typedef HpsErr_t (*TimerGetTimeFunc_t   )(void* ctx, unsigned int* time);
typedef HpsErr_t (*TimerGetRateFunc_t   )(void* ctx, unsigned int prescalar, unsigned int* rate);
typedef HpsErr_t (*TimerGetModeFunc_t   )(void* ctx, TimerMode* mode);
typedef HpsErr_t (*TimerOverflowedFunc_t)(void* ctx, bool autoClear);
typedef HpsErr_t (*TimerConfigureFunc_t )(void* ctx, TimerMode mode, unsigned int prescalar, unsigned int loadValue);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Driver Function Pointers
    TimerEnableFunc_t     enable;
    TimerDisableFunc_t    disable;
    TimerGetTimeFunc_t    getLoad;
    TimerGetTimeFunc_t    getTime;
    TimerGetRateFunc_t    getRate;
    TimerGetModeFunc_t    getMode;
    TimerOverflowedFunc_t checkOverflow;    
    TimerConfigureFunc_t  configure;    
} TimerCtx_t, *PTimerCtx_t;

// Check if driver initialised
static inline bool Timer_isInitialised(PTimerCtx_t timer) {
    if (!timer) return false;
    return DriverContextCheckInit(timer->ctx);
}

// Get the rounded timer clock rate
//  - Returns via *clockRate the current timer clock rate based on configured prescaler settings
//  - If prescaler is UINT32_MAX, will return based on previously configured.
//    prescaler settings, otherwise will calculate for specified value
static inline HpsErr_t Timer_getRate(PTimerCtx_t timer, unsigned int prescaler, unsigned int* rate) {
    if (!timer || !rate) return ERR_NULLPTR;
    if (!timer->getRate) return ERR_NOSUPPORT;
    return timer->getRate(timer->ctx,prescaler,rate);
}

// Get the current mode of the timer
static inline HpsErr_t Timer_getMode(PTimerCtx_t timer, TimerMode* mode) {
    if (!timer || !mode) return ERR_NULLPTR;
    if (!timer->getMode) return ERR_NOSUPPORT;
    return timer->getMode(timer->ctx,mode);
}

// Get the top/initial/load value of the timer
static inline HpsErr_t Timer_getLoad(PTimerCtx_t timer, unsigned int* loadTime) {
    if (!timer || !loadTime) return ERR_NULLPTR;
    if (!timer->getLoad) return ERR_NOSUPPORT;
    return timer->getLoad(timer->ctx,loadTime);
}

// Get current timer value
static inline HpsErr_t Timer_getTime(PTimerCtx_t timer, unsigned int* curTime) {
    if (!timer || !curTime) return ERR_NULLPTR;
    if (!timer->getTime) return ERR_NOSUPPORT;
    return timer->getTime(timer->ctx,curTime);
}

// Configure timer
//  - For one-shot mode, will disable the timer. Call NIOS_Timer_enable() to start one-shot.
//  - For other modes, timer will remain in current state (running or stopped)
static inline HpsErr_t Timer_configure(PTimerCtx_t timer, TimerMode mode, unsigned int prescalar, unsigned int loadValue) {
    if (!timer) return ERR_NULLPTR;
    if (!timer->configure) return ERR_NOSUPPORT;
    return timer->configure(timer->ctx, mode, prescalar, loadValue);
}

// Enable timer
//  - If fraction is non-zero, will set the load value
//    to the configured value divided by this fraction
//    for this run. Allows quickly scaling the timeout
//    between runs.
static inline HpsErr_t Timer_enable(PTimerCtx_t timer, unsigned int fraction) {
    if (!timer) return ERR_NULLPTR;
    if (!timer->enable) return ERR_NOSUPPORT;
    return timer->enable(timer->ctx,fraction);
}

// Disable timer
static inline HpsErr_t Timer_disable(PTimerCtx_t timer) {
    if (!timer) return ERR_NULLPTR;
    if (!timer->disable) return ERR_NOSUPPORT;
    return timer->disable(timer->ctx);
}

// Check and clear overflow/interrupt flag
//  - Returns 1 if timer overflow flag was set
//  - Returns 0 if interrupt flag was clear.
//  - If autoClear is true, will clear the interrupt flag.
static inline HpsErr_t Timer_checkOverflow(PTimerCtx_t timer, bool autoClear) {
    if (!timer) return ERR_NULLPTR;
    if (!timer->checkOverflow) return ERR_NOSUPPORT;
    return timer->checkOverflow(timer->ctx, autoClear);
}

#endif /* DRIVER_TIMER_H_ */
