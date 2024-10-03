/*
 * HPS DMA Controller Driver
 * -------------------------
 *
 * Driver for configuring an ARM AMBA DMA-330 controller as is
 * available in the Cyclone V and Arria 10 HPS bridges.
 * 
 * The simplest use case is a memory to memory transfer with
 * a read address (source), a write address (destination) and
 * a length in bytes. The addresses need not be aligned to the
 * DMA word size, it will automatically optimise the alignment.
 * This can be performed by calling HPS_DMA_setupTransfer() and
 * optionally HPS_DMA_startTransfer() if autoStart is set false.
 * 
 * The driver supports up to eight DMA channels. By default the
 * channel used will match the xfer->index parameter modulo 8
 * to allow programming up to eight DMA transfer chunks.
 * 
 * For more complex transfers such as with non-incrementing
 * source or destination addresses, the HPS_DMA_initDmaChunkParam
 * API can be used to create a transfer with optional parameters
 * structure to allow greater configuration.
 * 
 * For very short simple transfers (HPS_DMA_setupTransfer with 
 * xfer->params==NULL and autoStart==true), the driver will
 * copy using memcpy instead to reduce overhead. A short 
 * transfer is one with a length less than or equal to wordSize
 * times HPS_DMA_SHORT_TRANSFER_WORDS. This is by default defined
 * as 2 words. Globally define HPS_DMA_SHORT_TRANSFER_WORDS to
 * override this parameter (0 disables memcpy usage). This is
 * a global define as it is an optimisation based on the DMA
 * overhead rather than general performance.
 *
 * The DMA has configurable cache handling capabilites. When
 * performing a memory to memory transfer, the cache settings
 * can be specified using the transfer parameters structure. By
 * default the value is set to Non-Cacheable as this library set
 * assumes that chacking is disabled. To override this, globally
 * define HPS_DMA_DEFAULT_MEM_CACHE_VALUE to be one of the values
 * from the HPSDmaCacheable enum (see HPS_DMAControllerEnums.h).
 * 
 * The DMA controller is a highly configurable device with its
 * own 8-core processor and custom instruction set to allow all
 * manner of weird transfers to be performed. This capability can
 * be leveraged using the HPS_DMA_setupTransferWithProgram API
 * which allows configuring a transfer with a completely custom
 * program. The HPS_DMAControllerProgram.h header provides a set
 * of assembler APIs to build DMA programs.
 * 
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 18/02/2024 | Creation of driver.
 *
 */

#ifndef HPS_DMACONTROLLER_H_
#define HPS_DMACONTROLLER_H_

#include "Util/driver_ctx.h"
#include "Util/driver_dma.h"

#include "Util/bit_helpers.h"
#include "Util/macros.h"


#ifdef __ARRIA10__
#include "HPS_DMAControllerPeriphA10.h"
#else
#include "HPS_DMAControllerPeriphCV.h"
#endif

typedef enum {
    HPS_DMA_IRQ_NONE     = 0x0,
    HPS_DMA_IRQ_CHANNEL0 = _BV(HPS_DMA_EVENT_USER0),
    HPS_DMA_IRQ_CHANNEL1 = _BV(HPS_DMA_EVENT_USER1),
    HPS_DMA_IRQ_CHANNEL2 = _BV(HPS_DMA_EVENT_USER2),
    HPS_DMA_IRQ_CHANNEL3 = _BV(HPS_DMA_EVENT_USER3),
    HPS_DMA_IRQ_CHANNEL4 = _BV(HPS_DMA_EVENT_USER4),
    HPS_DMA_IRQ_CHANNEL5 = _BV(HPS_DMA_EVENT_USER5),
    HPS_DMA_IRQ_CHANNEL6 = _BV(HPS_DMA_EVENT_USER6),
    HPS_DMA_IRQ_CHANNEL7 = _BV(HPS_DMA_EVENT_USER7),
    HPS_DMA_IRQ_ABORTED  = _BV(HPS_DMA_EVENT_ABORTED),
    HPS_DMA_IRQ_ALL      = HPS_DMA_IRQ_CHANNEL0 | HPS_DMA_IRQ_CHANNEL1 | HPS_DMA_IRQ_CHANNEL2 |
                           HPS_DMA_IRQ_CHANNEL3 | HPS_DMA_IRQ_CHANNEL4 | HPS_DMA_IRQ_CHANNEL5 | 
                           HPS_DMA_IRQ_CHANNEL6 | HPS_DMA_IRQ_CHANNEL7 | HPS_DMA_IRQ_ABORTED
} HPSDmaIrqFlags;

