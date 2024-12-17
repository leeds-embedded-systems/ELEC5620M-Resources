/* 
 * WM8731 Audio Controller Driver
 * ------------------------------
 * Description: 
 * 
 * Driver for the WM8731 Audio Controller in the Leeds SoC Computer
 * 
 * This core will configure the audio codec to the correct interface
 * settings and by default power it on to use the Line In as the ADC
 * input, and Line Out as the DAC output.
 * 
 * FIFO APIs are provided to allow reading and writing samples to
 * and from the FIFOs in the FPGA. The FIFOs ensure data is fed out
 * and received at the correct sample rate, 48kHz currently, to allow
 * bursts of samples to be read/written by the CPU.
 *
 * When using with the DE1SoC, if you are using the HPS I2C
 * controller (as opposed to an FPGA I2C controller) as in the
 * Leeds SoC Computer, you must initialise the HPS GPIO instance
 * with the following code to ensure that the I2C mux selects 
 * the HPS I2C controller as the configuration source:
 * 
 *    HPS_GPIO_initialise(LSC_BASE_ARM_GPIO, ARM_GPIO_DIR, ARM_GPIO_I2C_GENERAL_MUX, 0, &gpio)
 * 
 * An additional WM8731_writeRegister API is provided to allow
 * changing registers in the audio codec, e.g. to configure the
 * volume or filtering settings.
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-------------------------------
 * 21/11/2024 | Use generic I2C interface
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
#include "Util/driver_i2c.h"

//I2C Register Addresses
typedef enum {
    WM8731_REG_LEFTINCNTRL   = (0x00/sizeof(unsigned short)),
    WM8731_REG_RIGHTINCNTRL  = (0x02/sizeof(unsigned short)),
    WM8731_REG_LEFTOUTCNTRL  = (0x04/sizeof(unsigned short)),
    WM8731_REG_RIGHTOUTCNTRL = (0x06/sizeof(unsigned short)),
    WM8731_REG_ANLGPATHCNTRL = (0x08/sizeof(unsigned short)),
    WM8731_REG_DGTLPATHCNTRL = (0x0A/sizeof(unsigned short)),
    WM8731_REG_POWERCNTRL    = (0x0C/sizeof(unsigned short)),
    WM8731_REG_DATAFMTCNTRL  = (0x0E/sizeof(unsigned short)),  // Changes to this register are not allowed
    WM8731_REG_SMPLINGCNTRL  = (0x10/sizeof(unsigned short)),
    WM8731_REG_ACTIVECNTRL   = (0x12/sizeof(unsigned short))
} WM8731RegAddress;

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    I2CCtx_t* i2c; // I2C peripheral used by Audio Codec
    unsigned short i2cAddr;
    unsigned int sampleRate;
} WM8731Ctx_t;

//Initialise Audio Codec
// - base is memory-mapped address of audio controller data interface
//   - If base is NULL, provides access to the I2C configuration interface only.
// - i2c is a generic I2C driver instance used to configure the controller.
HpsErr_t WM8731_initialise( void* base, I2CCtx_t* i2c, WM8731Ctx_t** pCtx );

//Check if driver initialised
// - Returns true if driver previously initialised
// - WM8731_initialise() must be called if false.
bool WM8731_isInitialised( WM8731Ctx_t* ctx );

//Configure an I2C register
// - Allows changing settings for codec (e.g. volumen control, filtering, etc)
// - Refer to Page 46 of WM8731 codec datasheet.
HpsErr_t WM8731_writeRegister( WM8731Ctx_t* ctx, WM8731RegAddress regAddr, unsigned int regVal );

//Get the sample rate for the ADC/DAC
HpsErr_t WM8731_getSampleRate( WM8731Ctx_t* ctx, unsigned int* sampleRate );

//Clears FIFOs
// - returns 0 if successful
HpsErr_t WM8731_clearFIFO( WM8731Ctx_t* ctx, bool adc, bool dac);

//Get FIFO Space (DAC)
HpsErr_t WM8731_getFIFOSpace( WM8731Ctx_t* ctx, unsigned int* fifoSpace );

//Get FIFO Fill (ADC)
HpsErr_t WM8731_getFIFOFill( WM8731Ctx_t* ctx, unsigned int* fifoFill );

//Write a sample to the FIFO for the both channels
// - You must check there is data available in the FIFO before calling this function.
HpsErr_t WM8731_writeSample( WM8731Ctx_t* ctx, unsigned int left, unsigned int right);

//Read a sample from the FIFO for both channels
// - You must check there is space in the FIFO before calling this function.
HpsErr_t WM8731_readSample( WM8731Ctx_t* ctx, unsigned int* left, unsigned int* right);

#endif /*DE1SoC_WM8731_H_*/
