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
 * for your graphics code, which takes an LT24Ctx_t instance
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

#include "DE1SoC_LT24.h"

#include "Util/watchdog.h"
#include "Util/delay.h"
#include "Util/bit_helpers.h"

//
// Useful Defines
//

//PIO Bit Map
#define LT24_WRn        (  1U << 16)
#define LT24_RS         (  1U << 17)
#define LT24_RDn        (  1U << 18)
#define LT24_CSn        (  1U << 19)
#define LT24_RESETn     (  1U << 20)
#define LT24_LCD_ON     (  1U << 21)
#define LT24_HW_OPT(en) ((en) << 23)
#define LT24_CMDDATMASK (LT24_CSn | LT24_RDn | LT24_RS | LT24_WRn | 0x0000FFFFU) //CMD and Data bits in PIO
#define LT24_PIOMASK    (LT24_CMDDATMASK | LT24_LCD_ON | LT24_RESETn | LT24_HW_OPT(1))

//LT24 Dedicated Address Offsets
#define LT24_DEDCMD  (0x00/sizeof(unsigned short))
#define LT24_DEDDATA (0x02/sizeof(unsigned short))

//Display Initialisation Data
//You don't need to worry about what all these registers are.
//The LT24 LCDs are complicated things with many settings that need
//to be configured - Contrast/Brightness/Data Format/etc.
#define LT24_INIT_DATA_LEN (sizeof(LT24_initData)/sizeof(LT24_initData[0]))
unsigned short LT24_initData [][2] = {
   //isDat, value
    {false, 0x00EF}, //Undocumented write sequence!
    {true , 0x0003},
    {true , 0x0080},
    {true , 0x0002},
    {false, 0x00CF}, //Power control B
    {true , 0x0000},
    {true , 0x0081},
    {true , 0x00C0},
    {false, 0x00ED}, //Power on sequence control
    {true , 0x0064},
    {true , 0x0003},
    {true , 0x0012},
    {true , 0x0081},
    {false, 0x00E8}, //Driver timing control A
    {true , 0x0085},
    {true , 0x0001},
    {true , 0x0078},
    {false, 0x00CB}, //Power controlA
    {true , 0x0039},
    {true , 0x002C},
    {true , 0x0000},
    {true , 0x0034},
    {true , 0x0002},
    {false, 0x00F7}, //Pump ratio control
    {true , 0x0020},
    {false, 0x00EA}, //Driver timing control B
    {true , 0x0000}, //NOP
    {true , 0x0000}, //NOP
    {false, 0x00C0}, //Power control 1
    {true , 0x0023}, //  VRH[5:0]
    {false, 0x00C1}, //Power control 2
    {true , 0x0010}, //  SAP[2:0];BT[3:0]
    {false, 0x00C5}, //VCOM Control 1
    {true , 0x003E},
    {true , 0x0028},
    {false, 0x00C7}, //VCOM Control 2
    {true , 0x0086},
    {false, 0x0036}, //Memory Access Control (MADCTL)
    {true , 0x0048},
    {false, 0x003A}, //Pixel Format Set
    {true , 0x0055}, //  16 bit RGB Interface, 16 bit MCU
    {false, 0x00B1}, //Frame Control
    {true , 0x0000}, //  No-clock oscillator division
    {true , 0x001B}, //  27 clock/line, 70 Hz default
    {false, 0x00B6}, //Display Function Control
    {true , 0x0008}, //  Non-Display Area Inaccessible
    {true , 0x0082}, //  Normally White, Normal Scan Direction (A2 = Reverse Scan Direction)
    {true , 0x0027}, //  320 Lines
    {false, 0x00F2}, //3-Gamma Function Disable
    {true , 0x0000},
    {false, 0x0026}, //Gamma curve selected
    {true , 0x0001},
    {false, 0x00E0}, //Positive Gamma Correction
    {true , 0x000F},
    {true , 0x0031},
    {true , 0x002B},
    {true , 0x000C},
    {true , 0x000E},
    {true , 0x0008},
    {true , 0x004E},
    {true , 0x00F1},
    {true , 0x0037},
    {true , 0x0007},
    {true , 0x0010},
    {true , 0x0003},
    {true , 0x000E},
    {true , 0x0009},
    {true , 0x0000},
    {false, 0x00E1}, //Negative Gamma Correction
    {true , 0x0000},
    {true , 0x000E},
    {true , 0x0014},
    {true , 0x0003},
    {true , 0x0011},
    {true , 0x0007},
    {true , 0x0031},
    {true , 0x00C1},
    {true , 0x0048},
    {true , 0x0008},
    {true , 0x000F},
    {true , 0x000C},
    {true , 0x0031},
    {true , 0x0036},
    {true , 0x000f},
    {false, 0x00B1}, //Frame Rate
    {true , 0x0000},
    {true , 0x0001},
    {false, 0x00F6}, //Interface Control
    {true , 0x0001},
    {true , 0x0010},
    {true , 0x0000},
    {false, 0x0011}, //Disable Internal Sleep
};

