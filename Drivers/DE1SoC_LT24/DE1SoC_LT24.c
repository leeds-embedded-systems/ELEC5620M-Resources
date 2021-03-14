
#include "DE1SoC_LT24.h"
#include "../HPS_Watchdog/HPS_Watchdog.h"
#include "../HPS_usleep/HPS_usleep.h" //some useful delay routines

//
// Driver global static variables (visible only to this .c file)
//

//Driver Base Addresses
volatile unsigned int   *lt24_pio_ptr    = 0x0;  //0xFF200060
volatile unsigned short *lt24_hwbase_ptr = 0x0;  //0xFF200080
//Driver Initialised
bool lt24_initialised = false;

//
// Useful Defines
//

//Globally define this macro to enable the Hardware Optimised mode.
//#define HARDWARE_OPTIMISED

//PIO Bit Map
#define LT24_WRn        (1 << 16)
#define LT24_RS         (1 << 17)
#define LT24_RDn        (1 << 18)
#define LT24_CSn        (1 << 19)
#define LT24_RESETn     (1 << 20)
#define LT24_LCD_ON     (1 << 21)
#define LT24_HW_OPT     (1 << 23)
#define LT24_CMDDATMASK (LT24_CSn | LT24_RDn | LT24_RS | LT24_WRn | 0x0000FFFF) //CMD and Data bits in PIO

//LT24 Dedicated Address Offsets
#define LT24_DEDCMD  (0x00/sizeof(unsigned short))
#define LT24_DEDDATA (0x02/sizeof(unsigned short))

//LT24 PIO Address Offsets
#define LT24_PIO_DATA (0x00/sizeof(unsigned int))
#define LT24_PIO_DIR  (0x04/sizeof(unsigned int))

//Display Initialisation Data
//You don't need to worry about what all these registers are.
//The LT24 LCDs are complicated things with many settings that need
//to be configured - Contrast/Brightness/Data Format/etc.
#define LT24_INIT_DATA_LEN (sizeof(LT24_initData)/sizeof(LT24_initData[0]))
unsigned short LT24_initData [][2] = {
   //isDat, value
    {false, 0x00EF},
    {true , 0x0003},
    {true , 0x0080},
    {true , 0X0002},
    {false, 0x00CF},
    {true , 0x0000},
    {true , 0x0081},
    {true , 0x00C0},
    {false, 0x00ED},
    {true , 0x0064},
    {true , 0x0003},
    {true , 0X0012},
    {true , 0X0081},
    {false, 0x00E8},
    {true , 0x0085},
    {true , 0x0001},
    {true , 0x0078},
    {false, 0x00CB},
    {true , 0x0039},
    {true , 0x002C},
    {true , 0x0000},
    {true , 0x0034},
    {true , 0x0002},
    {false, 0x00F7},
    {true , 0x0020},
    {false, 0x00EA},
    {true , 0x0000},
    {true , 0x0000},
    //Power control
    {false, 0x00C0},         
    {true , 0x0023}, //VRH[5:0]
    {false, 0x00C1},
    {true , 0x0010}, //SAP[2:0];BT[3:0]
    //VCM control
    {false, 0x00C5},         
    {true , 0x003E},
    {true , 0x0028},
    {false, 0x00C7},
    {true , 0x0086},
    // Memory Access Control (MADCTL)
    {false, 0x0036},         
    {true , 0x0048},
    // More settings...
    {false, 0x003A},
    {true , 0x0055},
    {false, 0x00B1},
    {true , 0x0000},
    {true , 0x001B},
    {false, 0x00B6},
    {true , 0x0008}, //Non-Display Area Inaccessible
    {true , 0x0082}, //Normally White, Normal Scan Direction (A2 = Reverse Scan Direction)
    {true , 0x0027}, //320 Lines
    //3-Gamma Function Disable
    {false, 0x00F2},         
    {true , 0x0000},
    //Gamma curve selected
    {false, 0x0026},         
    {true , 0x0001},
    //Set Gamma
    {false, 0x00E0},         
    {true , 0x000F},
    {true , 0x0031},
    {true , 0x002B},
    {true , 0x000C},
    {true , 0x000E},
    {true , 0x0008},
    {true , 0x004E},
    {true , 0X00F1},
    {true , 0x0037},
    {true , 0x0007},
    {true , 0x0010},
    {true , 0x0003},
    {true , 0x000E},
    {true , 0x0009},
    {true , 0x0000},
    {false, 0x00E1},
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
    //Frame Rate
    {false, 0x00B1},
    {true , 0x0000},
    {true , 0x0001},
    //Interface Control
    {false, 0x00F6},
    {true , 0x0001},
    {true , 0x0010},
    {true , 0x0000},
    //Disable Internal Sleep
    {false, 0x0011},
};

