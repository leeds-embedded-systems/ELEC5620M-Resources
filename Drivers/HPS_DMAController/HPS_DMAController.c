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

#define HPS_DMACONTROLLER_C

#include "HPS_DMAController.h"
#include "HPS_DMAControllerProgram.h"
#include "HPS_DMAControllerRegs.h"

#include "Util/irq.h"

// Can tune based on hardware performance. Set a default
// if not globally defined.
#ifndef HPS_DMA_SHORT_TRANSFER_WORDS
#define HPS_DMA_SHORT_TRANSFER_WORDS 2
#endif

/*
 * Internal Functions
 */

// Re-initialse an existing program structure
static HpsErr_t HPS_DMA_initialiseProgram(HPSDmaProgram_t* prog) {
    if (!prog) return ERR_NULLPTR;
    // Initialise first program buffer word to DMAEND instruction for safety.
    prog->buf[0] = _HPS_DMA_ENDVAL;
    // The last word is always an end marker.
    prog->buf[prog->size] = _HPS_DMA_ENDVAL;
    // Set initial program length to zero.
    prog->len = 0;
    return ERR_SUCCESS;
}

// Allocate a new program structure
static HpsErr_t HPS_DMA_allocateProgram(unsigned int maxSize, bool autoFree, HPSDmaProgram_t** pProg) {
    if (!pProg) return ERR_NULLPTR;
    if (!maxSize) return ERR_TOOSMALL; // No point allocating a program with no space for any instructions.
    // Allocate the program structure
    HPSDmaProgram_t* prog = malloc(sizeof(*prog) + maxSize);
    if (!prog) return ERR_ALLOCFAIL;
    prog->size = maxSize - 1; // Account for space taken up by END marker.
    // Set auto-free marker.
    prog->autoFree = autoFree;
    // Initialise the program
    HPS_DMA_initialiseProgram(prog);
    // And return the allocated program.
    *pProg = prog;
    return ERR_SUCCESS;
}

// Free a program structure
//  - if onlyAuto is true, will only perform free operation if *pProg->autoFree is true.
static HpsErr_t HPS_DMA_freeProgram(HPSDmaProgram_t** pProg, bool onlyAuto) {
    if (!pProg) return ERR_NULLPTR;
    // Check if skipping manual free objects
    if (onlyAuto && !(*pProg)->autoFree) return ERR_SKIPPED;
    // Free the program and NULL out the program pointer
    free(*pProg);
    *pProg = NULL;
    return ERR_SUCCESS;
}

//Check the debug command status.
// - Will return ERR_BUSY if a command is running.
static HpsErr_t _HPS_DMA_debugCommandStatus(HPSDmaCtx_t* ctx) {
    return MaskCheck(HPSDMA_REG_DEBUG_STATUS(ctx->base), HPSDMA_DEBUG_STATUS_ISBUSY_MASK, HPSDMA_DEBUG_STATUS_ISBUSY_OFFS) ? ERR_BUSY : ERR_SUCCESS;
}

//Macro for quick re-init of debug program used when preparing to call issueDebugCommand.
#define HPS_DMA_INIT_DBGPROG(ctx) (ctx)->dbgProg->len = HPSDMA_REG_DEBUG_INSTR_OFFS