/*
 * Internal Functions
 */

//Function for writing to LT24 Registers
static HpsErr_t _LT24_write( LT24Ctx_t* ctx, bool isData, unsigned short value ) {
    if (ctx->hwOpt) {
        // Use data interface in hwOpt mode
        if (isData) {
            ctx->hwOpt[LT24_DEDDATA] = value;
        } else {
            ctx->hwOpt[LT24_DEDCMD ] = value;
        }
        return ERR_SUCCESS;
    } else {
        //Build new register value, starting with the data bits
        unsigned int regVal = value; 
        if (isData) {
            //For data we set the RS bit high.
            regVal |= LT24_RS | LT24_RDn;
        } else {
            //For command we don't set the RS bit
            regVal |= LT24_RDn;
        }
        //Write to the PIO controller, changing only the command and data bits
        HpsErr_t status = GPIO_setOutput(ctx->cntrl, regVal, LT24_CMDDATMASK);
        if (ERR_IS_ERROR(status)) return status;
        //Output the same value again but with LT24_WRn high (2nd cycle of write)
        //Rest of regVal is unchanged, so we just OR the LT24_WRn bit
        regVal |= LT24_WRn;
        return GPIO_setOutput(ctx->cntrl, regVal, LT24_CMDDATMASK);
    }
}

//Internal function to generate Red/Green corner of test pattern
static HpsErr_t _LT24_redGreen( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height ) {
    HpsErr_t status;
	unsigned int i, j;
    unsigned short colour;
    ResetWDT();
    //Define Window
    status = LT24_setWindow(ctx, xleft,ytop,width,height);
    if (ERR_IS_ERROR(status)) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((i * 0x20)/width, (j * 0x20)/height, 0);
            _LT24_write(ctx, true, colour);
        }
    }
    //Done
    return ERR_SUCCESS;
}

//Internal function to generate Green/Blue corner of test pattern
static HpsErr_t _LT24_greenBlue( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height ) {
    HpsErr_t status;
	unsigned int i, j;
    unsigned short colour;
    ResetWDT();
    //Define Window
    status = LT24_setWindow(ctx, xleft,ytop,width,height);
    if (ERR_IS_ERROR(status)) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour(0, (i * 0x20)/width, (j * 0x20)/height);
            _LT24_write(ctx, true, colour);
        }
    }
    //Done
    return ERR_SUCCESS;
}

//Internal function to generate Blue/Red of test pattern
static HpsErr_t _LT24_blueRed( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height ) {
    HpsErr_t status;
	unsigned int i, j;
    unsigned short colour;
    ResetWDT();
    //Define Window
    status = LT24_setWindow(ctx, xleft,ytop,width,height);
    if (ERR_IS_ERROR(status)) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((j * 0x20)/height, 0, (i * 0x20)/width);
            _LT24_write(ctx, true, colour);
        }
    }
    return ERR_SUCCESS;
}

//Internal function to generate colour bars of test pattern
static HpsErr_t _LT24_colourBars( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height ) {
    HpsErr_t status;
	unsigned int i, j;
    unsigned int colourbars[6] = {LT24_RED,LT24_YELLOW,LT24_GREEN,LT24_CYAN,LT24_BLUE,LT24_MAGENTA};
    unsigned short colour;
    ResetWDT();
    //Define Window
    status = LT24_setWindow(ctx, xleft,ytop,width,height);
    if (ERR_IS_ERROR(status)) return status;
    //Generate Colour Bars
    for (j = 0;j < height/2;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((i * 0x20)/width, (i * 0x20)/width, (i * 0x20)/width);
            _LT24_write(ctx, true, colour);
        }
    }
    //Generate tone shades
    for (j = height/2;j < height;j++){
        for (i = 0;i < width;i++){
            _LT24_write(ctx, true, colourbars[(i*6)/width]);
        }
    }
    return ERR_SUCCESS;
}

