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

#include "Util/watchdog.h"

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

/*
 * crc16_compute() Compatibility for ICE Handle Comms
 */

PCRCCtx_t _crc16Proc = NULL;

// Set the CRC processor context used by the CRC16 routines.
// - If this returns an error code, the CRC16 functions will not work.
// - Passing a null pointer as crcCtx will clear the current handler.
//   This should be done if the crc driver being used as the current
//   handler is cleaned up.
HpsErr_t crc16_setCtx(PCRCCtx_t crcCtx) {
    _crc16Proc = NULL;
    if (!crcCtx) return ERR_SUCCESS;
    // Must be 16bit CRC
    HpsErr_t width = CRC_getWidth(crcCtx);
    if (width != 16) return ERR_IS_ERROR(width) ? (HpsErr_t)width : ERR_MISMATCH;
    // Save context and return status
    _crc16Proc = crcCtx;
    return ERR_SUCCESS;
}

// CRC16 calculation for data buffer
// - Length is in bytes
uint16_t crc16_compute(const uint32_t * data, uint32_t length) {
    uint32_t crc = 0;
    HpsErr_t status = CRC_calculate(_crc16Proc, true, (const uint8_t*)data, length, &crc);
    return (uint16_t)(ERR_IS_ERROR(status) ? -1 : crc);
}

/*
 * crc32() Compatibility for Image Header Decoding Function
 */

PCRCCtx_t _crc32Proc = NULL;

// Set the CRC processor context used by the CRC32 routines.
// - If this returns an error code, the CRC32 functions will not work.
// - Passing a null pointer as crcCtx will clear the current handler.
//   This should be done if the crc driver being used as the current
//   handler is cleaned up.
HpsErr_t crc32_setCtx(PCRCCtx_t crcCtx) {
    _crc32Proc = NULL;
    if (!crcCtx) return ERR_SUCCESS;
    // Must support initial value
    //  - Combined mode is guaranteed to.
    //  - Split mode we will check and see
    if (crcCtx->mode != CRC_FUNC_COMBINED) {
        if (!crcCtx->split.initialise) return ERR_NOSUPPORT;
        HpsErr_t status = crcCtx->split.initialise(crcCtx->ctx, 0);
        if (ERR_IS_ERROR(status)) return status;
    }
    // Must be 32bit CRC
    HpsErr_t width = CRC_getWidth(crcCtx);
    if (width != 32) return ERR_IS_ERROR(width) ? (HpsErr_t)width : ERR_MISMATCH;
    // Save context and return status
    _crc32Proc = crcCtx;
    return ERR_SUCCESS;
}

// Base CRC32 function
uint32_t crc32_base(uint32_t crc, const uint8_t *p, uint32_t len) {
    HpsErr_t status = CRC_calculate(_crc32Proc, true, p, len, &crc);
    return ERR_IS_ERROR(status) ? (uint32_t)-1 : crc;
}

// CRC32 calculation for small buffers
uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len) {
    return crc32_base(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
}

// CRC32 calculation for large buffers
// - Breaks the calculation into chunks interspersed with watchdog resets
uint32_t crc32_wd(uint32_t crc, const uint8_t *buf, uint32_t len, uint32_t chunk_sz) {
    ResetWDT();
    // Loop through the whole length
    while (len) {
        // How long is this chunk - the smaller of length and chunk size
        uint32_t chunkLen = len;
        if (chunkLen > chunk_sz) chunkLen = chunk_sz;
        // CRC this chunk
        crc = crc32(crc, buf, chunkLen);
        // Offset the buffer and length by how much we've processed
        buf += chunkLen;
        len -= chunkLen;
        // And pat the doggy
        ResetWDT();
    }
    return crc;
}


