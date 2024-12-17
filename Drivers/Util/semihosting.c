/*
 * Semi-Hosting Failback Handler
 * -----------------------------
 * 
 * When semi-hosting is enabled in the design for
 * use with the debugger, this library, if enabled
 * provides a set of failback routines to redirect
 * the semihosting interface to a UART core.
 * 
 * To enable semihosting failback handler, you must
 * define the symbol SEMIHOST_FAILBACK_ENABLE which
 * will allow the semihosting handler and config
 * routines to be compiled in.
 * 
 * The checkIfSemihostingConnected() API allows tests
 * to see if a debugger is connected and semihosting
 * is enabled. This API is always available.
 * 
 * Semihosting can be completely disabled by defining
 * the global symbol SEMIHOST_DISABLED
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

#include "semihosting.h"

#include <string.h>

/*
 * Semi-hosting Connected Check
 */

// We initially assume that semi-hosting is and connected. Before this is used
// we will trigger an SVC call to the semi-hosting ID. If the debugger is connected
// this SVC will be caught by it and not us. If it's not connected, our __svc_isr will
// be called which clears this flag. We can then return the flag value to show if
// connected or not.
__attribute__((used)) volatile unsigned int __semihostingEnabled = SVC_HAS_DEBUGGER;

// Function to check if semi-hosting is connected to a debugger.
bool checkIfSemihostingConnected(void) {
    __asm volatile (
        "MOV  R1, SP       ;"
        "MOVS R0, %[svcOp] ;"
        "SVC  %[svcId]     ;"
        :
#ifdef __thumb__
        : [svcId] "i" (SVC_ID_SEMIHOST_THUMB),
#else
        : [svcId] "i" (SVC_ID_SEMIHOST_ARM),
#endif
          [svcOp] "i" (SEMIHOST_OP_SYS_TIME)
        : "r0", "r1"
    );
    return (__semihostingEnabled ? SVC_HAS_DEBUGGER : SVC_NO_DEBUGGER);
}


// Default write handler
// - assumes everything was written.
HpsErr_t semihostWriteStr(const char* str, unsigned int len, bool block) __attribute__((weak));
HpsErr_t semihostWriteStr(const char* str, unsigned int len, bool block) {
    return 0;
}

// Default read handler
// - assumes everything was read
HpsErr_t semihostReadStr(char* str, unsigned int len, bool block) __attribute__((weak));
HpsErr_t semihostReadStr(char* str, unsigned int len, bool block) {
    memset(str, '\0', len);
    return 0;
}

// Default exit handler
// - does nothing.
void semihostExit(void) __attribute__((weak));
void semihostExit() {
    return;
}

#if defined(SEMIHOST_DISABLED)

#include "Util/irq.h"

// Disable semihosting
__asm(".global __use_no_semihosting\n\t");

void _sys_exit(int status) {
    IRQ_globalEnable(false);
    while (1);
}

void _ttywrch(int ch){
}

#elif defined(SEMIHOST_FAILBACK_ENABLE)

// Failback Semihosting Handler Routines
#include <errno.h>

typedef struct {
    unsigned int handle;
    char* str;
    unsigned int len;
} ReadParams_t;

typedef struct {
    unsigned int handle;
    const char* str;
    unsigned int len;
} WriteParams_t;

typedef struct {
    const char* str;
    unsigned int mode;
    unsigned int len;
} OpenParams_t;

enum {
    HANDLE_INVALID = -1,
    HANDLE_STDIN,
    HANDLE_STDOUT,
    HANDLE_STDERR
};
#define MAX_HANDLES (HANDLE_STDERR+1)
static bool _handleIsOpen [MAX_HANDLES] = {0};

signed int _semihost_isvalid(unsigned int handle, bool isOpen) {
    // Check that a valid handle
    if ((handle < 0) || (handle > MAX_HANDLES)) {
        errno = ERR_BADID;
        return SEMIHOST_ERROR;
    }
    // If requested, check if open
    if (!_handleIsOpen[handle]) {
        errno = ERR_NOTREADY;
        return SEMIHOST_ERROR;
    }
    // Success
    return SEMIHOST_SUCCESS;
}

signed int _semihost_write(unsigned int handle, const char* str, unsigned int len, bool block) {
    const char* escapeRed = "\x1b[31m";
    const char* escapeReset = "\x1b[0m";
    // Validate the handle is open and valid
    signed int status = _semihost_isvalid(handle, true);
    if (status == SEMIHOST_ERROR) return status;
    // Make sure handle is writable
    if (handle == HANDLE_STDIN) {
        errno = ERR_WRONGMODE;
        return len;
    }
    // Send ANSI escape code for red test if ERR handle
    bool escapeOpen = false;
    if (handle == HANDLE_STDERR) {
        escapeOpen = ERR_IS_SUCCESS(semihostWriteStr(escapeRed, sizeof(escapeRed)-1, block));
    }
    // Write string to user handler.
    HpsErr_t writeStat = semihostWriteStr(str, len, block);
    if (ERR_IS_SUCCESS(writeStat)) {
        len -= writeStat;
    }
    // Send ANSI reset escape code if ERR handle and we sent the open code.
    if (escapeOpen && (handle == HANDLE_STDERR)) {
        semihostWriteStr(escapeReset, sizeof(escapeReset)-1, block);
    }
    // Return number unsent.
    return len;
}