static HpsErr_t _LT24_powerConfig( LT24Ctx_t* ctx, bool isOn ) {
    if (isOn) {
        //To turn on we must set the RESETn and LCD_ON bits high
        return GPIO_setOutput(ctx->cntrl, UINT32_MAX, (LT24_RESETn | LT24_LCD_ON));
    } else {
        //To turn off we must set the RESETn and LCD_ON bits low
        return GPIO_setOutput(ctx->cntrl,          0, (LT24_RESETn | LT24_LCD_ON));
    }
}

// Cleanup function called when driver destroyed.
static void _LT24_cleanup( LT24Ctx_t* ctx ) {
    if (ctx->cntrl) {
        // Turn off LCD
        _LT24_write(ctx, false, 0x0028);
        _LT24_powerConfig(ctx, false);
    }
}

/*
 * User Facing APIs
 */

//Function to initialise the LCD
//  - cntrl is a GPIO instance used to configure the control pins for the LT24.
//  - dataBase if non-NULL indicates using hardware optimised mode. Must be base
//    address of the optimised data transfer buffer
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t LT24_initialise( GpioCtx_t* cntrl, void* dataBase, LT24Ctx_t** pCtx ) {
    //Control GPIO must be valid and initialised
    if (!GPIO_isInitialised(cntrl)) return ERR_BADDEVICE;
    //Optimised data address must be integer aligned (NULL is allowed)
    if (!pointerIsAligned(dataBase, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_LT24_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    LT24Ctx_t* ctx = *pCtx;
    ctx->cntrl = cntrl;
    ctx->hwOpt  = (unsigned short*)dataBase;
    
    //Initialise LCD PIO direction. All data/cmd bits are outputs
    GPIO_setDirection(ctx->cntrl, UINT32_MAX, LT24_PIOMASK);
    
    //Initialise LCD data/control register. Deselect Chip and set write and read signals
    //to idle. Set the HW opt bit if enabled.
    GPIO_setOutput(ctx->cntrl, (LT24_CSn | LT24_WRn | LT24_RDn | LT24_HW_OPT(ctx->hwOpt != NULL)), LT24_PIOMASK);
    
    //LCD requires specific reset sequence:
    _LT24_powerConfig(ctx, true);  //turn on for 1ms
    usleep(1000);
    _LT24_powerConfig(ctx, false); //then off for 10ms
    usleep(10000);
    _LT24_powerConfig(ctx, true);  //finally back on and wait 120ms for LCD to power on
    usleep(120000);
    
    //Upload Initialisation Data
    for (unsigned int idx = 0; idx < LT24_INIT_DATA_LEN; idx++) {
        _LT24_write(ctx, LT24_initData[idx][0], LT24_initData[idx][1]);
    }
    
    //Allow 120ms time for LCD to wake up
    usleep(120000);
    
    //Turn on display drivers
    _LT24_write(ctx, false, 0x0029);

    //Mark as initialised so later functions know we are ready
    DriverContextSetInit(ctx);
    
    //And clear the display
    return LT24_clearDisplay(ctx, LT24_BLACK);
}

//Check if driver initialised
bool LT24_isInitialised(LT24Ctx_t* ctx) {
    return DriverContextCheckInit(ctx);
}

//Function for writing to LT24 Registers
HpsErr_t LT24_write( LT24Ctx_t* ctx, bool isData, unsigned short value ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Then perform write
    return _LT24_write(ctx, isData, value);
}

//Function for configuring LCD reset/power (using PIO)
//You must check LT24_isInitialised() before calling this function
HpsErr_t LT24_powerConfig( LT24Ctx_t* ctx, bool isOn ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Then configure
    return _LT24_powerConfig(ctx, isOn);
}

//Function to clear display to a set colour
// - Returns true if successful
HpsErr_t LT24_clearDisplay( LT24Ctx_t* ctx, unsigned short colour) {
    ResetWDT();
    //Define window as entire display (LT24_setWindow will check if we are initialised).
    HpsErr_t status = LT24_setWindow(ctx, 0, 0, LT24_WIDTH, LT24_HEIGHT);
    if (ERR_IS_ERROR(status)) return status;
    //Loop through each pixel in the window writing the required colour
    for(unsigned int idx=0; idx<(LT24_WIDTH*LT24_HEIGHT); idx++) {
        _LT24_write(ctx, true, colour);
    }
    return ERR_SUCCESS;
}

//Function to convert Red/Green/Blue to RGB565 encoded colour value 
unsigned short LT24_makeColour( unsigned int R, unsigned int G, unsigned int B ) {
    unsigned short colour;
    //Limit the colours to the maximum range
    if (R > 0x1F) R = 0x1F;
    if (G > 0x3F) G = 0x3F;
    if (B > 0x1F) B = 0x1F;
    //Move the RGB values to the correct place in the encoded colour
    colour = (R << 11) + (G << 5) + (B << 0);
    return colour;
}

//Function to set the drawing window on the display
//  Returns 0 if successful
HpsErr_t LT24_setWindow( LT24Ctx_t* ctx, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Calculate bottom right corner location
    unsigned int xright = xleft + width - 1;
    unsigned int ybottom = ytop + height - 1;
    //Ensure end coordinates are in range
    if (xright >= LT24_WIDTH)   return LT24_INVALIDSIZE; //Invalid size
    if (ybottom >= LT24_HEIGHT) return LT24_INVALIDSIZE; //Invalid size
    //Ensure start coordinates are in range (top left must be <= bottom right)
    if (xleft > xright) return LT24_INVALIDSHAPE; //Invalid shape
    if (ytop > ybottom) return LT24_INVALIDSHAPE; //Invalid shape
    //Define the left and right of the display
    _LT24_write(ctx, false, 0x002A);
    _LT24_write(ctx, true , (xleft >> 8) & 0xFF);
    _LT24_write(ctx, true , xleft & 0xFF);
    _LT24_write(ctx, true , (xright >> 8) & 0xFF);
    _LT24_write(ctx, true , xright & 0xFF);
    //Define the top and bottom of the display
    _LT24_write(ctx, false, 0x002B);
    _LT24_write(ctx, true , (ytop >> 8) & 0xFF);
    _LT24_write(ctx, true , ytop & 0xFF);
    _LT24_write(ctx, true , (ybottom >> 8) & 0xFF);
    _LT24_write(ctx, true , ybottom & 0xFF);
    //Create window and prepare for data
    _LT24_write(ctx, false, 0x002c);
    return ERR_SUCCESS;
}

//Generates test pattern on display
// - returns 0 if successful
HpsErr_t LT24_testPattern( LT24Ctx_t* ctx ) {
    //Generate different test pattern for each corner of the display (test pattern funtion will validate ctx)
    HpsErr_t status;
    status = _LT24_redGreen  ( ctx,            0,            0,LT24_WIDTH/2,LT24_HEIGHT/2 );
    if (ERR_IS_ERROR(status)) return status;
    status = _LT24_greenBlue ( ctx,            0,LT24_HEIGHT/2,LT24_WIDTH/2,LT24_HEIGHT/2 );
    if (ERR_IS_ERROR(status)) return status;
    status = _LT24_blueRed   ( ctx, LT24_WIDTH/2,            0,LT24_WIDTH/2,LT24_HEIGHT/2 );
    if (ERR_IS_ERROR(status)) return status;
    status = _LT24_colourBars( ctx, LT24_WIDTH/2,LT24_HEIGHT/2,LT24_WIDTH/2,LT24_HEIGHT/2 );
    return status;
}

//Copy frame buffer to display
// - returns 0 if successful
HpsErr_t LT24_copyFrameBuffer( LT24Ctx_t* ctx, const unsigned short* framebuffer, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height ) {
    //Define Window (setWindow validates context for us)
    HpsErr_t status = LT24_setWindow(ctx, xleft, ytop, width, height);
    if (ERR_IS_ERROR(status)) return status;
    //And copy the required number of pixels
    unsigned int cnt = (height * width);
    while (cnt--) {
        _LT24_write(ctx, true, *framebuffer++);
    }
    return ERR_SUCCESS;
}

//Plot a single pixel on the LT24 display
// - returns 0 if successful
HpsErr_t LT24_drawPixel( LT24Ctx_t* ctx, unsigned short colour, unsigned int x, unsigned int y ) {
    //Define single pixel window (setWindow validates context for us)
    HpsErr_t status = LT24_setWindow(ctx, x, y, 1, 1);
    if (ERR_IS_ERROR(status)) return status;
    //Write one pixel of colour data
    return _LT24_write(ctx, true, colour);
}

