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

//I2C Register Format
#define WM8731_I2C_REGADDR_MASK  0x7FU
#define WM8731_I2C_REGADDR_OFFS  9
#define WM8731_I2C_REGDATA_MASK  0x1FFU
#define WM8731_I2C_REGDATA_OFFS  0

#define WM8731_I2C_ADDRESS 0x1A

//I2C Register Write
static HpsErr_t _WM8731_writeRegister(WM8731Ctx_t* ctx, WM8731RegAddress regAddr, unsigned int regVal) {
    //Build write data value
    uint16_t writeData = MaskInsert(regAddr, WM8731_I2C_REGADDR_MASK, WM8731_I2C_REGADDR_OFFS) | 
                         MaskInsert(regVal,  WM8731_I2C_REGDATA_MASK, WM8731_I2C_REGDATA_OFFS);
    //Remap data to big-endian
    writeData = reverseShort(writeData);
    //Send data as array
    HpsErr_t status = I2C_write(ctx->i2c, ctx->i2cAddr, (uint8_t*)&writeData, sizeof(writeData));
    while(ERR_IS_RETRY(status)) {
        //If ERR_IS_RETRY for I2C_write, then means still in progress. 
        //Check result by passing len=0, and keep going until complete.
        status = I2C_write(ctx->i2c, ctx->i2cAddr, NULL, 0);
    }
    return status;
}


//Driver Cleanup
static void _WM8731_cleanup(WM8731Ctx_t* ctx) {
    if (ctx->base) {
        // Assert FIFO resets
        ctx->base[WM8731_CONTROL] |= ((1<<WM8731_FIFO_RESET_ADC) | (1<<WM8731_FIFO_RESET_DAC));
    }
    if (ctx->i2c) {
        // Power down the codec if we have an I2C device context
        _WM8731_writeRegister(ctx, WM8731_REG_POWERCNTRL, 0x00); // Power down outputs
    }
}

//Initialise Audio Codec
// - base_address is memory-mapped address of audio controller
// - returns 0 if successful
HpsErr_t WM8731_initialise( void* base, I2CCtx_t* i2c, WM8731Ctx_t** pCtx ) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    if (!I2C_isInitialised(i2c)) return ERR_BADDEVICE;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_WM8731_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    WM8731Ctx_t* ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->i2c = i2c;
    ctx->i2cAddr = WM8731_I2C_ADDRESS;
    // - For the time being this is hard-coded to 48kHz, but could be changed later.
    ctx->sampleRate = 48000;
    //Initialise the WM8731 codec over I2C. See Page 46 of datasheet.
    status = _WM8731_writeRegister(ctx, WM8731_REG_POWERCNTRL,    0x12); //Power-up chip. Leave mic off as not used.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_LEFTINCNTRL,   0x17); //+4.5dB Volume. Unmute.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_RIGHTINCNTRL,  0x17); //+4.5dB Volume. Unmute.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_LEFTOUTCNTRL,  0x70); //-24dB Volume. Unmute.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_RIGHTOUTCNTRL, 0x70); //-24dB Volume. Unmute.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_ANLGPATHCNTRL, 0x12); //Use Line In. Disable Bypass. Use DAC
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_DGTLPATHCNTRL, 0x06); //Enable High-Pass filter. 48kHz sample rate.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_DATAFMTCNTRL,  0x4E); //I2S Mode, 24bit, Master Mode (do not change this!)
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_SMPLINGCNTRL,  0x00); //Normal Mode, 48kHz sample rate
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_ACTIVECNTRL,   0x01); //Enable Codec
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    status = _WM8731_writeRegister(ctx, WM8731_REG_POWERCNTRL,    0x02); //Power-up output.
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
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

//Configure an I2C register
// - Allows changing settings for codec (e.g. volumen control, filtering, etc)
// - Refer to Page 46 of WM8731 codec datasheet.
HpsErr_t WM8731_writeRegister( WM8731Ctx_t* ctx, WM8731RegAddress regAddr, unsigned int regVal ) {
    //Can't modify format register as this is fixed in DPGA hardware
    if (regAddr == WM8731_REG_DATAFMTCNTRL) return ERR_WRITEPROT;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Modify register
    return _WM8731_writeRegister(ctx, regAddr, regVal);
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

