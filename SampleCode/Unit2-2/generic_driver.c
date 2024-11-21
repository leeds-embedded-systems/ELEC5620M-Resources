/*Generic GPIO Driver*/
// IO Function Templates
typedef HpsErr_t (*GpioWriteFunc_t )(void* ctx, unsigned int out, unsigned int mask);
//...
// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Driver Function Pointers - Provides mapping to Hardware (Hw) driver
    GpioWriteFunc_t  setOutput;
    ...
} GpioCtx_t;
// APIs which user calls to automatically map to the hardware specific drivers
HpsErr_t GPIO_setOutput(GpioCtx_t* gpio, unsigned int out, unsigned int mask) {
    if (!gpio) return ERR_NULLPTR;
    if (!gpio->setOutput) return ERR_NOSUPPORT;
    return gpio->setOutput(gpio->ctx,out,mask);
}

/*FPGA PIO Driver*/
// Context includes its own copy of the generic driver structure:
typedef struct {
    ...
    GpioCtx_t gpio;
} FPGAPIOCtx_t;
// The mapping is set up in hardware specific initialise
HpsErr_t FPGA_PIO_initialise(void* base, ...) {
    ...
    //Populate the GPIO mapping structure
    ctx->gpio.ctx = ctx; //Will need our driver instance to pass into functions
    ctx->gpio.setOutput = (GpioWriteFunc_t)&FPGA_PIO_setOutput; //Hw specific API
    ...
}

/*Calling the HW specific API*/
//Initialise hardware driver instance as normal:
FPGAPIOCtx_t* fpgaCtx;
FPGA_PIO_initialise(..., &fpgaCtx);
//Can then get generic PIO context belonging to that hardware instance:
GpioCtx_t* genericCtx = &fpgaCtx->gpio;
//The generic API uses the generic GPIO structure meaning this:
GPIO_setOutput(genericCtx, out, mask);
//Is now equivalent to calling:
FPGA_PIO_setOutput(fpgaCtx, out, mask);
