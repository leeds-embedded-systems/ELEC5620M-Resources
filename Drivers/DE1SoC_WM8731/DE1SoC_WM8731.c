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

#include "DE1SoC_WM8731.h"
#include "Util/bit_helpers.h"
#include "Util/macros.h"

//WM8731 ARM Address Offsets
#define WM8731_CONTROL    (0x0/sizeof(unsigned int))
#define WM8731_FIFOSPACE  (0x4/sizeof(unsigned int))
#define WM8731_LEFTFIFO   (0x8/sizeof(unsigned int))
#define WM8731_RIGHTFIFO  (0xC/sizeof(unsigned int))

//Bits
#define WM8731_FIFO_RESET_ADC 2
#define WM8731_FIFO_RESET_DAC 3

//FIFO Offsets
#define WM8731_FIFO_RARC 0
#define WM8731_FIFO_RALC 8
#define WM8731_FIFO_WSRC 16
#define WM8731_FIFO_WSLC 24
#define WM8731_FIFO_MASK 0xFF

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

//Driver Cleanup
void _WM8731_cleanup(WM8731Ctx_t* ctx ) {
    if (ctx->base) {
        // Assert FIFO resets
        ctx->base[WM8731_CONTROL] |= ((1<<WM8731_FIFO_RESET_ADC) | (1<<WM8731_FIFO_RESET_DAC));
    }
    if (ctx->i2c) {
        // Power down the codec if we have an I2C device context
        HPS_I2C_write16b(ctx->i2c, ctx->i2cAddr, (WM8731_I2C_POWERCNTRL   <<9) | 0x00); // Power down outputs
    }
}

//Initialise Audio Codec
// - base_address is memory-mapped address of audio controller
// - returns 0 if successful
HpsErr_t WM8731_initialise( void* base, HPSI2CCtx_t* i2c, WM8731Ctx_t** pCtx ) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_WM8731_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    WM8731Ctx_t* ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->i2c = i2c;
    ctx->i2cAddr = 0x1A;
    // - For the time being this is hard-coded to 48kHz, but could be changed later.
    ctx->sampleRate = 48000;
    //Initialise the WM8731 codec over I2C. See Page 46 of datasheet.
    //I2C write is non-blocking & returns before complete.
    //Retry until complete by writing zero length data until successful.
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_POWERCNTRL   <<9) | 0x12); //Power-up chip. Leave mic off as not used.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); } //Retry until write successful.
    if (ERR_IS_ERROR(status)) return status; //Return if status is indicates an error.
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_LEFTINCNTRL  <<9) | 0x17); //+4.5dB Volume. Unmute.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_RIGHTINCNTRL <<9) | 0x17); //+4.5dB Volume. Unmute.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_LEFTOUTCNTRL <<9) | 0x70); //-24dB Volume. Unmute.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_RIGHTOUTCNTRL<<9) | 0x70); //-24dB Volume. Unmute.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_ANLGPATHCNTRL<<9) | 0x12); //Use Line In. Disable Bypass. Use DAC
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_DGTLPATHCNTRL<<9) | 0x06); //Enable High-Pass filter. 48kHz sample rate.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_DATAFMTCNTRL <<9) | 0x4E); //I2S Mode, 24bit, Master Mode (do not change this!)
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_SMPLINGCNTRL <<9) | 0x00); //Normal Mode, 48kHz sample rate
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_ACTIVECNTRL  <<9) | 0x01); //Enable Codec
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    status = HPS_I2C_write16b(ctx->i2c, 0x1A, (WM8731_I2C_POWERCNTRL   <<9) | 0x02); //Power-up output.
    while(ERR_IS_RETRY(status)){ status = HPS_I2C_write(ctx->i2c,0x1A,NULL,0); }
    if (ERR_IS_ERROR(status)) return status;
    //Initialised
    DriverContextSetInit(ctx);
    return WM8731_clearFIFO(ctx,true,true);
}

//Check if driver initialised
// - Returns true if driver previously initialised
// - WM8731_initialise() must be called if false.
bool WM8731_isInitialised( WM8731Ctx_t* ctx ) {
    return DriverContextCheckInit(ctx);
}

//Get the sample rate for the ADC/DAC
HpsErr_t WM8731_getSampleRate( WM8731Ctx_t* ctx, unsigned int* sampleRate ) {
	if (!sampleRate) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Return the sample rate.
	*sampleRate = ctx->sampleRate;
	return ERR_SUCCESS;
}

//Clears FIFOs
// - returns 0 if successful
HpsErr_t WM8731_clearFIFO( WM8731Ctx_t* ctx, bool adc, bool dac) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Assert reset flags by setting corresponding bits for clearing adc and/or dac FIFOs
    ctx->base[WM8731_CONTROL] |=  ((adc<<WM8731_FIFO_RESET_ADC) | (dac<<WM8731_FIFO_RESET_DAC));
    //Deassert again
    ctx->base[WM8731_CONTROL] &= ~((adc<<WM8731_FIFO_RESET_ADC) | (dac<<WM8731_FIFO_RESET_DAC));
    return ERR_SUCCESS;
}

//Get FIFO Space (DAC)
HpsErr_t WM8731_getFIFOSpace( WM8731Ctx_t* ctx, unsigned int* fifoSpace ) {
	if (!fifoSpace) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Get the FIFO fill register value
    unsigned int fill = ctx->base[WM8731_FIFOSPACE];
    //Space is the minimum space from either FIFO
    *fifoSpace = min(MaskExtract(fill, WM8731_FIFO_MASK, WM8731_FIFO_WSRC), MaskExtract(fill, WM8731_FIFO_MASK, WM8731_FIFO_WSLC));
    return ERR_SUCCESS;
}

//Get FIFO Fill (ADC)
HpsErr_t WM8731_getFIFOFill( WM8731Ctx_t* ctx, unsigned int* fifoFill ) {
	if (!fifoFill) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Get the FIFO fill register value
    unsigned int fill = ctx->base[WM8731_FIFOSPACE];
    //Space is the minimum space from either FIFO
    *fifoFill = min(MaskExtract(fill, WM8731_FIFO_MASK, WM8731_FIFO_RARC), MaskExtract(fill, WM8731_FIFO_MASK, WM8731_FIFO_RALC));
    return ERR_SUCCESS;
}


//Write a sample to the FIFO for the both channels
// - You must check there is data available in the FIFO before calling this function.
HpsErr_t WM8731_writeSample( WM8731Ctx_t* ctx, unsigned int left, unsigned int right) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
	//Write the sample
    ctx->base[WM8731_LEFTFIFO] = left;
    ctx->base[WM8731_RIGHTFIFO] = right;
    return ERR_SUCCESS;
}

//Read a sample from the FIFO for both channels
// - You must check there is space in the FIFO before calling this function.
HpsErr_t WM8731_readSample( WM8731Ctx_t* ctx, unsigned int* left, unsigned int* right ) {
	//Must provide place for returning values too.
	if (!left || !right) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
	//Write the sample
    *left = ctx->base[WM8731_LEFTFIFO];
    *right = ctx->base[WM8731_RIGHTFIFO];
    return ERR_SUCCESS;

}