// This structure may optionally be added to a PDmaChunk_t object ->params entry
// to allow configuring the DMA mode. If not provided, will default to Mem->Mem
// transfer with no endianness swap.
// You must call HPS_DMA_initParameters to initialise this structure.
typedef struct {
    // Source
    HPSDmaDataSource srcType;        // Source type determines whether to increment address during burst
    HPDDmaProtection srcProtCtrl;    // Protocol setting (value for ARPROT)
    HPSDmaCacheable  srcCacheCtrl;   // Cache setting (value for ARCACHE)
    // Destination
    HPSDmaDataDest   destType;       // Destination type determines whether to increment address during burst
    HPDDmaProtection destProtCtrl;   // Protocol setting (value for AWPROT)
    HPSDmaCacheable  destCacheCtrl;  // Cache setting (value for AWCACHE)
    // Common
    HPSDmaBurstSize  transferWidth;  // Maximum single transfer width (log2(bytes)). For example register width for _REGISTER type transfers. Defaults to wordSize set during initialisation.
    HPSDmaEndianSwap endian;         // Swap Endianness
    HPSDmaChannelId  channel;        // Channel to program
    bool             autoFreeParams; // If true, this structure will be free'd automatically when executed.
    bool             burstDisable;   // Whether bursting is allowed. If disabled, no transfer will be larger than a single "word".
    bool             doneEvent;      // If true, will issue an event for the current channel on transfer complete. Will trigger IRQ only if the corresponding IRQ is enabled.
    // Peripheral
    PDrvCtx_t        periphCtx;      // Driver context for peripheral (e.g. I2C or UART) or NULL.
    void*            periphFunc;     // Callback function for peripherals (not yet implemented)
    // Internally set values for standard programs. Must be manually set for custom programs.
    HPSDmaBurstSize  _destBurstSize; // Destination width of a burst "word"
    HPSDmaBurstSize  _srcBurstSize;  // Source width of a burst "word"
    unsigned int     _destBurstLen;  // Destination number of words in a burst (1 to 16)
    unsigned int     _srcBurstLen;   // Source number of words in a burst (1 to 16)
    bool             _destAddrInc;   // Destination incrementing address (true) or fixed address (false)
    bool             _srcAddrInc;    // Source incrementing address (true) or fixed address (false)
} HPSDmaChCtlParams_t, *PHPSDmaChCtlParams_t;

typedef struct PACKED {
    unsigned int size;       // Maximum program length
    unsigned int len;        // Current length of the program
    uint8_t      loopLvl;    // Depth of currently unterminated loops. Must end up back at 0 for valid program.
    uint8_t      loopCtrUse; // Mask indicating which loop counters are currently in use
    bool         nonSecure;  // Whether program is executed in non-secure mode.
    bool         autoFree;   // If true, will automatically free program structure after use.
    // Flexible length member. PHPSDmaProgram_t should be created with:
    //    pgm = malloc(sizeof(*pgm) + progSize)
    //    pgm->size = progSize;
    //    pgm->len = 0;
    // Then buf[] can contain up to `progSize` instruction bytes.
    // Can use `HPS_DMA_allocateProgram(progSize, autoFree, &pgm);` to allocate.
    uint8_t buf[] ALIGNED(sizeof(uint32_t));
} HPSDmaProgram_t, *PHPSDmaProgram_t;

//HW initialisation parameters for driver.
//  - For default values, initialise struct to all zeros (myStruct = {0})
//  - Then modify any fields as desired.
typedef struct {
    HPSDmaSecurity  mgrSecurity;                               // Security setting for the manager thread
    HPSDmaSecurity  irqSecurity    [HPS_DMA_USER_EVENT_COUNT]; // Security setting for each user DMA event (one for each HPSDmaEventId _USER entry)
    HPSDmaSecurity  periphSecurity [HPS_DMA_PERIPH_COUNT];     // Security setting for each peripheral (one for each HPSDmaPeripheralId entry)
    HPSDmaPeriphMux periphMux      [HPS_DMA_PERMUX_COUNT];     // Peripheral multiplexer configuration. Indexed with multiplexed HPSDmaPeripheralId minus HPS_DMA_PERMUX_OFFSET
} HPSDmaHwInit_t, *PHPSDmaHwInit_t;