//Issue debug command
// - First populate the debug command in ctx->dbgProg befores issuing.
static HpsErr_t _HPS_DMA_issueDebugCommand(HPSDmaCtx_t* ctx, HPSDmaThreadType thread, HPSDmaChannelId channel) {
    //Ensure the debug interface is not busy
    HpsErr_t status = _HPS_DMA_debugCommandStatus(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (thread != HPS_DMA_THREADTYPE_MGR) {
        if (thread != HPS_DMA_THREADTYPE_CH) return ERR_BADID;
        // Skip inactive channels.
        if ((channel >= HPS_DMA_CHANNEL_COUNT) || (channel < HPS_DMA_CHANNEL_MIN)) return ERR_BADID;
        if (ctx->channelState[channel] <= HPS_DMA_STATE_CHNL_FREE) return ERR_SKIPPED;
    }
    //Issue the new command
    uint32_t* dbgCmdBuf = (uint32_t*)ctx->dbgProg->buf;
    dbgCmdBuf[0] = MaskModify(dbgCmdBuf[0], thread,  HPSDMA_DEBUG_INSTR0_THREAD_MASK,  HPSDMA_DEBUG_INSTR0_THREAD_OFFS );
    dbgCmdBuf[0] = MaskModify(dbgCmdBuf[0], channel, HPSDMA_DEBUG_INSTR0_CHANNEL_MASK, HPSDMA_DEBUG_INSTR0_CHANNEL_OFFS);
    HPSDMA_REG_DEBUG_INSTR(ctx->base, 0) = dbgCmdBuf[0];
    HPSDMA_REG_DEBUG_INSTR(ctx->base, 1) = dbgCmdBuf[1];
    HPSDMA_REG_DEBUG_COMMAND(ctx->base) = HPSDMA_DEBUG_COMMAND_EXECUTE;
    return ERR_SUCCESS;
}

//Get state of DMA thread
static HpsErr_t _HPS_DMA_getState(HPSDmaCtx_t* ctx, HPSDmaThreadType thread, HPSDmaChannelId channel, unsigned int* state) {
    if (!state) return ERR_NULLPTR;
    if (thread == HPS_DMA_THREADTYPE_MGR) {
        *state = MaskExtract(HPSDMA_REG_CTRL_MGRSTAT(ctx->base), HPSDMA_CTRL_MGRSTAT_DMASTAT_MASK, HPSDMA_CTRL_MGRSTAT_DMASTAT_OFFS);
    } else {
        if ((channel >= HPS_DMA_CHANNEL_COUNT) || (channel < HPS_DMA_CHANNEL_MIN)) return ERR_BADID;
        *state = MaskExtract(HPSDMA_REG_THREADSTAT_CHSTAT(ctx->base, channel), HPSDMA_THREADSTAT_CHSTAT_DMASTAT_MASK, HPSDMA_THREADSTAT_CHSTAT_DMASTAT_OFFS);
    }
    return ERR_SUCCESS;
}

//Get fault status of DMA thread
// - For Mgr thread, flags can be checked against HPSDmaMgrFault
// - For Ch thread, flags can be checked against HPSDmaChFault
static HpsErr_t _HPS_DMA_getFault(HPSDmaCtx_t* ctx, HPSDmaThreadType thread, HPSDmaChannelId channel, unsigned int* flags) {
    if (!flags) return ERR_NULLPTR;
    if (thread == HPS_DMA_THREADTYPE_MGR) {
        *flags = HPSDMA_REG_CTRL_MGRFAULTTYPE(ctx->base);
    } else {
        if ((channel >= HPS_DMA_CHANNEL_COUNT) || (channel < HPS_DMA_CHANNEL_MIN)) return ERR_BADID;
        *flags = HPSDMA_REG_CTRL_CHFAULTTYPE(ctx->base, channel);
    }
    return ERR_SUCCESS;
}

//Manually an event using the debug interface
static HpsErr_t _HPS_DMA_sendEvent(HPSDmaCtx_t* ctx, HPSDmaEventId eventId) {
    if ((eventId >= HPS_DMA_EVENT_COUNT) || (eventId < HPS_DMA_EVENT_MIN)) return ERR_BADID;
    HPS_DMA_INIT_DBGPROG(ctx);
    HpsErr_t status = HPS_DMA_instMgrDMASEV(ctx->dbgProg, eventId);
    if (ERR_IS_ERROR(status)) return status;
    return _HPS_DMA_issueDebugCommand(ctx, HPS_DMA_THREADTYPE_MGR, HPS_DMA_CHANNEL_MGR);
}

//Issue a DMAKILL command to a DMA thread
static HpsErr_t _HPS_DMA_killThread(HPSDmaCtx_t* ctx, HPSDmaThreadType thread, HPSDmaChannelId channel) {
    HPS_DMA_INIT_DBGPROG(ctx);
    HpsErr_t status = HPS_DMA_instDbgDMAKILL(ctx->dbgProg);
    if (ERR_IS_ERROR(status)) return status;
    return _HPS_DMA_issueDebugCommand(ctx, thread, channel);
}

//Check the current state of all channel threads.
static HpsErr_t _HPS_DMA_checkState(HPSDmaCtx_t* ctx) {
    HpsErr_t status;
    // If the controller is in reset, return not ready
    if (*HPS_DMA_RSTMGR_DMAREG & HPS_DMA_RSTMGR_DMAMASK) return ERR_NOTREADY;
    // Update state of manager
    unsigned int mgrIsFault = HPSDMA_REG_CTRL_MGRFAULTSTAT(ctx->base);
    if (mgrIsFault) {
        ctx->managerFault = HPSDMA_REG_CTRL_MGRFAULTSTAT(ctx->base);
    } else {
        ctx->managerFault = 0;
    }
    // Update the state of all channels
    unsigned int channelIsFault = HPSDMA_REG_CTRL_CHFAULTSTAT(ctx->base);
    bool allChannelsAborted = true;
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        // Check the channel state
        unsigned int state;
        status = _HPS_DMA_getState(ctx, HPS_DMA_THREADTYPE_CH, channel, &state);
        if (ERR_IS_ERROR(status)) return status;
        HPSDmaChStatus channelStatus = (HPSDmaChStatus)state;
        switch (channelStatus) {
            case HPS_DMA_CHSTAT_STOPPED:
                // Channel is in stopped state
                if (ctx->abortPending) {
                    // If an abort is pending and we are now stopped, then abort is complete for this channel
                    ctx->channelState[channel] = HPS_DMA_STATE_CHNL_ABORTED;
                } else if (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_BUSY) {
                    // If the channel has just stopped, mark as done
                    ctx->channelState[channel] = HPS_DMA_STATE_CHNL_DONE;
                }
                break;
            case HPS_DMA_CHSTAT_KILLING:
                // If in killing state, mark as aborted. This will not yet clear the pending flag.
                ctx->channelState[channel] = HPS_DMA_STATE_CHNL_ABORTED;
                break;
            case HPS_DMA_CHSTAT_EXECUTING:
            default:
                // If the channel is executing, mark as busy.
                ctx->channelState[channel] = HPS_DMA_STATE_CHNL_BUSY;
                break;
        }
        // Check if channel is in a fault state
        if (MaskCheck(channelIsFault, 0x1, channel)) {
            status = _HPS_DMA_getFault(ctx, HPS_DMA_THREADTYPE_CH, channel, &ctx->channelFault[channel]);
            if (ERR_IS_ERROR(status)) return status;
            ctx->channelState[channel] = HPS_DMA_STATE_CHNL_ERROR;
        } else {
            ctx->channelFault[channel] = 0;
        }
        // Check if channel is in abort state, and abort is done (status of stopped)
        allChannelsAborted = allChannelsAborted && (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_ABORTED) && (channelStatus == HPS_DMA_CHSTAT_STOPPED);
    }
    // Clear abort pending flag if all channels are now aborted.
    if (allChannelsAborted) {
        ctx->abortPending = false;
    }
    return ERR_SUCCESS;
}

//Start the transfer for the specified channel
static HpsErr_t _HPS_DMA_startTransferCh(HPSDmaCtx_t* ctx, HPSDmaChannelId channel) {
    //Check if channel is allocated and ready to start
    if (ctx->channelState[channel] != HPS_DMA_STATE_CHNL_READY) {
        return ERR_NOTREADY;
    }
    //Grab the correct program. This is assumed validated by setupTransfer which
    //is the only API compliant way that a channel could have been marked as ready.
    //We do a basic null-pointer check in case the memory was freed in the mean time.
    HPSDmaProgram_t* prog = ctx->chProg[channel];
    if (!prog) return ERR_NULLPTR;
    //Issue DMA start
    HPS_DMA_INIT_DBGPROG(ctx);
    if (ERR_IS_ERROR(HPS_DMA_instMgrDMAGO(ctx->dbgProg, channel, prog->nonSecure, (uint32_t)prog->buf))) return ERR_NOSPACE;
    _HPS_DMA_issueDebugCommand(ctx, HPS_DMA_THREADTYPE_MGR, HPS_DMA_CHANNEL_MGR);
    //Now running
    ctx->channelState[channel] = HPS_DMA_STATE_CHNL_BUSY;
    return ERR_SUCCESS;
}