signed int LT24_initialise( unsigned int pio_base_address, unsigned int pio_hw_base_address )
{
    unsigned int regVal;
    unsigned int idx;
    
    //Set the local base address pointers
    lt24_pio_ptr = (unsigned int *) pio_base_address;
    lt24_hwbase_ptr = (unsigned short *) pio_hw_base_address;
    
    //Initialise LCD PIO direction
    //Read-Modify-Write
    regVal = lt24_pio_ptr[LT24_PIO_DIR]; //Read
    regVal = regVal | (LT24_CMDDATMASK | LT24_LCD_ON | LT24_RESETn | LT24_HW_OPT); //All data/cmd bits are outputs
    lt24_pio_ptr[LT24_PIO_DIR] = regVal; //Write
    
    //Initialise LCD data/control register.
    //Read-Modify-Write
    regVal = lt24_pio_ptr[LT24_PIO_DATA]; //Read
    regVal = regVal & ~(LT24_CMDDATMASK | LT24_LCD_ON | LT24_RESETn | LT24_HW_OPT); //Mask all data/cmd bits
    regVal = regVal | (LT24_CSn | LT24_WRn | LT24_RDn);  //Deselect Chip and set write and read signals to idle.
#ifdef HARDWARE_OPTIMISED
    regVal = regVal | LT24_HW_OPT; //Enable HW opt bit.
#endif
    lt24_pio_ptr[LT24_PIO_DATA] = regVal; //Write
        
    //LCD requires specific reset sequence: 
    LT24_powerConfig(true);  //turn on for 1ms
    usleep(1000);
    LT24_powerConfig(false); //then off for 10ms
    usleep(10000);
    LT24_powerConfig(true);  //finally back on and wait 120ms for LCD to power on
    usleep(120000);
    
    //Upload Initialisation Data
    for (idx = 0; idx < LT24_INIT_DATA_LEN; idx++) {
        LT24_write(LT24_initData[idx][0], LT24_initData[idx][1]);
    }
    
    //Allow 120ms time for LCD to wake up
    usleep(120000);
    
    //Turn on display drivers
    LT24_write(false, 0x0029);

    //Mark as initialised so later functions know we are ready
    lt24_initialised = true;
    
    //And clear the display
    return LT24_clearDisplay(LT24_BLACK);
}

//Check if driver initialised
bool LT24_isInitialised() {
    return lt24_initialised;
}

#ifdef HARDWARE_OPTIMISED
//If HARDWARE_OPTIMISED is defined, use this optimised function. It will be discussed in the lab on LCDs.

//Function for writing to LT24 Registers (using dedicated HW)
//You must check LT24_isInitialised() before calling this function
void LT24_write( bool isData, unsigned short value )
{
    if (isData) {
        lt24_hwbase_ptr[LT24_DEDDATA] = value;
    } else {
        lt24_hwbase_ptr[LT24_DEDCMD ] = value;
    }
}

#else
//Otherwise use the non-optimised function.

