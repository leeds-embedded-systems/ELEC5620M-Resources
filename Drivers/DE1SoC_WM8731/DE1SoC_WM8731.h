/* 
 * WM8731 Audio Controller Driver
 * ------------------------------
 * Description: 
 * Driver for the WM8731 Audio Controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-------------------------------
 * 31/01/2024 | Update to new driver contexts
 * 20/10/2017 | Change to include status codes
 * 20/09/2017 | Creation of driver
 *
 */

#ifndef DE1SoC_WM8731_H_
#define DE1SoC_WM8731_H_

//Include required header files
#include <stdint.h>
#include "Util/driver_ctx.h"
#include "HPS_I2C/HPS_I2C.h"

//FIFO Space Offsets
#define WM8731_RARC 0
#define WM8731_RALC 1
#define WM8731_WSRC 2
#define WM8731_WSLC 3

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    PHPSI2CCtx_t i2c; // I2C peripheral used by Audio Codec
    unsigned int i2cAddr;
} WM8731Ctx_t, *PWM8731Ctx_t;

//Initialise Audio Codec
// - base_address is memory-mapped address of audio controller
// - returns 0 if successful
HpsErr_t WM8731_initialise( void* base, PHPSI2CCtx_t i2c, PWM8731Ctx_t* pCtx );

//Check if driver initialised
// - Returns true if driver previously initialised
// - WM8731_initialise() must be called if false.
bool WM8731_isInitialised( PWM8731Ctx_t ctx );

//Clears FIFOs
// - returns 0 if successful
HpsErr_t WM8731_clearFIFO( PWM8731Ctx_t ctx, bool adc, bool dac);

//Get FIFO Space Address
HpsErr_t WM8731_getFIFOSpacePtr( PWM8731Ctx_t ctx, volatile unsigned char** fifoSpacePtr );

//Get Left FIFO Address
HpsErr_t WM8731_getLeftFIFOPtr( PWM8731Ctx_t ctx, volatile unsigned int** fifoPtr );

//Get Right FIFO Address
HpsErr_t WM8731_getRightFIFOPtr( PWM8731Ctx_t ctx, volatile unsigned int** fifoPtr );

#endif /*DE1SoC_WM8731_H_*/
