/* 
 * LT24 Display Controller Driver
 * ------------------------------
 * Description: 
 * Driver for the LT24 Display Controller
 * 
 * Provides APIs for initialising and writing data to the
 * LT24 display. The driver can operate either in bit-banging
 * mode using a GPIO controller instance, or in a special
 * hardware optimised mode which uses the FPGA fabric to
 * write commands and data to the display more efficiently.
 * 
 * The sideband signals such as power and reset are controlled
 * using a GPIO driver instance. For the Leeds SoC Computer this
 * is an instance of the FPGA_PIO driver. The GPIO instance has
 * been split out from the LT24 driver to make it easier to
 * access other sideband signals such as the touchscreen control
 * bits.
 * 
 * 
 * 
 * DO NOT MODIFY THIS FILE.
 *
 * 
 * 
 * This is a basic driver which provides only the interface
 * APIs. If you wish to make add graphics processing such as
 * Text and Shape display, you should create another driver
 * for your graphics code, which takes an LT24Ctx_t instance.
 * 
 * For example:
 * 
 *     MyGraphicDriver_initialise(LT24Ctx_t* display, ...);
 * 
 * 
 * 
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 21/11/2024 | Use external GPIO instance
 * 31/01/2024 | Update to new driver contexts
 * 20/10/2017 | Update driver to match new styles
 * 05/02/2017 | Creation of driver
 *
 */

#ifndef DE1SoC_LT24_H_
#define DE1SoC_LT24_H_

//Include required header files
#include <stdint.h>
#include "Util/driver_ctx.h"
#include "Util/driver_gpio.h"

//Map some error codes to their use
#define LT24_INVALIDSIZE  ERR_BEYONDEND
#define LT24_INVALIDSHAPE ERR_REVERSED

//Size of the LCD
#define LT24_WIDTH  240
#define LT24_HEIGHT 320

//Some basic colours
enum {
    LT24_BLACK   = (0x0000),
    LT24_WHITE   = (0xFFFF),
    LT24_RED     = (0x1F << 11),
    LT24_GREEN   = (0x1F << 6),
    LT24_BLUE    = (0x1F << 0),
    LT24_YELLOW  = (LT24_RED   | LT24_GREEN),
    LT24_CYAN    = (LT24_GREEN | LT24_BLUE ),
    LT24_MAGENTA = (LT24_BLUE  | LT24_RED  )
};

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    GpioCtx_t* cntrl;
    volatile unsigned short* hwOpt; // Uses hardware optimised interface if non-NULL
} LT24Ctx_t;

//Function to initialise the LCD
//  - cntrl is a GPIO instance used to configure the control pins for the LT24.
//  - dataBase if non-NULL indicates using hardware optimised mode. Must be base
//    address of the optimised data transfer buffer
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t LT24_initialise( GpioCtx_t* cntrl, void* dataBase, LT24Ctx_t** pCtx );

//Check if driver initialised
// - returns true if initialised
bool LT24_isInitialised( LT24Ctx_t* ctx );

//Function for writing to LT24 Registers (using dedicated HW)
//You must check LT24_isInitialised() before calling this function
HpsErr_t LT24_write( LT24Ctx_t* ctx, bool isData, unsigned short value );

//Function for configuring LCD reset/power (using PIO)
//You must check LT24_isInitialised() before calling this function
HpsErr_t LT24_powerConfig( LT24Ctx_t* ctx, bool isOn );

//Function to clear display to a set colour
// - Returns ERR_SUCCESS if successful
HpsErr_t LT24_clearDisplay( LT24Ctx_t* ctx, unsigned short colour );

//Function to convert Red/Green/Blue to RGB565 encoded colour value 
unsigned short LT24_makeColour( unsigned int R, unsigned int G, unsigned int B );

//Function to set the drawing window on the display
//  Returns ERR_SUCCESS if successful
HpsErr_t LT24_setWindow( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height);

//Generates test pattern on display
// - returns ERR_SUCCESS if successful
HpsErr_t LT24_testPattern( LT24Ctx_t* ctx );

//Copy frame buffer to display
// - returns ERR_SUCCESS if successful
HpsErr_t LT24_copyFrameBuffer( LT24Ctx_t* ctx, const unsigned short* framebuffer,
    unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height);

//Plot a single pixel on the LT24 display
// - returns ERR_SUCCESS if successful
HpsErr_t LT24_drawPixel( LT24Ctx_t* ctx, unsigned short colour, unsigned int x, unsigned int y);

#endif /*DE1SoC_LT24_H_*/

/*
Draw a pixel
LT24_setWindow(x,y,1,1);
LT24_write(true,LT24_makeColour(R,G,B));
*/