signed int _semihost_read(unsigned int handle, char* str, unsigned int len, bool block) {
    // Validate the handle is open and valid
    signed int status = _semihost_isvalid(handle, true);
    if (status == SEMIHOST_ERROR) return status;
    // Make sure handle is writable
    if (handle != HANDLE_STDIN) {
        errno = ERR_WRONGMODE;
        return len;
    }
    // Write string to user handler.
    HpsErr_t readStat = semihostReadStr(str, len, block);
    if (ERR_IS_SUCCESS(readStat)) {
        len -= readStat;
    }
    // Return number unsent.
    return len;
}

// Default SVC semi-host handler returns success
signed int __svc_semihost(unsigned int id, unsigned int* val) {
    signed int status = SEMIHOST_ERROR;
    // Ensure correct ID for semihosting call
    if ((id != SVC_ID_SEMIHOST_ARM) && (id != SVC_ID_SEMIHOST_THUMB)) return status;
    // Extract the op. This is in first user argument
    SemihostOpIDs opId = (SemihostOpIDs)val[0];
    // Decode the operation
    switch (opId) {
        case SEMIHOST_OP_SYS_OPEN: {
            // Get open parameters
            OpenParams_t* popen = (OpenParams_t*)val[1];
            status = HANDLE_INVALID;
            // Check for special TT streams (used for STDIN/OUT/ERR)
            if (!strncmp(popen->str,":tt",popen->len)) {
                // Check if open for read
                if (popen->mode <= 3) {
                    // Read, must be stdin
                    status = HANDLE_STDIN;
                } else {
                    // Write is either stdout or stderr. Assume they are opened in order.
                    if (!_handleIsOpen[HANDLE_STDOUT]) {
                        status = HANDLE_STDOUT;
                    } else if (!_handleIsOpen[HANDLE_STDERR]) {
                        status = HANDLE_STDERR;
                    } 
                }
            } // Otherwise handle is invalid.
            // Check if file is already open
            if (_handleIsOpen[status]) {
                // Can't open, already in use
                errno = ERR_INUSE;
                status = HANDLE_INVALID;
            } else {
                // Only support read/write mode. No special cases
                if ((popen->mode != 4) && (popen->mode != 0)) {
                    // Not write and not read, bad mode.
                    errno = ERR_WRONGMODE;
                    status = HANDLE_INVALID;
                } else {
                    // Handle is now open and ready for use
                    errno = ERR_SUCCESS;
                    _handleIsOpen[status] = true;
                    // Enable serial object
                    
                }
            }
            break;
        }
        case SEMIHOST_OP_SYS_CLOSE: {
            // Check the handle is valid (don't care whether open)
            unsigned int handle = val[1];
            status = _semihost_isvalid(handle, false);
            if (status != SEMIHOST_SUCCESS) break;
            // Mark the handle as closed
            _handleIsOpen[handle] = false;
            break;
        }
        case SEMIHOST_OP_SYS_WRITE0: {
            const char* str = (const char*)val[1]; // Null terminated string to write to STDOUT
            status = _semihost_write(HANDLE_STDOUT, str, strlen(str), true);
            break;
        }
        case SEMIHOST_OP_SYS_WRITEC: {
            const char* str = (const char*)&val[1]; // 1 character to write to STDOUT
            status = _semihost_write(HANDLE_STDOUT, str, 1, true);
            break;
        }
        case SEMIHOST_OP_SYS_WRITE: {
            WriteParams_t* pwrite = (WriteParams_t*)val[1]; // String with set length to write to handle
            status = _semihost_write(pwrite->handle, pwrite->str, pwrite->len, false);
            break;
        }
        case SEMIHOST_OP_SYS_READC: {
            char str; // 1 character to read from STDIN
            status = _semihost_read(HANDLE_STDIN, &str, 1, true);
            if (status == SEMIHOST_SUCCESS) {
                //Return character on success
                status = str;
            } else {
                status = SEMIHOST_ERROR;
            }
            break;
        }
        case SEMIHOST_OP_SYS_READ:{
            ReadParams_t* pread = (ReadParams_t*)val[1]; // Return buffer with set length to read to handle
            status = _semihost_read(pread->handle, pread->str, pread->len, false);
            break;
        }
        case SEMIHOST_OP_SYS_ISTTY:
            status = 1; // Always a console/TTY.
            break;
        case SEMIHOST_OP_SYS_SEEK:
            errno = ERR_NOSUPPORT;
            status = SEMIHOST_ERROR; // Can't seek, we're a TTY
            break;
        case SEMIHOST_OP_SYS_FLEN:
            status = INT32_MAX; // Can write as much as you like to UART.
            break;
        case SEMIHOST_OP_SYS_EXIT:
            semihostExit();
            status = SEMIHOST_SUCCESS;
            break;
        case SEMIHOST_OP_SYS_ERRNO:
            status = errno;
            break;
        case SEMIHOST_OP_SYS_ISERROR:
            status = (val[1] < 0);
            break;
        default:
            break;
    }
    // Return status value
    return status;
}

#endif

