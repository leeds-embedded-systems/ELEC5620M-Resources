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


#include "driver_crc.h"

/*
 * Main API
 */

// Calculate the CRC
//  - init controls whether to set the initial value to (*crc).
//     - In combined mode, this must always be true (in fact the parameter is ignored)
//     - in split mode, this controls whether the initialise function is called.
//  - CRC is calculated on length bytes from *data
//  - Result is returned to (*crc)
HpsErr_t CRC_calculate(PCRCCtx_t crcCtx, bool init, const uint8_t * data, unsigned int length, unsigned int* crc) {
    if (!crcCtx || !crc) return ERR_NULLPTR;
    // Check mode
    if (crcCtx->mode == CRC_FUNC_COMBINED) {
        // Call calculate function (or error if not set)
        if (!crcCtx->combined.calculate) return ERR_NOSUPPORT;
        return crcCtx->combined.calculate(crcCtx->ctx,data,length,crc);
    } else {
        // Initialise if required
        HpsErr_t status;
        if (init) {
            //ERR_NOSUPPORT from init is acceptable, it just means that the init value is fixed.
            if (crcCtx->split.initialise) {
                status = crcCtx->split.initialise(crcCtx->ctx, *crc);
                if ((status != ERR_NOSUPPORT) && ERR_IS_ERROR(status)) return status;
            }
        }
        // Ensure we have required function handles for calculation
        if (!crcCtx->split.calculate) return ERR_NOSUPPORT;
        if (!crcCtx->split.getResult) return ERR_NOSUPPORT;
        // And execute
        status = crcCtx->split.calculate(crcCtx->ctx, data, length, init);
        if (ERR_IS_ERROR(status)) return status;
        return crcCtx->split.getResult(crcCtx->ctx, crc);
    }
}

// Get the width of the CRC result in bits
//  - Maximum width is 32 for the generic CRC driver.
HpsErr_t CRC_getWidth(PCRCCtx_t crcCtx) {
    if (!crcCtx->getWidth) return ERR_NOSUPPORT;
    return crcCtx->getWidth(crcCtx->ctx);
}