//Function for writing to LT24 Registers (using PIO)
//You must check LT24_isInitialised() before calling this function
void LT24_write( bool isData, unsigned short value )
{
    //PIO controls more than just LT24, so need to Read-Modify-Write
    //First we have to output the value with the LT24_WRn bit low (first cycle of write)
    //Read
    unsigned int regVal = lt24_pio_ptr[LT24_PIO_DATA];
    //Modify
    //Mask all bits for command and data (sets them all to 0)
    regVal = regVal & ~LT24_CMDDATMASK;
    //Set the data bits (unsigned value, so cast pads MSBs with 0's)
    regVal = regVal | ((unsigned int)value); 
    if (isData) {
        //For data we set the RS bit high.
        regVal = regVal | (LT24_RS | LT24_RDn);
    } else {
        //For command we don't set the RS bit
        regVal = regVal | (LT24_RDn);
    }
    //Write
    lt24_pio_ptr[LT24_PIO_DATA] = regVal;
    //Then we need to output the value again with LT24_WRn high (second cycle of write)
    //Rest of regVal is unchanged, so we just or on the LT24_WRn bit
    regVal = regVal | (LT24_WRn); 
    //Write
    lt24_pio_ptr[LT24_PIO_DATA] = regVal;
}

#endif


//Function for configuring LCD reset/power (using PIO)
//You must check LT24_isInitialised() before calling this function
void LT24_powerConfig( bool isOn )
{
    //Read
    unsigned int regVal = lt24_pio_ptr[LT24_PIO_DATA];
    //Modify
    if (isOn) {
        //To turn on we must set the RESETn and LCD_ON bits high
        regVal = regVal |  (LT24_RESETn | LT24_LCD_ON);
    } else {
        //To turn off we must set the RESETn and LCD_ON bits low
        regVal = regVal & ~(LT24_RESETn | LT24_LCD_ON);
    }
    //Write
    lt24_pio_ptr[LT24_PIO_DATA] = regVal;
}

//Function to clear display to a set colour
// - Returns true if successful
signed int LT24_clearDisplay(unsigned short colour)
{
    signed int status;
    unsigned int idx;
    //Reset watchdog.
    ResetWDT();
    //Define window as entire display (LT24_setWindow will check if we are initialised).
    status = LT24_setWindow(0, 0, LT24_WIDTH, LT24_HEIGHT);
    if (status != LT24_SUCCESS) return status;
    //Loop through each pixel in the window writing the required colour
    for(idx=0;idx<(LT24_WIDTH*LT24_HEIGHT);idx++)
    {
        LT24_write(true, colour);
    }
    //And done.
    return LT24_SUCCESS;
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
signed int LT24_setWindow( unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height) {
    unsigned int xright, ybottom;
    if (!LT24_isInitialised()) return LT24_ERRORNOINIT; //Don't run if not yet initialised
    //Calculate bottom right corner location
    xright = xleft + width - 1;
    ybottom = ytop + height - 1;
    //Ensure end coordinates are in range
    if (xright >= LT24_WIDTH)   return LT24_INVALIDSIZE; //Invalid size
    if (ybottom >= LT24_HEIGHT) return LT24_INVALIDSIZE; //Invalid size
    //Ensure start coordinates are in range (top left must be <= bottom right)
    if (xleft > xright) return LT24_INVALIDSHAPE; //Invalid shape
    if (ytop > ybottom) return LT24_INVALIDSHAPE; //Invalid shape
    //Define the left and right of the display
    LT24_write(false, 0x002A);
    LT24_write(true , (xleft >> 8) & 0xFF);
    LT24_write(true , xleft & 0xFF);
    LT24_write(true , (xright >> 8) & 0xFF);
    LT24_write(true , xright & 0xFF);
    //Define the top and bottom of the display
    LT24_write(false, 0x002B);
    LT24_write(true , (ytop >> 8) & 0xFF);
    LT24_write(true , ytop & 0xFF);
    LT24_write(true , (ybottom >> 8) & 0xFF);
    LT24_write(true , ybottom & 0xFF);
    //Create window and prepare for data
    LT24_write(false, 0x002c);
    //Done
    return LT24_SUCCESS;
}

//Internal function to generate Red/Green corner of test pattern
signed int LT24_redGreen(unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height){
    signed int status;
	unsigned int i, j;
    unsigned short colour;
    //Reset watchdog
    ResetWDT();
    //Define Window
    status = LT24_setWindow(xleft,ytop,width,height);
    if (status != LT24_SUCCESS) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((i * 0x20)/width, (j * 0x20)/height, 0);
            LT24_write(true, colour);
        }
    }
    //Done
    return LT24_SUCCESS;
}

