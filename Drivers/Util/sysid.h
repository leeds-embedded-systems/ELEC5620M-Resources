/*
 * System ID Reader
 * ----------------
 *
 * Reads the timestamp and system ID from a Qsys
 * System ID peripheral.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 */

#ifndef SYSID_H_
#define SYSID_H_

#include <stdint.h>

#include "Util/ct_assert.h"

// System ID is just two 32bit registers.
typedef struct {
    uint32_t ident;
    uint32_t timestamp;
} SystemId_t;
ct_assert(SystemId_t, 2*sizeof(uint32_t));

// Ready the identity from a system ID peripheral
static inline unsigned int sysid_identity(void* sysIdBase) {
    if (!sysIdBase || !pointerIsAligned(sysIdBase, sizeof(SystemId_t))) return 0;
    volatile SystemId_t* sysId = (SystemId_t*)sysIdBase;
    return sysId->ident;
}

// Ready the timestamp from a system ID peripheral
static inline unsigned int sysid_timestamp(void* sysIdBase) {
    if (!sysIdBase || !pointerIsAligned(sysIdBase, sizeof(SystemId_t))) return 0;
    volatile SystemId_t* sysId = (SystemId_t*)sysIdBase;
    return sysId->timestamp;
}


#endif /* SYSID_H_ */
