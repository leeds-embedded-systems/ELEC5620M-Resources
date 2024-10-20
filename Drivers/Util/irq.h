/*
 * Generic Interrupt Routines
 * -------------------------
 *
 * Maps APIs specific to different CPU types to a
 * common set of APIs.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 12/10/2024 | Add support for Nios2
 * 09/04/2024 | Creation of header
 * 
 */

#ifndef UTIL_IRQ_H
#define UTIL_IRQ_H

#include <stdbool.h>
#include <stdint.h>

#include "Util/error.h"

#if defined(__arm__)
// ARM uses HPS IRQ Header
#include "HPS_IRQ/HPS_IRQ.h"
#elif defined(__NIOS2__)
// Nios IRQ Header
#include "NIOS_IRQ/NIOS_IRQ.h"
#endif

//Globally enable or disable interrupts
// - If trying to enable:
//    - Requires that driver has been initialised
//    - Returns ERR_SUCCESS if interrupts have been enabled
// - If trying to disable
//    - Returns ERR_SUCCESS if interrupts have been disabled
//    - Returns ERR_SKIPPED if interrupts were already disabled
// - This function can be used to temporarily disable interrupts
//   if for example you are changing the IRQ flags in a peripheral.
static inline HpsErr_t IRQ_globalEnable(bool enable) {
#if defined(__arm__)
    return HPS_IRQ_globalEnable(enable);
#elif defined(__NIOS2__)
    return NIOS_IRQ_globalEnable(enable);
#else
    return ERR_SUCCESS;
#endif
}

#endif /* UTIL_IRQ_H */