//Setup transfer for the specified channel
static HpsErr_t _HPS_DMA_setupTransfer(HPSDmaCtx_t* ctx, HPSDmaProgram_t* prog, HPSDmaChCtlParams_t* params, bool autoStart) {
    //Update channel status flags to check if any previously running 
    //transfers have finished so we know if we can reuse this channel.
    HpsErr_t status = _HPS_DMA_checkState(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if channel is in use
    HPSDmaChannelId channel = params->channel;
    if (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_READY) {
        return ERR_INUSE;
    }
    if (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_BUSY) {
        return ERR_BUSY;
    }
    //Any other state means we are free to use the channel. Configure it.
    //Ensure program buffer is non-null, the final instruction is DMAEND,
    //and all loops are terminated.
    if (!prog || !prog->size) return ERR_NULLPTR;
    if (prog->len > prog->size) return ERR_BEYONDEND;
    if (prog->buf[prog->len-1] != _HPS_DMA_ENDVAL) return ERR_PARTIAL;
    if (prog->loopCtrUse || prog->loopLvl) return ERR_PARTIAL;
    //Mark as ready to go
    ctx->chProg[channel] = prog;
    ctx->channelState[channel] = HPS_DMA_STATE_CHNL_READY;
    //Ensure the IRQ flag for this channel is clear in case they are enabled.
    HPSDMA_REG_CTRL_IRQCLEAR(ctx->base) = MaskCreate(0x1, params->channel);
    //Start immediately if required.
    if (autoStart) {
        status = _HPS_DMA_startTransferCh(ctx, channel);
        if (ERR_IS_ERROR(status) && (status != ERR_NOTREADY)) {
            // Failed to auto-start. Return channel state to idle
            ctx->channelState[channel] = HPS_DMA_STATE_CHNL_FREE;
        }
    }
    return status;
}

//Basic copy program
static HpsErr_t _HPS_DMA_loadStoreProgram(HPSDmaCtx_t* ctx, HPSDmaProgram_t* prog, bool storeZero) {
    if (storeZero) {
        if (ERR_IS_ERROR(HPS_DMA_instChDMASTZ(prog))) return ERR_NOSPACE;
    } else {
        if (ERR_IS_ERROR(HPS_DMA_instChDMALD(prog))) return ERR_NOSPACE;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAST(prog))) return ERR_NOSPACE;
    }
    return ERR_SUCCESS;
}


