/* Generic Flash Driver Interface
 * -------------------------------
 *
 * Provides a common interface for different Flash
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
 * 01/01/2024 | Creation of driver.
 */

#ifndef DRIVER_FLASH_H_
#define DRIVER_FLASH_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"
#include "Util/bit_helpers.h"


// IO Function Templates
typedef HpsErr_t (*FlashReadFunc_t  )(void* ctx, unsigned int address, unsigned int length, uint8_t* dest);
typedef HpsErr_t (*FlashEraseFunc_t )(void* ctx, unsigned int address, unsigned int length);
typedef HpsErr_t (*FlashWriteFunc_t )(void* ctx, unsigned int address, unsigned int length, const uint8_t* src);

// Flash Type
// - Add more types as they become required.
// - Ensure new types are **ALWAYS** added to the end of the num.
typedef enum {
    FLASH_TYPE_UNKNOWN,
    FLASH_TYPE_EPCQ,
    FLASH_TYPE_CFI
} FlashType;

// Flash Region
typedef struct {
    bool valid;
    unsigned int start;
    unsigned int end;
} FlashRegion_t, *PFlashRegion_t;

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Status returned from initialisation
    HpsErr_t initStatus;
    unsigned int wordSize;
    unsigned int blockSize;
    FlashType type;
    // Driver Function Pointers
    FlashReadFunc_t  read;
    FlashEraseFunc_t erase;
    FlashWriteFunc_t write;
    FlashWriteFunc_t verify;
} FlashCtx_t, *PFlashCtx_t;

// Check if driver initialised
static inline bool FLASH_isInitialised(PFlashCtx_t flash) {
    if (!flash) return false;
    return DriverContextCheckInit(flash->ctx);
}

// Returns true if the specified address range is within the specified region
bool FLASH_rangeInRegion(PFlashRegion_t region, unsigned int address, unsigned int length);

// Check if an address or length is aligned to the flash word size
static inline HpsErr_t FLASH_checkAlignment(PFlashCtx_t flash, unsigned int val) {
    if (!flash) return ERR_NULLPTR;
    return addressIsAligned(val, flash->wordSize) ? ERR_SUCCESS : ERR_ALIGNMENT;
}

// Align address or length to the flash word size
// - If to next, will round up. Else will round down.
static inline HpsErr_t FLASH_ensureAlignment(PFlashCtx_t flash, unsigned int *val, bool toNext) {
    if (!flash) return ERR_NULLPTR;
    *val = (unsigned int)alignPointer((void*)*val, flash->wordSize, toNext);
    return ERR_SUCCESS;
}

// Check the flash word size
static inline HpsErr_t FLASH_wordSize(PFlashCtx_t flash, unsigned int* wordSize) {
    if (!flash || !wordSize) return ERR_NULLPTR;
    *wordSize = flash->wordSize;
    return ERR_SUCCESS;
}

// Check the flash block size
static inline HpsErr_t FLASH_blockSize(PFlashCtx_t flash, unsigned int* blockSize) {
    if (!flash || !blockSize) return ERR_NULLPTR;
    *blockSize = flash->blockSize;
    return ERR_SUCCESS;
}

// Check the flash type
static inline HpsErr_t FLASH_type(PFlashCtx_t flash, FlashType* type) {
    if (!flash || !type) return ERR_NULLPTR;
    *type = flash->type;
    return ERR_SUCCESS;
}

// Read from the flash
static inline HpsErr_t FLASH_read(PFlashCtx_t flash, unsigned int address, unsigned int length, uint8_t* dest) {
    if (!flash) return ERR_NULLPTR;
    if (!flash->read) return ERR_NOSUPPORT;
    return flash->read(flash->ctx,address,length,dest);
}

// Erase a flash region
//  - On verify error, may return address of failed erase as error (if ERR_IS_SIGNMAGERR use FROM_SIGNMAG_ERR to extract)
static inline HpsErr_t FLASH_erase(PFlashCtx_t flash, unsigned int address, unsigned int length) {
    if (!flash) return ERR_NULLPTR;
    if (!flash->erase) return ERR_NOSUPPORT;
    return flash->erase(flash->ctx,address,length);
}

// Write to a flash region
static inline HpsErr_t FLASH_write(PFlashCtx_t flash, unsigned int address, unsigned int length, const uint8_t* src) {
    if (!flash) return ERR_NULLPTR;
    if (!flash->write) return ERR_NOSUPPORT;
    return flash->write(flash->ctx,address,length,src);
}

// Verify a flash region
//  - On verify error, may return address of failed verification as error (if ERR_IS_SIGNMAGERR use FROM_SIGNMAG_ERR to extract)
static inline HpsErr_t FLASH_verify(PFlashCtx_t flash, unsigned int address, unsigned int length, const uint8_t* src) {
    if (!flash) return ERR_NULLPTR;
    if (!flash->verify) return ERR_NOSUPPORT;
    return flash->verify(flash->ctx,address,length,src);
}

#endif /* DRIVER_FLASH_H_ */
