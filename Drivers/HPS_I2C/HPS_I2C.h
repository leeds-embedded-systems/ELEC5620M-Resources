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

#ifndef HPS_I2C_H_
#define HPS_I2C_H_

#include "Util/driver_i2c.h"
#include <stdbool.h>

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    // I2C common interface
    I2CCtx_t i2c;
    // Read/Write Status
    bool writeQueued;
    unsigned int writeLength;
    bool readQueued;
    unsigned int readLength;
} HPSI2CCtx_t, *PHPSI2CCtx_t;

//Initialise HPS I2C Controller
// - base is the base address of the I2C controller
// - speed is either standard or fastmode
// - Returns 0 if successful.
HpsErr_t HPS_I2C_initialise(void* base, I2CSpeed speed, PHPSI2CCtx_t* pCtx);

//Check if driver initialised
// - Returns true if driver context is initialised
bool HPS_I2C_isInitialised(PHPSI2CCtx_t ctx);

//Abort a pending read or write
// - Aborts read if isRead, otherwise aborts write
HpsErr_t HPS_I2C_abort(PHPSI2CCtx_t ctx, bool isRead);

//Functions to write data
// - 7bit address is I2C slave device address
// - data is data to be sent (8bit, 16bit, 32bit or array respectively)
// - Non-blocking. Write will be queued to FIFO and will then return.
//   - To check if complete, perform an array write with length 0.
//   - Returns ERR_AGAIN if not yet finished.
//   - Returns number of bytes written if successful.
HpsErrExt_t HPS_I2C_write8b (PHPSI2CCtx_t ctx, unsigned short address, unsigned char data);
HpsErrExt_t HPS_I2C_write16b(PHPSI2CCtx_t ctx, unsigned short address, unsigned short data);
HpsErrExt_t HPS_I2C_write32b(PHPSI2CCtx_t ctx, unsigned short address, unsigned int data);
HpsErrExt_t HPS_I2C_write   (PHPSI2CCtx_t ctx, unsigned short address, unsigned char data[], unsigned int length);

//Function to read data
// - 7bit address is I2C slave device address
// - When starting a read, writeLen bytes from writeData will be written before the restart (e.g. to send a register address)
// - Up to readLen bytes will be stored to readData[] once the read is completed.
// - Read length must be >= 1.
// - Non-blocking. Read will be queued to FIFO and will then return.
//   - To check if complete, perform an read with writeLen = 0.
//   - Returns ERR_AGAIN if not yet finished.
//   - Returns number of bytes written if successful.
HpsErr_t HPS_I2C_read(PHPSI2CCtx_t ctx, unsigned short address, unsigned char writeData[], unsigned int writeLen, unsigned char readData[], unsigned int readLen);

#endif /* HPS_I2C_H_ */