//Create the default program
static HpsErr_t _HPS_DMA_generateMainProgram(HPSDmaCtx_t* ctx, DmaChunk_t* xfer, HPSDmaProgram_t** pProg) {
    //Allocate our program memory
    HpsErr_t status = HPS_DMA_allocateProgram(256, true, pProg);
    if (ERR_IS_ERROR(status)) return status;
    HPSDmaProgram_t* prog = *pProg;
    //Get transfer params
    HPSDmaChCtlParams_t* params = (HPSDmaChCtlParams_t*)xfer->params;
    //Build the DMA program from op-codes
    unsigned int readAddr = xfer->readAddr;
    unsigned int writeAddr = xfer->writeAddr;
    unsigned int wordSize = params->transferWidth;
    bool storeZero = false;
    bool memToMem = true;
    bool mustBeAligned = (params->endian != HPS_DMA_ENDIAN_NOSWAP); // Don't support endian swap on unaligned to make life easier.
    //Default to incrementing address
    params->_destAddrInc = true;
    params->_srcAddrInc  = true;
    //Check source type
    switch (params->srcType) {
        case HPS_DMA_SOURCE_REGISTER:
            // Register mode requires strict alignment of source/destination.
            mustBeAligned = true;
            // And is non-incrementing address
            params->_srcAddrInc = false;
            // Not mem to mem.
            memToMem = false;
            FALLTHROUGH;
            //no break
        case HPS_DMA_SOURCE_MEMORY:
            // Load source address
            if (ERR_IS_ERROR(HPS_DMA_instChDMAMOV(prog, HPS_DMA_TARGET_SAR, xfer->readAddr ))) return ERR_NOSPACE;
            break;
        case HPS_DMA_SOURCE_ZERO:
            // Override source address width destination address to align destination during first transfer.
            readAddr = writeAddr;
            // And use store zeros mode. 
            storeZero = false;
            // Not mem to mem.
            memToMem = false;
            break;
        default:
            // e.g. PERIPHERAL mode is not yet supported.
            return ERR_WRONGMODE;
    }
    //Check destination type
    switch (params->destType) {
        case HPS_DMA_DESTINATION_REGISTER:
            // Register mode requires strict alignment of source/destination.
            mustBeAligned = true;
            // And is non-incrementing address
            params->_destAddrInc = false;
            // Not mem to mem.
            memToMem = false;
            FALLTHROUGH;
            //no break
        case HPS_DMA_DESTINATION_MEMORY:
            // Load destination address
            if (ERR_IS_ERROR(HPS_DMA_instChDMAMOV(prog, HPS_DMA_TARGET_DAR, xfer->writeAddr))) return ERR_NOSPACE;
            break;
        default:
            // e.g. PERIPHERAL mode is not yet supported.
            return ERR_WRONGMODE;
    }
    //Alignment of source/destination addresses affect how we are going to program our DMA transfer
    unsigned int wordBytes = 1 << wordSize;
    unsigned int wordMask = wordBytes - 1;
    unsigned int readRemain = xfer->length;
    unsigned int writeRemain = xfer->length;
    // - Bytes needed to be read to align source address with word size, but no more than total length.
    unsigned int readInitial  = (-xfer->readAddr) & wordMask;
    if (readInitial > xfer->length) readInitial = xfer->length;
    // - Bytes needed to fill the first write to destination word to align to word size, but no more than total length.
    unsigned int writeInitial = (-xfer->writeAddr) & wordMask;
    if (writeInitial > xfer->length) writeInitial = xfer->length;
    // Check if address must be aligned. If not, return alignment error
    if (mustBeAligned && (readInitial || writeInitial)) return ERR_ALIGNMENT;
    // If we have a non-aligned source, then we start by reading enough words to align it
    // Not used in register mode as strict alignment is enabled.
    if (readInitial) {
        // Initial transfer will be a load burst of 1-byte transfers to acheive the desired alignment,
        // with an optional single wordSize store.
        params->_srcBurstSize = HPS_DMA_BURSTSIZE_1BYTE;
        params->_srcBurstLen = readInitial;
        params->_destBurstSize = wordSize;
        params->_destBurstLen = 1;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAMOVCCR(prog, params))) return ERR_NOSPACE; // Transfer params
        // First load is always used
        if (ERR_IS_ERROR(HPS_DMA_instChDMALD(prog))) return ERR_NOSPACE;
        readRemain -= readInitial;
        // If there is enough data available from the load to fill the first write word, we also do a store
        if (writeInitial >= readInitial) {
            if (ERR_IS_ERROR(HPS_DMA_instChDMAST(prog))) return ERR_NOSPACE;
            writeRemain -= readInitial; // Done readInitial bytes of write.
        }
    }
    // Bulk of transfer is done using wordSize chunks
    params->_srcBurstSize  = wordSize;
    params->_destBurstSize = wordSize;
    unsigned int wordsRemain = readRemain >> wordSize; // How many of transfers we need.
    unsigned int burstsRemain;
    if (wordsRemain <= HPS_DMA_BURSTLEN_MAX) {
        burstsRemain = 0; // If less than or equal to one burst worth of words remain, skip first burst loop.
    } else {
        burstsRemain = wordsRemain / HPS_DMA_BURSTLEN_MAX; // How many bursts are possible
    }
    // Update remaining counts
    readRemain  &= wordMask; // Mask off all full words as these will be sent in the main body.
    writeRemain &= wordMask; 
    wordsRemain -= (burstsRemain * HPS_DMA_BURSTLEN_MAX);
    // If we have any full length bursts remaining
    if (burstsRemain) {
        // Configure the transfer parameters for our burst
        params->_srcBurstLen  = params->burstDisable ? 1 : HPS_DMA_BURSTLEN_MAX;
        params->_destBurstLen = params->burstDisable ? 1 : HPS_DMA_BURSTLEN_MAX;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAMOVCCR(prog, params))) return ERR_NOSPACE;
        // Generate enough load-store instructions to complete the burst
        do {
            // Number of loops we need for this burst.
            //  - either bursts remaining, or max loop count, whichever is smaller
            unsigned int loopCount = min(burstsRemain, HPS_DMA_LOOP_COUNTER_MAX);
            burstsRemain -= loopCount;
            // Create a looped simple load-store program. Initialise the outer loop counter
            if (ERR_IS_ERROR(HPS_DMA_instChDMALP(prog, loopCount, HPS_DMA_TARGET_CNTR1, 0))) return ERR_NOSPACE;
            unsigned int lpStartOuter = prog->len;
            // If bursting is disabled, initialise the inner loop counter to make up the burst length.
            if (params->burstDisable && ERR_IS_ERROR(HPS_DMA_instChDMALP(prog, HPS_DMA_BURSTLEN_MAX, HPS_DMA_TARGET_CNTR0, 0))) return ERR_NOSPACE;
            unsigned int lpStartInner = prog->len;
            // Add correct body for mode
            status = _HPS_DMA_loadStoreProgram(ctx, prog, storeZero);
            if (ERR_IS_ERROR(status)) return status;
            // If bursting is disabled, complete the inner loop
            if (params->burstDisable) {
                status = HPS_DMA_instChDMALPEND(prog, lpStartInner, false, HPS_DMA_TARGET_CNTR0);
                if (ERR_IS_ERROR(status)) return status;
            }
            // Complete the outer loop
            status = HPS_DMA_instChDMALPEND(prog, lpStartOuter, false, HPS_DMA_TARGET_CNTR1);
            if (ERR_IS_ERROR(status)) return status;
        } while (burstsRemain);
    }
    // If there are any full words remaining, create a program to transfer them
    if (wordsRemain) {
        // Configure the transfer parameters for our burst
        params->_srcBurstLen = params->burstDisable ? 1 : wordsRemain;
        params->_destBurstLen = params->burstDisable ? 1 : wordsRemain;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAMOVCCR(prog, params))) return ERR_NOSPACE;
        // Create a simple load-store program. If bursting is disabled, initialise the inner loop counter to make up the burst length.
        if (params->burstDisable && ERR_IS_ERROR(HPS_DMA_instChDMALP(prog, wordsRemain, HPS_DMA_TARGET_CNTR0, 0))) return ERR_NOSPACE;
        unsigned int lpStartInner = prog->len;
        // Add correct body for mode
        status = _HPS_DMA_loadStoreProgram(ctx, prog, storeZero);
        if (ERR_IS_ERROR(status)) return status;
        // If bursting is disabled, complete the inner loop
        if (params->burstDisable) {
            status = HPS_DMA_instChDMALPEND(prog, lpStartInner, false, HPS_DMA_TARGET_CNTR0);
            if (ERR_IS_ERROR(status)) return status;
        }
    }
    // If there are any partial read words remaining, create a program to transfer them
    // Not used in register mode as strict alignment is enabled.
    if (readRemain) {
        // If there is any write bytes remaining, we an also store up to wordBytes while we are at it
        unsigned int writeLen = min(writeRemain, wordBytes);
        writeRemain -= writeLen;
        params->_srcBurstSize = HPS_DMA_BURSTSIZE_1BYTE;
        params->_srcBurstLen = readRemain;
        params->_destBurstSize = HPS_DMA_BURSTSIZE_1BYTE;
        params->_destBurstLen = writeLen;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAMOVCCR(prog, params))) return ERR_NOSPACE;
        // Add correct body for mode
        status = _HPS_DMA_loadStoreProgram(ctx, prog, storeZero);
        if (ERR_IS_ERROR(status)) return status;
    }
    // If there are any final write bytes left in the FIFO, issue a store to finish us off.
    // Not used in register mode as strict alignment is enabled.
    if (writeRemain) {
        if (storeZero) return ERR_PARTIAL; // This should never happen as readRemean and writeRemain are equal in storeZero mode.
        params->_destBurstSize = HPS_DMA_BURSTSIZE_1BYTE;
        params->_destBurstLen = writeRemain;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAMOVCCR(prog, params))) return ERR_NOSPACE;
        if (ERR_IS_ERROR(HPS_DMA_instChDMAST(prog))) return ERR_NOSPACE;
    }
    // If an event on done is requested, issue it. This will assert the corresponding IRQ if enabled.
    if (!params->doneEvent) {
        // Add memory barrier to ensure all transfers are complete. Can skip if not memory to memory.
        if (memToMem && ERR_IS_ERROR(HPS_DMA_instChDMAWMB(prog))) return ERR_NOSPACE;
        // And issue event
        if (ERR_IS_ERROR(HPS_DMA_instChDMASEV(prog, (HPSDmaEventId)params->channel))) return ERR_NOSPACE;
    }
    // End of the program
    return HPS_DMA_instChDMAEND(prog);
}

