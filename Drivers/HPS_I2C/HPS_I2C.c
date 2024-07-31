/*
 * HPS I2C Driver
 * ------------------------------
 *
 * Driver for the HPS embedded I2C controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-----------------------------------
 * 30/12/2023 | Switch to new driver context model
 *            | Add support for reading data
 *            | Change behaviour to non-blocking
 * 13/07/2019 | Support Controller ID 1 (LTC Hdr)
 * 20/10/2017 | Change to include status codes
 * 20/09/2017 | Creation of driver
 *
 */

#include "HPS_I2C.h"
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "Util/bit_helpers.h"

// Registers in I2C controller
#define HPS_I2C_CON	    (0x00/sizeof(unsigned int))
#define HPS_I2C_TAR     (0x04/sizeof(unsigned int))
#define HPS_I2C_DATCMD  (0x10/sizeof(unsigned int))
#define HPS_I2C_SSHCNT  (0x14/sizeof(unsigned int))
#define HPS_I2C_SSLCNT  (0x18/sizeof(unsigned int))
#define HPS_I2C_FSHCNT  (0x1C/sizeof(unsigned int))
#define HPS_I2C_FSLCNT  (0x20/sizeof(unsigned int))
#define HPS_I2C_IRQFLG  (0x2C/sizeof(unsigned int))
#define HPS_I2C_CLRRDRQ (0x50/sizeof(unsigned int))
#define HPS_I2C_CLRTXA  (0x54/sizeof(unsigned int))
#define HPS_I2C_CLRRXD  (0x58/sizeof(unsigned int))
#define HPS_I2C_ENABLE  (0x6C/sizeof(unsigned int))
#define HPS_I2C_STATUS  (0x70/sizeof(unsigned int))
#define HPS_I2C_TXFILL  (0x74/sizeof(unsigned int))
#define HPS_I2C_RXFILL  (0x78/sizeof(unsigned int))

// IRQ flags
#define HPS_I2C_IRQFLAG_ACTIVITY 8
#define HPS_I2C_IRQFLAG_RXDONE   7
#define HPS_I2C_IRQFLAG_TXABORT  6

// Status flags
#define HPS_I2C_STATUS_MASBUSY 5

// Control flags
#define HPS_I2C_CONTROL_MASTER       0
#define HPS_I2C_CONTROL_SPEED        1
#define HPS_I2C_CONTROL_SLV10BITONLY 3
#define HPS_I2C_CONTROL_MAS10BITCAPS 4
#define HPS_I2C_CONTROL_RESTARTEN    5
#define HPS_I2C_CONTROL_SLVDISBL     6
#define HPS_I2C_CONTROL_STOPIFADDR   7
#define HPS_I2C_CONTROL_TXEMPTYIRQEN 8

#define HPS_I2C_CONTROL_SPEED_MASK 0x3U

// Control speed settings
enum {
    HPS_I2C_SPEED_SLOW = 1,
    HPS_I2C_SPEED_FAST = 2
};

// Enable flags
#define HPS_I2C_ENABLE_I2CEN 0
#define HPS_I2C_ENABLE_ABORT 1

// Data/Command flags
#define HPS_I2C_DATACMD_RESTART 10
#define HPS_I2C_DATACMD_STOP     9
#define HPS_I2C_DATACMD_READ     8

/*
 * Internal Functions
 */

//Check if write complete
static HpsErr_t _HPS_I2C_writeCheckResult(PHPSI2CCtx_t ctx) {
    //Check if there is a write queued
    if (!ctx->writeQueued) {
        return ERR_NOTFOUND; //Nothing running
    }
    //Check for a TX abort IRQ
    if (ctx->base[HPS_I2C_IRQFLG] & _BV(HPS_I2C_IRQFLAG_TXABORT)){
        ctx->base[HPS_I2C_CLRTXA] = 1; //Clear TX abort flag.
        ctx->writeQueued = false;
        return ERR_ABORTED; //Aborted if any of the abort flags are set.
    }
    //Check if master to finished
    if (ctx->base[HPS_I2C_STATUS] & _BV(HPS_I2C_STATUS_MASBUSY)) {
        return ERR_AGAIN;
    }
    //And done.
    ctx->writeQueued = false;
    return ctx->writeLength;
}