//Driver context
typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile unsigned int* base;
    HPSDmaBurstSize wordSize;
    unsigned int wordBytes;
    DmaCtx_t dma;
    bool abortPending;
    //State of DMA manager
    unsigned int managerFault; // See HPSDmaMgrFault for flag bitmasks
    //State of each DMA channel
    HPSDmaChannelState channelState[HPS_DMA_CHANNEL_COUNT]; // Channel execution state machine.
    unsigned int channelFault[HPS_DMA_CHANNEL_COUNT]; // Last reported fault state for channel. See HPSDmaChFault for flag bitmasks
    //Program memory pointers for each DMA channel
    PHPSDmaProgram_t chProg[HPS_DMA_CHANNEL_COUNT];
    //Debug command buffer.
    PHPSDmaProgram_t dbgProg;
} HPSDmaCtx_t, *PHPSDmaCtx_t;

// Initialise the HPS DMA Driver
//  - base is a pointer to DMA peripheral in the HPS, both secure
//    or non-secure controllers are supported.
//  - wordSize is the width of the DMA word in log2(bytes).
//  - hwInit is optional configuration settings for hardware. If NULL, will use default for all HW settings.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t HPS_DMA_initialise(void* base, HPSDmaBurstSize wordSize, PHPSDmaHwInit_t hwInit, PHPSDmaCtx_t* pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool HPS_DMA_isInitialised(PHPSDmaCtx_t ctx);

// Set interrupt mask
//  - Configure whether masked flags to generate an interrupt to the processor.
//  - Any masked flag with a 1 bit in the flags will generate IRQ.
//  - Use HPSDmaIrqFlags enum to build IRQ bit masks
HpsErr_t HPS_DMA_setInterruptEnable(PHPSDmaCtx_t ctx, unsigned int flags, unsigned int mask);

// Get interrupt flags
//  - Returns flags indicating which sources have generated an interrupt
//  - If autoClear true, then interrupt flags will be cleared on read.
//  - Use HPSDmaIrqFlags enum to decode IRQ bit masks
HpsErr_t HPS_DMA_getInterruptFlags(PHPSDmaCtx_t ctx, unsigned int* flags, unsigned int mask, bool autoClear);

// Clear interrupt flags
//  - Clear interrupt flags with bits set in mask.
//  - Use HPSDmaIrqFlags enum to build IRQ bit masks
HpsErr_t HPS_DMA_clearInterruptFlags(PHPSDmaCtx_t ctx, unsigned int mask);

// Initialise a DMA Chunk structure
//  - Allocates a HPSDmaChCtlParams_t structure and assigns it to the
//    the provided chunk.
//  - By default the autoFreeParams value will be set to true so that
//    it is auto-free'd when the chunk is processed.
HpsErr_t HPS_DMA_initDmaChunkParam(PHPSDmaCtx_t ctx, PDmaChunk_t xfer);

// Initialise a HPSDmaChCtlParams structure
//  - Populates the default fields into a parameters structure
//  - By default the autoFreeParams value will be set to false.
HpsErr_t HPS_DMA_initParameters(PHPSDmaCtx_t ctx, PHPSDmaChCtlParams_t param, HPSDmaDataSource source, HPSDmaDataDest destination);

// Configure a DMA transfer
//  - Can optionally start the transfer immediately
//  - If selected DMA channel is in use, returns ERR_BUSY.
//  - Standard API defaults to (xfer->index % CH_COUNT) to perform a simple memory to memory transfer
//    - Can optionally configure transfer by setting xfer->params to a PHPSDmaChCtlParams_t structure.
//    - Channel can be changed by setting the params->channel
//    - For burst transfers, configure params appropriately
//  - For simple transfers (xfer withot params) and with autoStart==true, if the length is
//    very short, will use memcpy instead to reduce overhead. If the memcpy approach is used
//    the API will return ERR_SKIPPED to indicate that it completed without starting a DMA
//    transfer.
//  - Will return ERR_SUCCESS if a transfer was successfully queued. Once queued:
//    - Call HPS_DMA_startTransfer*() if autoStart=false
//    - Use HPS_DMA_busy*()/HPS_DMA_completed*()/HPS_DMA_aborted*() to check the status of the transfer.
//    - Use HPS_DMA_abort() to stop any running transfers.
HpsErr_t HPS_DMA_setupTransfer(PHPSDmaCtx_t ctx, PDmaChunk_t xfer, bool autoStart);

