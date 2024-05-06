/*
 * HPS DMA Controller Driver Enums
 * -------------------------------
 *
 * Common enumerations for the HPS DMA Controller
 * driver.
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 04/05/2024 | Creation of header.
 *
 */

#ifndef HPS_DMACONTROLLERENUMS_H_
#define HPS_DMACONTROLLERENUMS_H_

#include "Util/bit_helpers.h"
#include "Util/macros.h"

typedef enum {
    HPS_DMA_BURSTSIZE_1BYTE  = 0,
    HPS_DMA_BURSTSIZE_2BYTE  = 1,
    HPS_DMA_BURSTSIZE_4BYTE  = 2,
    HPS_DMA_BURSTSIZE_8BYTE  = 3,
    HPS_DMA_BURSTSIZE_16BYTE = 4
} HPSDmaBurstSize;

#define HPS_DMA_BURSTLEN_MAX 16

typedef enum {
    HPS_DMA_ENDIAN_NOSWAP   = 0, // Do not swap endianness
    HPS_DMA_ENDIAN_SWAP16b  = 1, // Swap endianness of 16bit data words
    HPS_DMA_ENDIAN_SWAP32b  = 2, //             ... of 32bit ...
    HPS_DMA_ENDIAN_SWAP64b  = 3, //             ... of 64bit ...
    HPS_DMA_ENDIAN_SWAP128b = 4  //             ... of 128bit ...
} HPSDmaEndianSwap;

// Data Source flags
typedef enum {
    HPS_DMA_SOURCE_MEMORY   = 0, // Data is read from a memory region (incrementing address)
    HPS_DMA_SOURCE_REGISTER = 1, // Data is read from a register (non-incrementing address)
    HPS_DMA_SOURCE_PERIPH   = 2, // Data is written to a peripheral (custom, not yet supported)
    HPS_DMA_SOURCE_ZERO     = 3  // Data will be just zeros.
} HPSDmaDataSource;

// Data Destination flags
typedef enum {
    HPS_DMA_DESTINATION_MEMORY   = 0, // Data is written to a memory region (incrementing address)
    HPS_DMA_DESTINATION_REGISTER = 1, // Data is written to a register (non-incrementing address)
    HPS_DMA_DESTINATION_PERIPH   = 2  // Data is written to a peripheral (custom, not yet supported)
} HPSDmaDataDest;

// If unsure, use default Protection and Cacheable settings.
typedef enum {
    HPS_DMA_PROT_DAT_SEC_NPVLG = 0,                        // Data Access,        Secure,     Unprivileged
    HPS_DMA_PROT_DAT_SEC_PVLGD =                   _BV(0), // Data Access,        Secure,     Privileged
    HPS_DMA_PROT_DAT_NSC_NPVLG =          _BV(1)         , // Data Access,        Non-Secure, Unprivileged
    HPS_DMA_PROT_DAT_NSC_PVLGD =          _BV(1) | _BV(0), // Data Access,        Non-Secure, Privileged
    HPS_DMA_PROT_INS_SEC_NPVLG = _BV(2)                  , // Instruction Access, Secure,     Unprivileged
    HPS_DMA_PROT_INS_SEC_PVLGD = _BV(2)          | _BV(0), // Instruction Access, Secure,     Privileged
    HPS_DMA_PROT_INS_NSC_NPVLG = _BV(2) | _BV(1)         , // Instruction Access, Non-Secure, Unprivileged
    HPS_DMA_PROT_INS_NSC_PVLGD = _BV(2) | _BV(1) | _BV(0), // Instruction Access, Non-Secure, Privileged
    // Default protection setting:
    HPS_DMA_PROT_DEFAULT       = HPS_DMA_PROT_DAT_SEC_NPVLG
} HPDDmaProtection;

typedef enum {
    HPS_DMA_CACHE_NALLC_NCACH_NBF = 0,                        // Non-Allocatable, Non-Cachable, Non-Bufferable
    HPS_DMA_CACHE_NALLC_NCACH_BUF =                   _BV(0), // Non-Allocatable, Non-Cachable, Bufferable
    HPS_DMA_CACHE_NALLC_CACHE_NBF =          _BV(1)         , // Non-Allocatable, Cachable,     Non-Bufferable
    HPS_DMA_CACHE_NALLC_CACHE_BUF =          _BV(1) | _BV(0), // Non-Allocatable, Cachable,     Bufferable
    HPS_DMA_CACHE_ALLOC_NCACH_NBF = _BV(2)                  , // Allocatable,     Non-Cachable, Non-Bufferable
    HPS_DMA_CACHE_ALLOC_NCACH_BUF = _BV(2)          | _BV(0), // Allocatable,     Non-Cachable, Bufferable
    HPS_DMA_CACHE_ALLOC_CACHE_NBF = _BV(2) | _BV(1)         , // Allocatable,     Cachable,     Non-Bufferable
    HPS_DMA_CACHE_ALLOC_CACHE_BUF = _BV(2) | _BV(1) | _BV(0), // Allocatable,     Cachable,     Bufferable
    // Default memory access setting:
    HPS_DMA_CACHE_DEFAULT_MEM     = HPS_DMA_CACHE_ALLOC_CACHE_BUF,
    // Default register or zero access setting:
    HPS_DMA_CACHE_DEFAULT_REG     = HPS_DMA_CACHE_NALLC_NCACH_NBF
} HPSDmaCacheable;