static HpsErr_t _HPS_I2C_readCheckResult(PHPSI2CCtx_t ctx, unsigned char data[]) {
    //Check if there is a read queued
    if (!ctx->readQueued) {
        return ERR_NOTFOUND; //Nothing running
    }
    //Check for a TX abort IRQ
    if (ctx->base[HPS_I2C_IRQFLG] & _BV(HPS_I2C_IRQFLAG_TXABORT)){
        (ctx->base[HPS_I2C_CLRTXA]); //Clear TX abort flag.
        ctx->writeQueued = false;
        return ERR_ABORTED; //Aborted if any of the abort flags are set.
    }
    //Check if master to finished
    if (ctx->base[HPS_I2C_STATUS] & _BV(HPS_I2C_STATUS_MASBUSY)) {
        return ERR_AGAIN;
    }
    //Read from the RX FIFO
    HpsErr_t fifoFill = ctx->base[HPS_I2C_RXFILL];
    unsigned int readLen = ctx->readLength;
    for (unsigned int rxIdx = 0; rxIdx < fifoFill; rxIdx++) {
        //Read the word
        unsigned char rx = (unsigned char)ctx->base[HPS_I2C_DATCMD];
        //If within our data length, save it
        if ((rxIdx < readLen) && data) data[rxIdx] = rx;
    }
    //Pin the returned amount read to readLen if it was over (should never be, just worth cleaning out the whole FIFO).
    if (fifoFill < readLen) fifoFill = readLen;
    //Clear the receive complete IRQ
    (ctx->base[HPS_I2C_CLRRXD]);
    //And done.
    ctx->readQueued = false;
    return fifoFill;
}

static void _HPS_I2C_cleanup(PHPSI2CCtx_t ctx) {
    //Disable the I2C controller.
    if (ctx->base) {
        ctx->base[HPS_I2C_ENABLE] = 0x0;
    }
}

/*
 * User Facing APIs
 */

