
#include "DE1SoC_WM8731.h"
#include "../HPS_I2C/HPS_I2C.h"

//
// Driver global static variables (visible only to this .c file)
//

//Driver Base Address
volatile unsigned int *wm8731_base_ptr = 0x0;
//Driver Initialised
bool wm8731_initialised = false;

//
// Useful Defines
//

//WM8731 ARM Address Offsets
#define WM8731_CONTROL    (0x0/sizeof(unsigned int))
#define WM8731_FIFOSPACE  (0x4/sizeof(unsigned int))
#define WM8731_LEFTFIFO   (0x8/sizeof(unsigned int))
#define WM8731_RIGHTFIFO  (0xC/sizeof(unsigned int))

//I2C Register Address Offsets
#define WM8731_I2C_LEFTINCNTRL   (0x00/sizeof(unsigned short))
#define WM8731_I2C_RIGHTINCNTRL  (0x02/sizeof(unsigned short))
#define WM8731_I2C_LEFTOUTCNTRL  (0x04/sizeof(unsigned short))
#define WM8731_I2C_RIGHTOUTCNTRL (0x06/sizeof(unsigned short))
#define WM8731_I2C_ANLGPATHCNTRL (0x08/sizeof(unsigned short))
#define WM8731_I2C_DGTLPATHCNTRL (0x0A/sizeof(unsigned short))
#define WM8731_I2C_POWERCNTRL    (0x0C/sizeof(unsigned short))
#define WM8731_I2C_DATAFMTCNTRL  (0x0E/sizeof(unsigned short))
#define WM8731_I2C_SMPLINGCNTRL  (0x10/sizeof(unsigned short))
#define WM8731_I2C_ACTIVECNTRL   (0x12/sizeof(unsigned short))

//Initialise Audio Controller
signed int WM8731_initialise ( unsigned int base_address ) {
    signed int status;
    //Set the local base address pointer
    wm8731_base_ptr = (unsigned int *) base_address;
    //Ensure I2C Controller "I2C1" is initialised
    if (!HPS_I2C_isInitialised(0)) {
        status = HPS_I2C_initialise(0);
        if (status != HPS_I2C_SUCCESS) return status;
    }
    //Initialise the WM8731 codec over I2C. See Page 46 of datasheet
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_POWERCNTRL   <<9) | 0x12); //Power-up chip. Leave mic off as not used.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_LEFTINCNTRL  <<9) | 0x17); //+4.5dB Volume. Unmute.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_RIGHTINCNTRL <<9) | 0x17); //+4.5dB Volume. Unmute.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_LEFTOUTCNTRL <<9) | 0x70); //-24dB Volume. Unmute.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_RIGHTOUTCNTRL<<9) | 0x70); //-24dB Volume. Unmute.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_ANLGPATHCNTRL<<9) | 0x12); //Use Line In. Disable Bypass. Use DAC
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_DGTLPATHCNTRL<<9) | 0x06); //Enable High-Pass filter. 48kHz sample rate.
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_DATAFMTCNTRL <<9) | 0x4E); //I2S Mode, 24bit, Master Mode (do not change this!)
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_SMPLINGCNTRL <<9) | 0x00); //Normal Mode, 48kHz sample rate
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_ACTIVECNTRL  <<9) | 0x01); //Enable Codec
    if (status != HPS_I2C_SUCCESS) return status;
    status = HPS_I2C_write16b(0, 0x1A, (WM8731_I2C_POWERCNTRL   <<9) | 0x02); //Power-up output.
    if (status != HPS_I2C_SUCCESS) return status;
    //Check if the base pointer is valid. This allows us to use the library to initialise the I2C side only.
    if (base_address == 0x0) return WM8731_ERRORNOINIT;
    //Mark as initialised so later functions know we are ready
    wm8731_initialised = true;
    //Clear the audio FIFOs
    return WM8731_clearFIFO(true,true);
}

//Check if driver initialised
bool WM8731_isInitialised() {
    return wm8731_initialised;
}

//Clears FIFOs
// - returns true if successful
signed int WM8731_clearFIFO( bool adc, bool dac) {
    unsigned int cntrl;
    if (!WM8731_isInitialised()) return WM8731_ERRORNOINIT; //not initialised
    //Read in current control value
    cntrl = wm8731_base_ptr[WM8731_CONTROL];
    //Calculate new value - with corresponding bits for clearing adc and/or dac FIFOs
    if (adc) {
        cntrl |= (1<<2);
    }
    if (dac) {
        cntrl |= (1<<3);
    }
    //Assert reset flags
    wm8731_base_ptr[WM8731_CONTROL] = cntrl;
    //Clear the flags
    if (adc) {
        cntrl &= ~(1<<2);
    }
    if (dac) {
        cntrl &= ~(1<<3);
    }
    //Then clear reset flags
    wm8731_base_ptr[WM8731_CONTROL] = cntrl;
    //And done.
    return WM8731_SUCCESS; //success
}

//Get FIFO Space Address
volatile unsigned char* WM8731_getFIFOSpacePtr( void ) {
    return (unsigned char*)&wm8731_base_ptr[WM8731_FIFOSPACE];
}

//Get Left FIFO Address
volatile unsigned int* WM8731_getLeftFIFOPtr( void ) {
    return &wm8731_base_ptr[WM8731_LEFTFIFO];
}

//Get Right FIFO Address
volatile unsigned int* WM8731_getRightFIFOPtr( void ) {
    return &wm8731_base_ptr[WM8731_RIGHTFIFO];
}