//Internal function to generate Green/Blue corner of test pattern
signed int LT24_greenBlue(unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height){
    signed int status;
	unsigned int i, j;
    unsigned short colour;
    //Reset watchdog
    ResetWDT();
    //Define Window
    status = LT24_setWindow(xleft,ytop,width,height);
    if (status != LT24_SUCCESS) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour(0, (i * 0x20)/width, (j * 0x20)/height);
            LT24_write(true, colour);
        }
    }
    //Done
    return LT24_SUCCESS;
}

//Internal function to generate Blue/Red of test pattern
signed int LT24_blueRed(unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height){
    signed int status;
	unsigned int i, j;
    unsigned short colour;
    //Reset watchdog
    ResetWDT();
    //Define Window
    status = LT24_setWindow(xleft,ytop,width,height);
    if (status != LT24_SUCCESS) return status;
    //Create test pattern
    for (j = 0;j < height;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((j * 0x20)/height, 0, (i * 0x20)/width);
            LT24_write(true, colour);
        }
    }
    //Done
    return LT24_SUCCESS;
}

//Internal function to generate colour bars of test pattern
signed int LT24_colourBars(unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height){
    signed int status;
	unsigned int i, j;
    unsigned int colourbars[6] = {LT24_RED,LT24_YELLOW,LT24_GREEN,LT24_CYAN,LT24_BLUE,LT24_MAGENTA};
    unsigned short colour;
    //Reset watchdog
    ResetWDT();
    //Define Window
    status = LT24_setWindow(xleft,ytop,width,height);
    if (status != LT24_SUCCESS) return status;
    //Generate Colour Bars
    for (j = 0;j < height/2;j++){
        for (i = 0;i < width;i++){
            colour = LT24_makeColour((i * 0x20)/width, (i * 0x20)/width, (i * 0x20)/width);
            LT24_write(true, colour);
        }
    }
    //Generate tone shades
    for (j = height/2;j < height;j++){
        for (i = 0;i < width;i++){
            LT24_write(true, colourbars[(i*6)/width]);
        }
    }
    //Done
    return LT24_SUCCESS;
}

//Generates test pattern on display
// - returns 0 if successful
signed int LT24_testPattern(){
    signed int status; 
    status = LT24_redGreen  (           0,            0,LT24_WIDTH/2,LT24_HEIGHT/2);
    if (status != LT24_SUCCESS) return status;
    status = LT24_greenBlue (           0,LT24_HEIGHT/2,LT24_WIDTH/2,LT24_HEIGHT/2);
    if (status != LT24_SUCCESS) return status;
    status = LT24_blueRed   (LT24_WIDTH/2,            0,LT24_WIDTH/2,LT24_HEIGHT/2);
    if (status != LT24_SUCCESS) return status;
    status = LT24_colourBars(LT24_WIDTH/2,LT24_HEIGHT/2,LT24_WIDTH/2,LT24_HEIGHT/2);
    return status;
}

//Copy frame buffer to display
// - returns 0 if successful
signed int LT24_copyFrameBuffer(const unsigned short* framebuffer, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height)
{
    unsigned int cnt;
    //Define Window
    signed int status = LT24_setWindow(xleft,ytop,width,height);
    if (status != LT24_SUCCESS) return status;
    //And Copy
    cnt = (height * width); //How many pixels.
    while (cnt--) {
        LT24_write(true, *framebuffer++);
    }
    //Done
    return LT24_SUCCESS;
}

//Plot a single pixel on the LT24 display
// - returns 0 if successful
signed int LT24_drawPixel(unsigned short colour,unsigned int x,unsigned int y)
{
    signed int status = LT24_setWindow(x,y,1,1); //Define single pixel window
    if (status != LT24_SUCCESS) return status;   //Check for any errors
    LT24_write(true, colour);                    //Write one pixel of colour data
    return LT24_SUCCESS;                         //And Done
}

