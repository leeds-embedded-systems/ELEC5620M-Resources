/*
 * Semi-Hosting Failback Handler
 * -----------------------------
 * 
 * When semi-hosting is enabled in the design for
 * use with the debugger, this library, if enabled
 * provides a set of failback routines to redirect
 * the semihosting interface to somewhere such as
 * a UART core.
 * 
 * To enable semihosting failback handler, you must
 * define the symbol SEMIHOST_FAILBACK_ENABLE which
 * will allow the semihosting handler and config
 * routines to be compiled in. You should also implement
 * the semihostWriteStr() and semihostReadStr() APIs
 * which should read and write from the desired
 * interface.
 * 
 * The checkIfSemihostingConnected() API allows tests
 * to see if a debugger is connected and semihosting
 * is enabled. This API is always available.
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+------------------------------------
 * 31/03/2024 | Creation of file
 *
 */


#ifndef SEMIHOSTING_H_
#define SEMIHOSTING_H_

#include "Util/error.h"

#include <stdint.h>
#include <stdbool.h>

// ARM defined semihosting operations
//  - Not all of these are implemented!
typedef enum {
    SEMIHOST_OP_SYS_OPEN     = 0x01,
    SEMIHOST_OP_SYS_CLOSE    = 0x02,
    SEMIHOST_OP_SYS_WRITEC   = 0x03,
    SEMIHOST_OP_SYS_WRITE0   = 0x04,
    SEMIHOST_OP_SYS_WRITE    = 0x05,
    SEMIHOST_OP_SYS_READ     = 0x06,
    SEMIHOST_OP_SYS_READC    = 0x07,
    SEMIHOST_OP_SYS_ISERROR  = 0x08,
    SEMIHOST_OP_SYS_ISTTY    = 0x09,
    SEMIHOST_OP_SYS_SEEK     = 0x0A,
    SEMIHOST_OP_SYS_FLEN     = 0x0C,
    SEMIHOST_OP_SYS_TMPNAM   = 0x0D,
    SEMIHOST_OP_SYS_REMOVE   = 0x0E,
    SEMIHOST_OP_SYS_RENAME   = 0x0F,
    SEMIHOST_OP_SYS_CLOCK    = 0x10,
    SEMIHOST_OP_SYS_TIME     = 0x11,
    SEMIHOST_OP_SYS_SYSTEM   = 0x12,
    SEMIHOST_OP_SYS_ERRNO    = 0x13,
    SEMIHOST_OP_SYS_GETCMDLN = 0x15,
    SEMIHOST_OP_SYS_HEAPINFO = 0x16,
    SEMIHOST_OP_SYS_EXIT     = 0x18,
    SEMIHOST_OP_SYS_ELAPSED  = 0x30,
    SEMIHOST_OP_SYS_TICKFREQ = 0x31
} SemihostOpIDs;

// Semihosting status codes used internally semihost handler
enum {
    SEMIHOST_SUCCESS = 0,
    SEMIHOST_ERROR   = -1
};

// Semihosting SVC ID. Depends on whether arm or thumb code
#define SVC_ID_SEMIHOST_ARM   0x123456
#define SVC_ID_SEMIHOST_THUMB 0xAB

// Function to check if semi-hosting is connected to a debugger.
//  - Returns either of these:
#define SVC_NO_DEBUGGER  false
#define SVC_HAS_DEBUGGER true
bool checkIfSemihostingConnected(void) __attribute__((used));

// Character buffer write handler
// - This must be implemented by the user if required (default implementation is no-op)
// - Should write 'len' characters from 'str' to UART or wherever so desired
// - if block, wait until character sent, otherwise may return early.
// - returns number of bytes not written.
HpsErr_t semihostWriteStr(const char* str, unsigned int len, bool block);

// Character buffer read handler
// - This must be implemented by the user if required (default implementation is no-op)
// - Should read 'len' characters from 'str' UART or wherever so desired
// - if block, wait until character available, otherwise may return early.
// - returns number of bytes not read.
HpsErr_t semihostReadStr(char* str, unsigned int len, bool block);

// System exit callback
// - Can be used to cleanup any driver instances used for read/write APIs.
void semihostExit(void);

#endif /* SEMIHOSTING_H_ */

