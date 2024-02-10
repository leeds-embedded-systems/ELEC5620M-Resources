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
 * 10/02/2024 | Add new API for FIFO access
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

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    PHPSI2CCtx_t i2c; // I2C peripheral used by Audio Codec
    unsigned int i2cAddr;
    unsigned int sampleRate;
} WM8731Ctx_t, *PWM8731Ctx_t;

//Initialise Audio Codec
// - base_address is memory-mapped address of audio controller
// - returns 0 if successful
HpsErr_t WM8731_initialise( void* base, PHPSI2CCtx_t i2c, PWM8731Ctx_t* pCtx );

//Check if driver initialised
// - Returns true if driver previously initialised
// - WM8731_initialise() must be called if false.
bool WM8731_isInitialised( PWM8731Ctx_t ctx );

//Get the sample rate for the ADC/DAC
HpsErr_t WM8731_getSampleRate( PWM8731Ctx_t ctx, unsigned int* sampleRate );

//Clears FIFOs
// - returns 0 if successful
HpsErr_t WM8731_clearFIFO( PWM8731Ctx_t ctx, bool adc, bool dac);

//Get FIFO Space (DAC)
HpsErr_t WM8731_getFIFOSpace( PWM8731Ctx_t ctx, unsigned int* fifoSpace );

//Get FIFO Fill (ADC)
HpsErr_t WM8731_getFIFOFill( PWM8731Ctx_t ctx, unsigned int* fifoFill );

//Write a sample to the FIFO for the both channels
// - You must check there is data available in the FIFO before calling this function.
HpsErr_t WM8731_writeSample( PWM8731Ctx_t ctx, unsigned int left, unsigned int right);

//Read a sample from the FIFO for both channels
// - You must check there is space in the FIFO before calling this function.
HpsErr_t WM8731_readSample( PWM8731Ctx_t ctx, unsigned int* left, unsigned int* right);

#endif /*DE1SoC_WM8731_H_*/
