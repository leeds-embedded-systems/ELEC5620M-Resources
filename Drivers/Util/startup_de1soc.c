/*
 * startup_de1soc.c
 *
 *  Created on: 14 Nov 2024
 *      Author: eentca
 */

#include <stdint.h>
#include "Util/macros.h"

#include "DE1SoC_Addresses/DE1SoC_Addresses.h"

// Load data region
#define __LOAD_BASE SCATTER_LOAD_BASE(LOADREGION)
#define __LOAD_LEN  SCATTER_LOAD_LENGTH(LOADREGION)

extern unsigned int __LOAD_BASE;
extern unsigned int __LOAD_LEN;

#define LOAD_BASE (uintptr_t)&__LOAD_BASE
#define LOAD_LEN  (uintptr_t)&__LOAD_LEN

// Board specific initialisation routines used during startup
void __init_board() {
    // For DE1-SoC, backup OCRAM to Bootloader Cache if we are using
    // the on-chip RAM. This is to allow loading programs with the
    // debugger whilst allowing watchdog reset.
    if (LOAD_BASE == (uintptr_t)LSC_BASE_PROC_OCRAM) {
        volatile unsigned int* bootldrCache = (unsigned int*)LSC_BASE_BOOTLDR_CACHE;
        volatile unsigned int* hpsOcram     = (unsigned int*)LSC_BASE_PROC_OCRAM;
        uintptr_t memSize = CEIL_DIV(LOAD_LEN, sizeof(unsigned int));
        while (memSize--) {
            *bootldrCache++ = *hpsOcram++;
        }
    }
}