// Configure a DMA transfer with a custom program
//  - allows performing transfers with arbitrary programs.
//  - Use HPS_DMA_instCh*() API from HPS_DMAControllerProgram.h to create these programs.
//  - If selected DMA channel is in use, returns ERR_BUSY.
//  - The memory pointed to by prog *must* remain valid in memory through the entire transfer
//    as the buffer pointer is passed directly to the DMA controller.
HpsErr_t HPS_DMA_setupTransferWithProgram(PHPSDmaCtx_t ctx, PHPSDmaProgram_t prog, PHPSDmaChCtlParams_t params, bool autoStart);

// Start the previously configured transfer
//  - If not enough space to start, returns ERR_BUSY.
//  - Standard API starts all prepared channels. The xxxCh() API can start any channel.
HpsErr_t HPS_DMA_startTransfer(PHPSDmaCtx_t ctx);
HpsErr_t HPS_DMA_startTransferCh(PHPSDmaCtx_t ctx, HPSDmaChannelId channel);

// Check if the DMA controller is busy
//  - Will return ERR_BUSY if the DMA processor is busy.
//  - Standard API returns busy if any channel is busy. The xxxCh() API can check a specific channel.
HpsErr_t HPS_DMA_busy(PHPSDmaCtx_t ctx);
HpsErr_t HPS_DMA_busyCh(PHPSDmaCtx_t ctx, HPSDmaChannelId channel);

// Check the state of a DMA channel
//  - Returns HPSDmaChannelState on success.
//  - Optionally returns the last fault condition for the channel to *fault.
//     - Use HPSDmaChFault bitmasks to decode fault condition.
HpsErr_t HPS_DMA_checkState(PHPSDmaCtx_t ctx, HPSDmaChannelId channel, unsigned int* fault);

// Check if the controller is in an error state
// - Returns success if not in error state.
// - Returns ERR_IOFAIL if in an error state
//    - Optional second argument can be used to read error information.
// - Non-stateful. Can be used to check at any time.
HpsErr_t HPS_DMA_transferError(PHPSDmaCtx_t ctx, unsigned int* errorInfo);

// Check if the DMA controller completed
//  - Calling this function will return success if the DMA has just completed
//  - It will return ERR_BUSY if (a) the transfer has not completed, or (b) the completion has already been acknowledged
//  - If ERR_SUCCESS is returned, the done flag will be cleared automatically acknowledging the completion.
//  - Standard API returns complete once all channels are done. The xxxCh() API can check any channel.
HpsErr_t HPS_DMA_completed(PHPSDmaCtx_t ctx);
HpsErr_t HPS_DMA_completedCh(PHPSDmaCtx_t ctx, HPSDmaChannelId channel);

// Issue an abort request to the DMA controller
//  - This will abort all channels.
//  - DMA_ABORT_NONE clears the abort request (whether or not it has actually completed). Must be called after DMA_ABORT_FORCE abort to release from reset.
//  - DMA_ABORT_SAFE requests the controller stop once any outstanding bus requests are handled
//  - DMA_ABORT_FORCE stops immediately which may require a wider reset.
HpsErr_t HPS_DMA_abort(PHPSDmaCtx_t ctx, DmaAbortType abort);

// Check if the DMA controller aborted
//  - Calling this function will return TRUE if the controller aborted
//  - It will return ERR_BUSY if (a) the transfer has not aborted, or (b) the abort has already been acknowledged
//  - If ERR_SUCCESS is returned, the abort flag will be cleared automatically acknowledging it.
//  - Standard API returns aborted once all channels are aborted. The xxxCh() API can check any channel.
HpsErr_t HPS_DMA_aborted(PHPSDmaCtx_t ctx);
HpsErr_t HPS_DMA_abortedCh(PHPSDmaCtx_t ctx, HPSDmaChannelId channel);

#endif /* HPS_DMACONTROLLER_H_ */