//Initialise hardware
static HpsErr_t _HPS_DMA_initHardware(HPSDmaCtx_t* ctx, HPSDmaHwInit_t* hwInit) {
    HpsErr_t status;
    // Ensure the DMA controller in reset to allow config
    *HPS_DMA_RSTMGR_DMAREG |= HPS_DMA_RSTMGR_DMAMASK;
    // Generate system manager configuration
    unsigned int sysMgrDmaReg = 0;
    unsigned int sysMgrDmaPeriphReg = 0;
    if (hwInit) {
        // Peripheral Multiplexing
        status = _HPS_DMA_periphMuxToSysMgr(hwInit->periphMux, &sysMgrDmaReg);
        if (ERR_IS_ERROR(status)) return status;
        // Manager Security
        status = _HPS_DMA_mgrSecToSysMgr(hwInit->mgrSecurity, &sysMgrDmaReg);
        if (ERR_IS_ERROR(status)) return status;
        // IRQ Security
        for (unsigned int eventId = HPS_DMA_EVENT_MIN; eventId < HPS_DMA_USER_EVENT_COUNT; eventId++) {
            status = _HPS_DMA_eventSecToSysMgr((HPSDmaEventId)eventId, hwInit->irqSecurity[eventId], &sysMgrDmaReg);
            if (ERR_IS_ERROR(status)) return status;
        }
        // Peripheral Security
        for (unsigned int periphId = 0; periphId < HPS_DMA_PERIPH_COUNT; periphId++) {
            status = _HPS_DMA_periphSecToSysMgrPeriph((HPSDmaPeripheralId)periphId, hwInit->periphSecurity[periphId], &sysMgrDmaPeriphReg); // Values in range but not in enum are don't care values in register so can just loop.
            if (ERR_IS_ERROR(status)) return status;
        }
    }
    // Update system manager config registers
    *HPS_DMA_SYSMGR_DMAREG       = sysMgrDmaReg;
    *HPS_DMA_SYSMGR_DMAPERIPHREG = sysMgrDmaPeriphReg;
    // Release the DMA controller from reset
    *HPS_DMA_RSTMGR_DMAREG &= ~HPS_DMA_RSTMGR_DMAMASK;
    // DMA controller hardware initialised successfully.
    return ERR_SUCCESS;
}

//Cleanup
static void _HPS_DMA_cleanup(HPSDmaCtx_t* ctx) {
    if (ctx->base) {
        // Kill all threads
        for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
            _HPS_DMA_killThread(ctx, HPS_DMA_THREADTYPE_CH, (HPSDmaChannelId)channel);
        }
        // Free constructor allocated debug program memory
        HPS_DMA_freeProgram(&ctx->dbgProg, false);
    }
    // Place the DMA controller in reset
    *HPS_DMA_RSTMGR_DMAREG |= HPS_DMA_RSTMGR_DMAMASK;
}

/*
 * User Facing APIs
 */

// Initialise the HPS DMA Driver
//  - base is a pointer to DMA peripheral in the HPS, both secure
//    or non-secure controllers are supported.
//  - wordSize is the width of the DMA word in log2(bytes).
//  - hwInit is optional configuration settings for hardware. If NULL, will use default for all HW settings.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t HPS_DMA_initialise(void* base, HPSDmaBurstSize wordSize, HPSDmaHwInit_t* hwInit, HPSDmaCtx_t** pCtx) {
    //Ensure user pointers valid
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_HPS_DMA_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    HPSDmaCtx_t* ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    ctx->wordSize = wordSize;
    ctx->wordBytes = 1 << wordSize;
    ctx->dma.ctx = ctx;
    ctx->dma.transferSpace = NULL;//(DmaXferSpaceFunc_t)&HPS_DMA_transferRequestSpace;
    ctx->dma.initXferParams = (DmaXferParamFunc_t)&HPS_DMA_initDmaChunkParam;
    ctx->dma.setupTransfer = (DmaXferFunc_t)&HPS_DMA_setupTransfer;
    ctx->dma.startTransfer = (DmaXferStartFunc_t)&HPS_DMA_startTransfer;
    ctx->dma.abortTransfer = (DmaAbortFunc_t)&HPS_DMA_abort;
    ctx->dma.transferBusy = (DmaStatusFunc_t)&HPS_DMA_busy;
    ctx->dma.transferDone = (DmaStatusFunc_t)&HPS_DMA_completed;
    ctx->dma.transferError = (DmaStatInfoFunc_t)&HPS_DMA_transferError;
    ctx->dma.transferAborted = (DmaStatusFunc_t)&HPS_DMA_aborted;
    //Initialise hardware
    status = _HPS_DMA_initHardware(ctx, hwInit);
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    //Initialise default programs
    status = HPS_DMA_allocateProgram(HPSDMA_REG_DEBUG_INSTR_OFFS + HPSDMA_REG_DEBUG_INSTR_LEN + HPS_DMA_INSTCH_DMAEND_LEN, false, &ctx->dbgProg);
    if (ERR_IS_ERROR(status)) return DriverContextInitFail(pCtx, status);
    //Set a sane default in the debug prog memory.
    HPS_DMA_INIT_DBGPROG(ctx);
    HPS_DMA_instChDMANOP(ctx->dbgProg);
    //Initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

