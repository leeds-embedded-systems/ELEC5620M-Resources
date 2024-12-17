/*
 * FPGA Avalon-MM Reset Controller
 * -------------------------------
 *
 * APIs for controlling the Av-MM Reset PIO core.
 * 
 * Provides a simple API to assert or release reset.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 13/12/2024 | Creation of driver.
 *
 */

#ifndef FPGA_AVMMRESET_H_
#define FPGA_AVMMRESET_H_

#include "Util/driver_ctx.h"


typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile unsigned int* base;
    bool defaultAssert;
} FPGAAvMMResetCtx_t;

// Initialise Av-MM Reset Driver
//  - base is a pointer to the reset control CSR
//  - defaultAssert is the state restored on cleanup.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t FPGA_Reset_initialise(void* base, bool defaultAssert, FPGAAvMMResetCtx_t** pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool FPGA_Reset_isInitialised(FPGAAvMMResetCtx_t* ctx);

//Set reset state
// - If assertReset is true, will apply reset. Otherwise releases reset.
HpsErr_t FPGA_Reset_configureReset(FPGAAvMMResetCtx_t* ctx, bool assertReset);

//Check if reset is asserted
// - Returns ERR_TRUE if in reset, else ERR_FALSE.
HpsErr_t FPGA_Reset_isAsserted(FPGAAvMMResetCtx_t* ctx);


#endif /* FPGA_AVMMRESET_H_ */