//Initialise HPS I2C Controller
// - For base, DE1-SoC uses 0xFFC04000 for Accelerometer/VGA/Audio/ADC. 0xFFC05000 for LTC 14pin Hdr.
// - Returns 0 if successful.
HpsErr_t HPS_I2C_initialise(void* base, I2CSpeed speed, PHPSI2CCtx_t* pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_HPS_I2C_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    PHPSI2CCtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    //Initialise I2C common context
    ctx->i2c.ctx = ctx;
    ctx->i2c.read = (I2CReadFunc_t)&HPS_I2C_read;
    ctx->i2c.write = (I2CWriteFunc_t)&HPS_I2C_write;
    ctx->i2c.abort = (I2CAbortFunc_t)&HPS_I2C_abort;
    //Ensure I2C disabled for configuration
    ctx->base[HPS_I2C_ENABLE] = 0x00;
    //Configure the I2C peripheral
    //See "Hard Processor System Technical Reference Manual" section 20 for magic calculations of HCNT/LCNT
    if (speed == I2C_SPEED_FASTMODE) {
        ctx->base[HPS_I2C_CON   ] = _BV(HPS_I2C_CONTROL_SLVDISBL) | _BV(HPS_I2C_CONTROL_RESTARTEN) | (HPS_I2C_SPEED_FAST << HPS_I2C_CONTROL_SPEED) | _BV(HPS_I2C_CONTROL_MASTER);  //I2C master mode, 400kHz, 7-bit address
        ctx->base[HPS_I2C_FSHCNT] = 0x3C;  //I2C clock high parameter for 400kHz
        ctx->base[HPS_I2C_FSLCNT] = 0x82;  //I2C clock low parameter for 400kHz
    } else {
        ctx->base[HPS_I2C_CON   ] = _BV(HPS_I2C_CONTROL_SLVDISBL) | _BV(HPS_I2C_CONTROL_RESTARTEN) | (HPS_I2C_SPEED_SLOW << HPS_I2C_CONTROL_SPEED) | _BV(HPS_I2C_CONTROL_MASTER);  //I2C master mode, 100kHz, 7-bit address
        ctx->base[HPS_I2C_SSHCNT] = 0x190; //I2C clock high parameter for 100kHz
        ctx->base[HPS_I2C_SSLCNT] = 0x1D6; //I2C clock low parameter for 100kHz
    }
    //Enable the I2C peripheral
    ctx->base[HPS_I2C_ENABLE] = _BV(HPS_I2C_ENABLE_I2CEN);    //I2C enabled
    //And initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

//Check if driver initialised
// - Returns true if driver context is initialised
bool HPS_I2C_isInitialised(PHPSI2CCtx_t ctx){
    return DriverContextCheckInit(ctx);
}

//Abort a pending read or write
// - Aborts read if isRead, otherwise aborts write
HpsErr_t HPS_I2C_abort(PHPSI2CCtx_t ctx, bool isRead) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if anything to abort
    if (isRead && !ctx->readQueued) return ctx->writeQueued ? ERR_BUSY : ERR_NOTFOUND;
    else if (!isRead && !ctx->writeQueued) return ctx->readQueued ? ERR_BUSY : ERR_NOTFOUND;
    //Abort the current transfer
    ctx->base[HPS_I2C_ENABLE] = _BV(HPS_I2C_ENABLE_ABORT) | _BV(HPS_I2C_ENABLE_I2CEN);
    //Wait until the abort flag clears
    while(ctx->base[HPS_I2C_ENABLE] & _BV(HPS_I2C_ENABLE_ABORT));
    //Abort complete
    ctx->readQueued = false;
    ctx->writeQueued = false;
    return ERR_SUCCESS;
}

//Functions to write data
// - 7bit address is I2C slave device address
// - data is data to be sent (8bit, 16bit, 32bit or array respectively)
// - Non-blocking. Write will be queued to FIFO and will then return.
//   - To check if complete, perform an array write with length 0.
//   - Returns ERR_AGAIN if not yet finished.
//   - Returns number of bytes written if successful.
HpsErr_t HPS_I2C_write8b(PHPSI2CCtx_t ctx, unsigned short address, unsigned char data) {
    return HPS_I2C_write(ctx, address, &data, 1);
}
HpsErr_t HPS_I2C_write16b(PHPSI2CCtx_t ctx, unsigned short address, unsigned short data) {
    //Remap data to big-endian
    data = reverseShort(data);
    //Send data as array
    return HPS_I2C_write(ctx, address, (unsigned char*)&data, sizeof(data));
}
HpsErr_t HPS_I2C_write32b(PHPSI2CCtx_t ctx, unsigned short address, unsigned int data) {
    //Remap data to big-endian
    data = reverseInt(data);
    //Send data as array
    return HPS_I2C_write(ctx, address, (unsigned char*)&data, sizeof(data));
}
HpsErr_t HPS_I2C_write(PHPSI2CCtx_t ctx, unsigned short address, unsigned char data[], unsigned int length) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Length of 0 means check status
    if (!length) {
        return _HPS_I2C_writeCheckResult(ctx);
    }
    //Check if busy
    if (ctx->writeQueued || ctx->readQueued) return ERR_BUSY;
    if (ctx->base[HPS_I2C_STATUS] & _BV(HPS_I2C_STATUS_MASBUSY)) return ERR_BUSY; //I2C master busy
    //Ensure arguments are valid
    if (!data) return ERR_NULLPTR;   //Empty array
    //Validate that there is space for the write request
    unsigned int txFifoSpace = (64 - ctx->base[HPS_I2C_TXFILL]);
    if (length > txFifoSpace) return ERR_NOSPACE; //Transfer too long. FIFO has space for 64 data words. Could add throttling later.
    //If a valid controller and the master is not currently busy, perform our transfer
    ctx->base[HPS_I2C_TAR] = address; //Load the target address as 7bit address in master mode
    //Write is queued
    ctx->writeQueued = true;
    ctx->writeLength = length;
    //Load the transmit data into the TX FIFO
    while (length--) {
        //Load the next data byte into our data/command packet
        unsigned int datcmd = (unsigned int)*data++;
        //Must set the stop bit in the last word
        if (!length) datcmd |= _BV(HPS_I2C_DATACMD_STOP); //Set the "stop" bit of the datcmd register so controller issues end of I2C transaction
        //And send it
        ctx->base[HPS_I2C_DATCMD] = datcmd;
    };
    //Check if finished
    return _HPS_I2C_writeCheckResult(ctx);
}