// Check if driver initialised
//  - Returns true if driver previously initialised
bool HPS_DMA_isInitialised(HPSDmaCtx_t* ctx) {
    return DriverContextCheckInit(ctx);
}

// Set interrupt mask
//  - Configure whether masked flags to generate an interrupt to the processor.
//  - Any masked flag with a 1 bit in the flags will generate IRQ.
HpsErr_t HPS_DMA_setInterruptEnable(HPSDmaCtx_t* ctx, unsigned int flags, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Before changing anything we need to mask global interrupts temporarily
    HpsErr_t irqStatus = IRQ_globalEnable(false);
    //Modify the enable flags
    unsigned int curEn = MaskExtract(HPSDMA_REG_CTRL_IRQEN(ctx->base), HPSDMA_CTRL_IRQ_MASK, HPSDMA_CTRL_IRQ_OFFS);
    HPSDMA_REG_CTRL_IRQEN(ctx->base) = MaskInsert((flags & mask) | (curEn & ~mask), HPSDMA_CTRL_IRQ_MASK, HPSDMA_CTRL_IRQ_OFFS);
    //Re-enable interrupts if they were enabled
    IRQ_globalEnable(ERR_IS_SUCCESS(irqStatus));
    return ERR_SUCCESS;
}

// Get interrupt flags
//  - Returns flags indicating which sources have generated an interrupt
//  - If autoClear true, then interrupt flags will be cleared on read.
HpsErr_t HPS_DMA_getInterruptFlags(HPSDmaCtx_t* ctx, unsigned int* flags, unsigned int mask, bool autoClear) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Read the flags
    unsigned int irqflag = MaskExtract(HPSDMA_REG_CTRL_IRQSTATRAW(ctx->base), mask, HPSDMA_CTRL_IRQ_OFFS);
    //Clear the flags if requested
    if (irqflag && autoClear) {
        HPSDMA_REG_CTRL_IRQCLEAR(ctx->base) = MaskInsert(mask, HPSDMA_CTRL_IRQ_MASK, HPSDMA_CTRL_IRQ_OFFS);
    }
    //Return the flags if there is somewhere to return them to.
    if (flags) *flags = irqflag;
    return ERR_SUCCESS;
}

// Clear interrupt flags
//  - Clear interrupt flags with bits set in mask.
HpsErr_t HPS_DMA_clearInterruptFlags(HPSDmaCtx_t* ctx, unsigned int mask) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Clear Flags
    HPSDMA_REG_CTRL_IRQCLEAR(ctx->base) = MaskInsert(mask, HPSDMA_CTRL_IRQ_MASK, HPSDMA_CTRL_IRQ_OFFS);
    return ERR_SUCCESS;
}

// Initialise a DMA Chunk structure
//  - Allocates a HPSDmaChCtlParams_t structure and assigns it to the
//    the provided chunk.
//  - By default the autoFreeParams value will be set to true so that
//    it is auto-free'd when the chunk is processed.
HpsErr_t HPS_DMA_initDmaChunkParam(HPSDmaCtx_t* ctx, DmaChunk_t* xfer) {
    if (!xfer) return ERR_NULLPTR;
    // Allocate parameters
    HPSDmaChCtlParams_t* params = malloc(sizeof(*params));
    if (!params) return ERR_ALLOCFAIL;
    // Initialise the parameters
    HpsErr_t status = HPS_DMA_initParameters(ctx, params, HPS_DMA_SOURCE_MEMORY, HPS_DMA_DESTINATION_MEMORY);
    if (ERR_IS_ERROR(status)) {
        free(params);
        return status;
    }
    // Default to auto-free
    params->autoFreeParams = true;
    // Default channel based on chunk index
    params->channel = (HPSDmaChannelId)(xfer->index % HPS_DMA_CHANNEL_COUNT);
    // And save to chunk
    xfer->params = params;
    return ERR_SUCCESS;
}

// Initialise a HPSDmaChCtlParams structure
//  - Populates the default fields into a parameters structure
//  - By default the autoFreeParams value will be set to false.
HpsErr_t HPS_DMA_initParameters(HPSDmaCtx_t* ctx, HPSDmaChCtlParams_t* params, HPSDmaDataSource source, HPSDmaDataDest destination) {
    if (!params) return ERR_NULLPTR;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Configure the source defaults
    params->srcType = source;
    params->srcProtCtrl = HPS_DMA_PROT_DEFAULT;
    if (source == HPS_DMA_SOURCE_MEMORY) {
        params->srcCacheCtrl = HPS_DMA_CACHE_DEFAULT_MEM;
    } else {
        params->srcCacheCtrl = HPS_DMA_CACHE_DEFAULT_REG;
    }
    // Configure the destination defaults
    params->destType = destination;
    params->destProtCtrl = HPS_DMA_PROT_DEFAULT;
    if (destination == HPS_DMA_DESTINATION_MEMORY) {
        params->destCacheCtrl = HPS_DMA_CACHE_DEFAULT_MEM;
    } else {
        params->destCacheCtrl = HPS_DMA_CACHE_DEFAULT_REG;
    }
    // Configure the common defaults.
    params->transferWidth = ctx->wordSize;
    params->doneEvent = false;
    params->autoFreeParams = false;
    params->endian = HPS_DMA_ENDIAN_NOSWAP;
    return ERR_SUCCESS;
}
 