typedef enum {
    HPS_DMA_THREADTYPE_MGR = 0,
    HPS_DMA_THREADTYPE_CH  = 1
} HPSDmaThreadType;

typedef enum {
    HPS_DMA_CHANNEL_MGR   = -1, // Placeholder used for APIs where thread type is set as MGR.
    HPS_DMA_CHANNEL_USER0 = 0,
    HPS_DMA_CHANNEL_USER1,
    HPS_DMA_CHANNEL_USER2,
    HPS_DMA_CHANNEL_USER3,
    HPS_DMA_CHANNEL_USER4,
    HPS_DMA_CHANNEL_USER5,
    HPS_DMA_CHANNEL_USER6,
    HPS_DMA_CHANNEL_USER7
} HPSDmaChannelId;
#define HPS_DMA_CHANNEL_MIN   (HPS_DMA_CHANNEL_USER0)
#define HPS_DMA_CHANNEL_COUNT (HPS_DMA_CHANNEL_USER7 + 1)

typedef enum {
    HPS_DMA_EVENT_USER0 = 0,
    HPS_DMA_EVENT_USER1,
    HPS_DMA_EVENT_USER2,
    HPS_DMA_EVENT_USER3,
    HPS_DMA_EVENT_USER4,
    HPS_DMA_EVENT_USER5,
    HPS_DMA_EVENT_USER6,
    HPS_DMA_EVENT_USER7,
    HPS_DMA_EVENT_ABORTED = 8
} HPSDmaEventId;
#define HPS_DMA_EVENT_MIN        (HPS_DMA_EVENT_USER0)
#define HPS_DMA_USER_EVENT_COUNT (HPS_DMA_EVENT_ABORTED)
#define HPS_DMA_EVENT_COUNT      (HPS_DMA_EVENT_ABORTED + 1)

typedef enum {
    HPS_DMA_MGRFAULT_UNDEFINS = _BV(0 ), // Undefined instruction in DMA manager command queue
    HPS_DMA_MGRFAULT_INVLDOP  = _BV(1 ), // Invalid operation for current state in DMA manager command queue
    HPS_DMA_MGRFAULT_DMAGOERR = _BV(4 ), // Manager tried to execute DMAGO without correct security permissions
    HPS_DMA_MGRFAULT_EVENTERR = _BV(5 ), // Manager tried to execute DMAWFE/DMASEV without correct security permissions
    HPS_DMA_MGRFAULT_FETCHINS = _BV(16), // Whether an error occurred while fetching DMA manager instruction
    HPS_DMA_MGRFAULT_DEBUGINS = _BV(30)  // Whether instruction error was generated from debug interface or system mem
} HPSDmaMgrFault;

typedef enum {
    HPS_DMA_CHFAULT_UNDEFINS  = _BV(0 ), // Undefined instruction in DMA channel command queue
    HPS_DMA_CHFAULT_INVLDOP   = _BV(1 ), // Invalid operation for current state in DMA channel command queue
    HPS_DMA_CHFAULT_EVENTERR  = _BV(5 ), // Channel tried to execute DMAWFE/DMASEV without correct security permissions
    HPS_DMA_CHFAULT_PERIPHERR = _BV(6 ), // Channel tried to execute DMAWFP/DMALDP/DMASTP/DMAFLUSHP without correct security permissions
    HPS_DMA_CHFAULT_RDWRERR   = _BV(7 ), // Channel tried to perform read/write without correct security permissions
    HPS_DMA_CHFAULT_MFIFOERR  = _BV(12), // Request to read or write was larger the the peripherals FIFO
    HPS_DMA_CHFAULT_STDATAERR = _BV(13), // Not enough data in FIFO for store command to complete
    HPS_DMA_CHFAULT_FETCHINS  = _BV(16), // Whether an error occurred while fetching DMA channel instruction
    HPS_DMA_CHFAULT_WRITEERR  = _BV(17), // An error occurred during a data write operation
    HPS_DMA_CHFAULT_READERR   = _BV(18), // An error occurred during a data read operation
    HPS_DMA_CHFAULT_DEBUGINS  = _BV(30), // Whether instruction error was generated from debug interface or system mem
    HPS_DMA_CHFAULT_LOCKUP    = _BV(31)  // Indicates DMA channel has locked up due to resource starvation
} HPSDmaChFault;

typedef enum {
    HPS_DMA_SECURITY_DEFAULT = 0, // Use power-on default.
    HPS_DMA_SECURITY_SECURE  = 1,
    HPS_DMA_SECURITY_NONSEC  = 2
} HPSDmaSecurity;

typedef enum {
    HPS_DMA_STATE_CHNL_FREE,
    HPS_DMA_STATE_CHNL_READY,
    HPS_DMA_STATE_CHNL_BUSY,
    HPS_DMA_STATE_CHNL_DONE,
    HPS_DMA_STATE_CHNL_ERROR,  // Set if error detected
    HPS_DMA_STATE_CHNL_ABORTED // Set after aborted
} HPSDmaChannelState;

#endif /* HPS_DMACONTROLLERENUMS_H_ */