//Function to read data
// - 7bit address is I2C slave device address
// - When starting a read, writeLen bytes from writeData will be written before the restart (e.g. to send a register address)
// - Up to readLen bytes will be stored to readData[] once the read is completed.
// - Read length must be >= 1.
// - Non-blocking. Read will be queued to FIFO and will then return.
//   - To check if complete, perform an read with writeLen = 0.
//   - Returns ERR_AGAIN if not yet finished.
//   - Returns number of bytes written if successful.
HpsErr_t HPS_I2C_read(PHPSI2CCtx_t ctx, unsigned short address, unsigned char writeData[], unsigned int writeLen, unsigned char readData[], unsigned int readLen) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Length of 0 means check status
    if (!writeLen) {
        return _HPS_I2C_readCheckResult(ctx, readData);
    }
    //Check if busy
    if (ctx->writeQueued || ctx->readQueued) return ERR_BUSY;
    if (ctx->base[HPS_I2C_STATUS] & _BV(HPS_I2C_STATUS_MASBUSY)) return ERR_BUSY; //I2C master busy
    //Ensure arguments are valid
    if (!writeData) return ERR_NULLPTR; //Empty array
    //Validate there is space for the read request
    if (!readLen) return ERR_TOOSMALL; //Transfer too short. Must have a read length of at least 1
    unsigned int txFifoSpace = (64 - ctx->base[HPS_I2C_TXFILL]);
    unsigned int rxFifoSpace = (64 - ctx->base[HPS_I2C_RXFILL]);
    if ((writeLen + readLen) > txFifoSpace) return ERR_NOSPACE; //Transfer too long. FIFO has space for 64 data/cmd words. Could add throttling later.
    if (readLen > rxFifoSpace) return ERR_NOSPACE;              //Read length is too big for the space in the read FIFO
    //If a valid controller and the master is not currently busy, perform our transfer
    ctx->base[HPS_I2C_TAR] = address; //Load the target address as 7bit address in master mode
    //read is queued
    ctx->readQueued = true;
    ctx->readLength = readLen;
    //Ensure read request/complete flags are clear
    (ctx->base[HPS_I2C_CLRRDRQ]);
    (ctx->base[HPS_I2C_CLRRXD]);
    //Start the read by writing the register address to the TX FIFO
    while (writeLen--) {
        //Load the next data byte into our data/command packet
        ctx->base[HPS_I2C_DATCMD] = (unsigned int)*writeData++;
    }
    //Now issue the first read, writing to the FIFO with the restart flag asserted and read asserted
    unsigned int datcmd = _BV(HPS_I2C_DATACMD_RESTART) | _BV(HPS_I2C_DATACMD_READ);
    while (readLen--) {
        //Must set the stop bit in the last word
        if (!readLen) datcmd |= _BV(HPS_I2C_DATACMD_STOP); //Set the "stop" bit of the datcmd register so controller issues end of I2C transaction
        //Send the word
        ctx->base[HPS_I2C_DATCMD] = datcmd;
        //Just need read flag for subsequent words
        datcmd = _BV(HPS_I2C_DATACMD_READ);
    }
    //And done. Check if we succeeded
    return _HPS_I2C_readCheckResult(ctx, readData);
}
