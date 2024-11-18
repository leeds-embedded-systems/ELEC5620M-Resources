/* Generic CRC Driver Interface
 * -----------------------------
 *
 * Provides a common interface for different CRC
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
 * 03/01/2024 | Creation of driver.
 */

#ifndef DRIVER_CRC_H_
#define DRIVER_CRC_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

// IO Function Templates.
typedef HpsErr_t (*CRCGetWidth_t)(void* ctx);
// There are two modes:
//  - Mode 1 - Combined:
//      - A single function initialises with (*crc), calculates, then returns the result into (*crc)
typedef HpsErr_t (*CRCCalcCombinedFunc_t)(void* ctx, const uint8_t * data, unsigned int length, unsigned int* crc);
//  - Mode 2 - Split:
//      - A function that sets the custom initialisation value
//          - Must return ERR_NOSUPPORT if custom values are not supported
//      - A function that calculates the CRC, with choice to initialise or chain calculations
//      - A function returns the result
typedef HpsErr_t (*CRCInitialiseFunc_t  )(void* ctx, unsigned int initVal);
typedef HpsErr_t (*CRCCalculateFunc_t   )(void* ctx, const uint8_t * data, unsigned int length, bool reset);
typedef HpsErr_t (*CRCResultFunc_t      )(void* ctx, unsigned int* res);

// Which mode
typedef enum {
    CRC_FUNC_COMBINED,
    CRC_FUNC_SPLIT
} CRCFuncMode;

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Mode
    CRCFuncMode mode;
    // Driver Function Pointers
    CRCGetWidth_t getWidth;
    struct {
        CRCCalcCombinedFunc_t calculate;
    } combined;
    struct {
        CRCInitialiseFunc_t   initialise;
        CRCCalculateFunc_t    calculate;
        CRCResultFunc_t       getResult;
    } split;
} CRCCtx_t;

// Check if driver initialised
static inline bool CRC_isInitialised(CRCCtx_t* crc) {
    if (!crc) return false;
    return DriverContextCheckInit(crc->ctx);
}

// Calculate the CRC
//  - init controls whether to set the initial value to (*crc).
//     - In combined mode, this must always be true (in fact the parameter is ignored)
//     - in split mode, this controls whether the initialise function is called.
//  - CRC is calculated on length bytes from *data
//  - Result is returned to (*crc)
HpsErr_t CRC_calculate(CRCCtx_t* crcCtx, bool init, const uint8_t * data, unsigned int length, unsigned int* crc);

// Get the width of the CRC result in bits
//  - Maximum width is 32 for the generic CRC driver.
HpsErr_t CRC_getWidth(CRCCtx_t* crcCtx);


#endif /* DRIVER_CRC_H_ */
