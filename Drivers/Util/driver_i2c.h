/* Generic I2C Driver Interface
 * -----------------------------
 *
 * Provides a common interface for different I2C
 * drivers to allow them to be used as a generic
 * handler.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 08/01/2024 | Creation of driver.
 */

#ifndef DRIVER_I2C_H_
#define DRIVER_I2C_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

typedef enum {
    I2C_SPEED_STANDARD = 100, //kHz
    I2C_SPEED_FASTMODE = 400  //kHz
} I2CSpeed;


// IO Function Templates
typedef HpsErr_t (*I2CWriteFunc_t)(void* ctx, unsigned short address, uint8_t writeData[], unsigned int writeLen);
typedef HpsErr_t (*I2CReadFunc_t )(void* ctx, unsigned short address, uint8_t writeData[], unsigned int writeLen, uint8_t readData[], unsigned int readLen);
typedef HpsErr_t    (*I2CAbortFunc_t)(void* ctx, bool isRead);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Driver Function Pointers
    I2CWriteFunc_t write;
    I2CReadFunc_t  read;
    I2CAbortFunc_t abort;
} I2CCtx_t, *PI2CCtx_t;

// Check if driver initialised
static inline bool I2C_isInitialised(PI2CCtx_t i2c) {
    if (!i2c) return false;
    return DriverContextCheckInit(i2c->ctx);
}

// Write data
// - Queues a non-blocking write of the specified length then returns ERR_AGAIN if in progress
// - Call I2C_write again with writeLen = 0 to check if complete.
//    - Returns ERR_AGAIN if still running
//    - Positive return value indicates amount of data written.
//    - Other error codes mean failure
static inline HpsErr_t I2C_write(PI2CCtx_t i2c, unsigned short address, uint8_t writeData[], unsigned int writeLen) {
    if (!i2c || !writeData) return ERR_NULLPTR;
    if (!i2c->write) return ERR_NOSUPPORT;
    return i2c->write(i2c->ctx,address,writeData,writeLen);
}

// Read data
// - Queues a non-blocking read of the specified length then returns ERR_AGAIN if in progress
// - Write Length bytes from write data will be sent before the restart flag (e.g. to send register address)
// - Read length bytes will be requested from the slave.
// - Read data may be null pointer, in cases where the actual value of the data being read is irrelevant.
// - Call I2C_read again with writeLen = 0 to check if complete.
//    - Returns ERR_AGAIN if still running
//    - Positive return value indicates amount of data written.
//    - Other error codes mean failure
static inline HpsErr_t I2C_read(PI2CCtx_t i2c, unsigned short address, uint8_t writeData[], unsigned int writeLen, uint8_t readData[], unsigned int readLen) {
    if (!i2c || !writeData) return ERR_NULLPTR;
    if (!i2c->read) return ERR_NOSUPPORT;
    return i2c->read(i2c->ctx,address,writeData,writeLen,readData,readLen);
}

// Abort read or write
// - Aborts read if 'isRead' true, otherwise aborts write.
static inline HpsErr_t I2C_abort(PI2CCtx_t i2c, bool isRead) {
    if (!i2c) return ERR_NULLPTR;
    if (!i2c->abort) return ERR_NOSUPPORT;
    return i2c->abort(i2c->ctx,isRead);
}

#endif /* DRIVER_I2C_H_ */