// Configure a DMA transfer
//  - Can optionally start the transfer immediately
//  - If selected DMA channel is in use, returns ERR_BUSY.
//  - Standard API defaults to (xfer->index % CH_COUNT) to perform a simple memory to memory transfer
//    - Can optionally configure transfer by setting xfer->params to a HPSDmaChCtlParams_t* structure.
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
HpsErr_t HPS_DMA_setupTransfer(HPSDmaCtx_t* ctx, DmaChunk_t* xfer, bool autoStart) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if we have been given optional parameters
    if (!xfer->params) {
        //Check if auto-start is requested, and there is only a small number of words to transfer
#if (HPS_DMA_SHORT_TRANSFER_WORDS > 0)
        if (autoStart && (xfer->length <= (HPS_DMA_SHORT_TRANSFER_WORDS * ctx->wordBytes))) {
            //If so, the overhead of setting up the DMA to perform a simple transfer
            //is not worth it, we might as well just copy manually.
            if ((xfer->readAddr > UINT32_MAX) || (xfer->writeAddr > UINT32_MAX)) return ERR_BEYONDEND;
            memcpy((void*)xfer->writeAddr, (void*)xfer->readAddr, xfer->length);
            return ERR_SKIPPED;
        }
#endif
        //Make default parameter structure
        status = HPS_DMA_initDmaChunkParam(ctx, xfer);
        if (ERR_IS_ERROR(status)) return status;
    }
    //Create the default memory copy program
    HPSDmaProgram_t* prog;
    status = _HPS_DMA_generateMainProgram(ctx, xfer, &prog);
    //Setup the transfer
    HPSDmaChCtlParams_t* params = (HPSDmaChCtlParams_t*)xfer->params;
    if (ERR_IS_SUCCESS(status)) {
        status = _HPS_DMA_setupTransfer(ctx, prog, params, autoStart);
    }
    //Free the optional parameters if required
    if (params->autoFreeParams) {
        xfer->params = NULL;
        free(params);
    }
    return status;
}

// Configure a DMA transfer with a custom program
//  - allows performing transfers with arbitrary programs.
//  - Use HPS_DMA_instCh*() API from HPS_DMAControllerProgram.h to create these programs.
//  - If selected DMA channel is in use, returns ERR_BUSY.
HpsErr_t HPS_DMA_setupTransferWithProgram(HPSDmaCtx_t* ctx, HPSDmaProgram_t* prog, HPSDmaChCtlParams_t* params, bool autoStart){
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure valid channel
    if ((params->channel < HPS_DMA_CHANNEL_USER0) || (params->channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Setup the transfer
    return _HPS_DMA_setupTransfer(ctx, prog, params, autoStart);
}

// Start the previously configured transfer
//  - If not enough space to start, returns ERR_BUSY.
//  - Standard API starts all prepared channels. The xxxCh() API can start any channel.
HpsErr_t HPS_DMA_startTransfer(HPSDmaCtx_t* ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Loop through starting all channels. Any not ready to start or busy will be skipped.
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        status = _HPS_DMA_startTransferCh(ctx, (HPSDmaChannelId)channel);
        if (ERR_IS_ERROR(status) && (status != ERR_NOTREADY)) return status;
    }
    return ERR_SUCCESS;
}
HpsErr_t HPS_DMA_startTransferCh(HPSDmaCtx_t* ctx, HPSDmaChannelId channel) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure valid channel
    if ((channel < HPS_DMA_CHANNEL_USER0) || (channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Start the transfer
    return _HPS_DMA_startTransferCh(ctx, channel);
}


// Check if the DMA controller is busy
//  - Will return ERR_BUSY if the DMA processor is busy.
//  - Standard API returns busy if any channel is busy. The xxxCh() API can check a specific channel.
HpsErr_t HPS_DMA_busy(HPSDmaCtx_t* ctx) {
    HpsErr_t status;
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        status = HPS_DMA_busyCh(ctx, (HPSDmaChannelId)channel);
        if (ERR_IS_ERROR(status)) return status;
    }
    return ERR_SUCCESS;
}
HpsErr_t HPS_DMA_busyCh(HPSDmaCtx_t* ctx, HPSDmaChannelId channel) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure a valid channel (can check all channels with this API, even main)
    if ((channel < HPS_DMA_CHANNEL_USER0) || (channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Check state of DMA
    status = _HPS_DMA_checkState(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Return whether busy
    return (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_BUSY) ? ERR_BUSY : ERR_SUCCESS;
}

// Check the state of a DMA channel
//  - Returns HPSDmaChannelState on success.
//  - Optionally returns the last fault condition for the channel to *fault.
HpsErr_t HPS_DMA_checkState(HPSDmaCtx_t* ctx, HPSDmaChannelId channel, unsigned int* fault) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure a valid channel (can check all channels with this API, even main)
    if ((channel < HPS_DMA_CHANNEL_USER0) || (channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Check state of DMA
    status = _HPS_DMA_checkState(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Return fault state if requested
    if (fault) *fault = ctx->channelFault[channel];
    //Return the current channel state
    return ctx->channelState[channel];
}


// Check if the controller is in an error state
// - Returns success if not in error state.
// - Returns ERR_IOFAIL if in an error state
//    - Optional second argument can be used to read error information.
// - Non-stateful. Can be used to check at any time.
HpsErr_t HPS_DMA_transferError(HPSDmaCtx_t* ctx, unsigned int* errorInfo) {
    HpsErr_t status;
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        // Check the channel state
        status = HPS_DMA_checkState(ctx, (HPSDmaChannelId)channel, errorInfo);
        // If a general failure, return the error
        if (ERR_IS_ERROR(status)) return status;
        // If controller is in error state, return ERR_IOFAIL
        if (status == HPS_DMA_STATE_CHNL_ERROR) return ERR_IOFAIL;
    }
    // No channels in error state.
    return ERR_SUCCESS;
}

// Check if the DMA controller completed
//  - Calling this function will return TRUE if the DMA has just completed
//  - It will return ERR_BUSY if (a) the transfer has not completed, or (b) the completion has already been acknowledged
//  - If ERR_SUCCESS is returned, the done flag will be cleared automatically acknowledging the completion.
//  - Standard API returns complete once all channels are done. The xxxCh() API can check any channel.
HpsErr_t HPS_DMA_completed(HPSDmaCtx_t* ctx) {
    //Check if busy.
    HpsErr_t status = HPS_DMA_busy(ctx);
    if (ERR_IS_ERROR(status)) return status; // One or more channels is busy
    //All channels have stopped. Loop through all channels
    //to check if they have just completed
    status = ERR_BUSY;
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        //Check if transfer has just completed
        if (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_DONE) {
            //If so, success. Clear the channel state.
            ctx->channelState[channel] = HPS_DMA_STATE_CHNL_FREE;
            //If auto-free is enabled for the program, free it now we are finished with it.
            HPS_DMA_freeProgram(&ctx->chProg[channel], true);
            //We will return complete as none are busy and at least one is done.
            status = ERR_SUCCESS;
        }
    }
    return status;
}
HpsErr_t HPS_DMA_completedCh(HPSDmaCtx_t* ctx, HPSDmaChannelId channel) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure a valid channel (can check all channels with this API, even main)
    if ((channel < HPS_DMA_CHANNEL_USER0) || (channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Check state of DMA
    status = _HPS_DMA_checkState(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if transfer has just completed
    if (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_DONE) {
        //If so, success. Clear the channel state.
        ctx->channelState[channel] = HPS_DMA_STATE_CHNL_FREE;
        //If auto-free is enabled for the program, free it now we are finished with it.
        HPS_DMA_freeProgram(&ctx->chProg[channel], true);
        return ERR_SUCCESS;
    }
    return ERR_BUSY;
}


// Issue an abort request to the DMA controller
//  - This will abort all channels.
//  - DMA_ABORT_NONE clears the abort request (whether or not it has actually completed). Must be called after DMA_ABORT_FORCE abort to release from reset.
//  - DMA_ABORT_SAFE requests the controller stop once any outstanding bus requests are handled
//  - DMA_ABORT_FORCE stops immediately which may require a wider reset.
HpsErr_t HPS_DMA_abort(HPSDmaCtx_t* ctx, DmaAbortType abort) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if clearing abort state
    if (abort == DMA_ABORT_NONE) {
        //Clear the abort pending flag, whether or not we have actually competed
        ctx->abortPending = false;
        //And release the DMA controller from reset in case it was put there by a forced abort.
        *HPS_DMA_RSTMGR_DMAREG &= ~HPS_DMA_RSTMGR_DMAMASK;
        //Refresh states
        return _HPS_DMA_checkState(ctx);
    }
    //Mark as abort pending
    ctx->abortPending = true;
    //Check the abort type
    if (abort == DMA_ABORT_SAFE) {
        //For safe abort, issue kill instruction to all channels.
        for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
            _HPS_DMA_killThread(ctx, HPS_DMA_THREADTYPE_CH, (HPSDmaChannelId)channel);
        }
        //Refresh states
        return _HPS_DMA_checkState(ctx);
    } else {
        //For forced abort, simply place in reset the whole DMA controller
        *HPS_DMA_RSTMGR_DMAREG |= HPS_DMA_RSTMGR_DMAMASK;
        //Mark all channels as aborted immediately.
        for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
            ctx->channelState[channel] = HPS_DMA_STATE_CHNL_ABORTED;
        }
        //Force abort done.
        return ERR_SUCCESS;
    }
}


// Check if the DMA controller aborted
//  - Calling this function will return TRUE if the controller aborted
//  - It will return ERR_BUSY if (a) the transfer has not aborted, or (b) the abort has already been acknowledged
//  - If ERR_SUCCESS is returned, the abort flag will be cleared automatically acknowledging it.
//  - Standard API returns aborted once all channels are aborted. The xxxCh() API can check any channel.
HpsErr_t HPS_DMA_aborted(HPSDmaCtx_t* ctx) {
    //Check if busy.
    HpsErr_t status = HPS_DMA_busy(ctx);
    if (ERR_IS_ERROR(status)) return status; // One or more channels is busy
    //All channels have stopped. Loop through all channels
    //to check if they have just aborted
    status = ERR_BUSY;
    for (unsigned int channel = HPS_DMA_CHANNEL_MIN; channel < HPS_DMA_CHANNEL_COUNT; channel++) {
        //Check if transfer has just aborted
        if (!ctx->abortPending && (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_ABORTED)) {
            //If so, success. Clear the channel state.
            ctx->channelState[channel] = HPS_DMA_STATE_CHNL_FREE;
            //If auto-free is enabled for the program, free it now we are finished with it.
            HPS_DMA_freeProgram(&ctx->chProg[channel], true);
            //We will return complete as none are busy and at least one is done.
            status = ERR_SUCCESS;
        }
    }
    return status;
}
HpsErr_t HPS_DMA_abortedCh(HPSDmaCtx_t* ctx, HPSDmaChannelId channel) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure a valid channel (can check all channels with this API, even main)
    if ((channel < HPS_DMA_CHANNEL_USER0) || (channel > HPS_DMA_CHANNEL_USER7)) return ERR_BADID;
    //Check state of DMA
    status = _HPS_DMA_checkState(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if transfer has just completed
    if (!ctx->abortPending && (ctx->channelState[channel] == HPS_DMA_STATE_CHNL_ABORTED)) {
        //If so, success. Clear the channel state.
        ctx->channelState[channel] = HPS_DMA_STATE_CHNL_FREE;
        //If auto-free is enabled for the program, free it now we are finished with it.
        HPS_DMA_freeProgram(&ctx->chProg[channel], true);
        return ERR_SUCCESS;
    }
    return ERR_BUSY;
}


