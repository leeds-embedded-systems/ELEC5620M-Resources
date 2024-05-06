/*
 * HPS DMA Controller Instructions
 * -------------------------------
 *
 * Instruction set for an ARM AMBA DMA-330
 * controller as is available in the Cyclone V
 * and Arria 10 HPS bridges.
 * 
 * This header can be included by advanced users
 * to allow developing of custon programs to be
 * run on a DMA controller channel.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 18/02/2024 | Creation of header.
 *
 */

#ifndef HPS_DMACONTROLLERPROGRAM_H_
#define HPS_DMACONTROLLERPROGRAM_H_

#include "HPS_DMAController/HPS_DMAController.h"

#include <stdint.h>
#include <stdbool.h>

#include "Util/ct_assert.h"
#include "Util/macros.h"
#include "Util/bit_helpers.h"

typedef enum {
    HPS_DMA_REQCOND_NONE   = 0,
    HPS_DMA_REQCOND_SINGLE = 1,
    HPS_DMA_REQCOND_BURST  = 3
} HPSDmaRequestCondition;

typedef enum {
    HPS_DMA_REQUEST_SINGLE = 0,
    HPS_DMA_REQUEST_PERIPH = 1,
    HPS_DMA_REQUEST_BURST  = 2
} HPSDmaRequestType;

typedef enum {
    HPS_DMA_TARGET_SAR = 0,
    HPS_DMA_TARGET_CCR = 1,
    HPS_DMA_TARGET_DAR = 2
} HPSDmaTargetReg;

typedef enum {
    HPS_DMA_TARGET_CNTR0 = 0,
    HPS_DMA_TARGET_CNTR1 = 1
} HPSDmaTargetCounter;
#define HPS_DMA_LOOP_COUNTER_MAX 256

/*
 * Instruction Set
 *
 * Instructions can have variable length payloads. These
 * macros can be used as initialisers for an uint8_t array.
 */

// Channel Control Bits

#define _HPS_DMA_CHCNTRL_SRCADRINC_MASK 0x1  // Whether to increment source address during burst
#define _HPS_DMA_CHCNTRL_SRCADRINC_OFFS 0
#define _HPS_DMA_CHCNTRL_SRCBRSTSZ_MASK 0x7  // Size of source burst data. clog2 of number of bytes per beat. Max 16 bytes.
#define _HPS_DMA_CHCNTRL_SRCBRSTSZ_OFFS 1
#define _HPS_DMA_CHCNTRL_SRCBRSTLN_MASK (HPS_DMA_BURSTLEN_MAX-1)  // Length of burst. Number of read transfers to perform in a burst. Max 16 xfers.
#define _HPS_DMA_CHCNTRL_SRCBRSTLN_OFFS 4
#define _HPS_DMA_CHCNTRL_SRCPROT_MASK   0x7  // Protocol setting (ARPROT) for source AXI transfer.
#define _HPS_DMA_CHCNTRL_SRCPROT_OFFS   8
#define _HPS_DMA_CHCNTRL_SRCCACHE_MASK  0x7  // Cache setting (ARCACHE) for source AXI transfer.
#define _HPS_DMA_CHCNTRL_SRCCACHE_OFFS  11
#define _HPS_DMA_CHCNTRL_DSTADRINC_MASK 0x1  // Whether to increment destination address during burst
#define _HPS_DMA_CHCNTRL_DSTADRINC_OFFS 14
#define _HPS_DMA_CHCNTRL_DSTBRSTSZ_MASK 0x7  // Size of destination burst data. clog2 of number of bytes per beat. Max 16 bytes.
#define _HPS_DMA_CHCNTRL_DSTBRSTSZ_OFFS 15
#define _HPS_DMA_CHCNTRL_DSTBRSTLN_MASK (HPS_DMA_BURSTLEN_MAX-1)  // Length of burst. Number of write transfers to perform in a burst. Max 16 xfers.
#define _HPS_DMA_CHCNTRL_DSTBRSTLN_OFFS 18
#define _HPS_DMA_CHCNTRL_DSTPROT_MASK   0x7  // Protocol setting (AWPROT) for destination AXI transfer.
#define _HPS_DMA_CHCNTRL_DSTPROT_OFFS   22
#define _HPS_DMA_CHCNTRL_DSTCACHE_MASK  0x7  // Cache setting (AWCACHE) for destination AXI transfer.
#define _HPS_DMA_CHCNTRL_DSTCACHE_OFFS  25
#define _HPS_DMA_CHCNTRL_ENDIANSWP_MASK 0x7  // Swap data endianness if source and destination use different byte order.
#define _HPS_DMA_CHCNTRL_ENDIANSWP_OFFS 28

#define _HPS_DMA_ENDVAL 0x00
#define _HPS_DMA_NOPVAL 0x18

#define _HPS_DMA_INST_APPEND8(pl)   ((pl) & 0xFF)
#define _HPS_DMA_INST_APPEND16(pl)  ((pl) & 0xFF),(((pl) >> 8) & 0xFF)
#define _HPS_DMA_INST_APPEND32(pl)  ((pl) & 0xFF),(((pl) >> 8) & 0xFF),(((pl) >> 16) & 0xFF),(((pl) >> 24) & 0xFF)

// Helper used to convert the instruction initialiser macros into consecutive memory writes
//  - This also keeps the program terminated with an end marker to prevent issues if an incomplete program is started.
//static inline HpsErr_t __HPS_DMA_copy(HPSDmaProgram_t* prog, unsigned int len, ...) {
//    va_list pl;
//    va_start(pl, len);
//    for (unsigned int argIdx = 0; argIdx < min(len,6); argIdx++) {
//        buf[argIdx] = va_arg(pl, unsigned int);
//    }
//    va_end(pl);
//    return len;
//}
static inline HpsErr_t __HPS_DMA_copy1(HPSDmaProgram_t* prog, uint8_t pl0, bool insert) {
    if ((prog->size < 1) || (prog->size - 1 < prog->len)) return ERR_NOSPACE;
    prog->buf[prog->len++] = pl0;
    if (!insert) prog->buf[prog->len] = _HPS_DMA_ENDVAL;
    return ERR_SUCCESS;
}
static inline HpsErr_t __HPS_DMA_copy2(HPSDmaProgram_t* prog, uint8_t pl0, uint8_t pl1, bool insert) {
    if ((prog->size < 2) || (prog->size - 2 < prog->len)) return ERR_NOSPACE;
    prog->buf[prog->len++] = pl0;
    prog->buf[prog->len++] = pl1;
    if (!insert) prog->buf[prog->len] = _HPS_DMA_ENDVAL;
    return ERR_SUCCESS;
}
static inline HpsErr_t __HPS_DMA_copy3(HPSDmaProgram_t* prog, uint8_t pl0, uint8_t pl1, uint8_t pl2, bool insert) {
    if ((prog->size < 3) || (prog->size - 3 < prog->len)) return ERR_NOSPACE;
    prog->buf[prog->len++] = pl0;
    prog->buf[prog->len++] = pl1;
    prog->buf[prog->len++] = pl2;
    if (!insert) prog->buf[prog->len] = _HPS_DMA_ENDVAL;
    return ERR_SUCCESS;
}
static inline HpsErr_t __HPS_DMA_copy6(HPSDmaProgram_t* prog, uint8_t pl0, uint8_t pl1, uint8_t pl2, uint8_t pl3, uint8_t pl4, uint8_t pl5, bool insert) {
    if ((prog->size < 6) || (prog->size - 6 < prog->len)) return ERR_NOSPACE;
    prog->buf[prog->len++] = pl0;
    prog->buf[prog->len++] = pl1;
    prog->buf[prog->len++] = pl2;
    prog->buf[prog->len++] = pl3;
    prog->buf[prog->len++] = pl4;
    prog->buf[prog->len++] = pl5;
    if (!insert) prog->buf[prog->len] = _HPS_DMA_ENDVAL;
    return ERR_SUCCESS;
}
#define __HPS_DMA_COPY(prog, insert, len, ...) __HPS_DMA_copy##len(prog, __VA_ARGS__ insert)
#define _HPS_DMA_COPY(prog, len, ...) __HPS_DMA_COPY(prog, false, len, __VA_ARGS__)
#define _HPS_DMA_COPY_INS(prog, insert, len, ...) __HPS_DMA_COPY(prog, insert, len, __VA_ARGS__)


#define HPS_DMA_INST_CNDFLAG(v,ofs)     (((v) & 0x1) << ofs)

//
// Channel Thread Instructions
//

// Add half-word to Source or Destination address register
#define HPS_DMA_INSTCH_DMAADDH_LEN                  3
#define HPS_DMA_INSTCH_DMAADDH(isDest,halfWord)     _HPS_DMA_INST_APPEND8(0x55 | HPS_DMA_INST_CNDFLAG(isDest,1)), _HPS_DMA_INST_APPEND16(halfWord),
static inline HpsErr_t HPS_DMA_instChDMAADDH(HPSDmaProgram_t* prog, bool isDest, uint16_t halfWord) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAADDH_LEN, HPS_DMA_INSTCH_DMAADDH(isDest,halfWord));
}

// End of sequence
#define HPS_DMA_INSTCH_DMAEND_LEN                   1
#define HPS_DMA_INSTCH_DMAEND()                     _HPS_DMA_INST_APPEND8(_HPS_DMA_ENDVAL),
static inline HpsErr_t HPS_DMA_instChDMAEND(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAEND_LEN, HPS_DMA_INSTCH_DMAEND());
}

// Flush Peripheral to reload its level status
#define HPS_DMA_INSTCH_DMAFLUSHP_LEN                2
#define HPS_DMA_INSTCH_DMAFLUSHP(periph)            _HPS_DMA_INST_APPEND8(0x35), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMAFLUSHP(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAFLUSHP_LEN, HPS_DMA_INSTCH_DMAFLUSHP(periph));
}

// Perform a DMA load from the source address
#define HPS_DMA_INSTCH_DMALD_LEN                    1
#define HPS_DMA_INSTCH_DMALD()                      _HPS_DMA_INST_APPEND8(0x04 | HPS_DMA_REQCOND_NONE),
static inline HpsErr_t HPS_DMA_instChDMALD(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMALD_LEN, HPS_DMA_INSTCH_DMALD());
}

// Perform a DMA load from the source address conditially on configured `request_type` being single xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALDS_LEN                   1
#define HPS_DMA_INSTCH_DMALDS()                     _HPS_DMA_INST_APPEND8(0x04 | HPS_DMA_REQCOND_SINGLE),
static inline HpsErr_t HPS_DMA_instChDMALDS(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMALDS_LEN, HPS_DMA_INSTCH_DMALDS());
}

// Perform a DMA load from the source address conditially on configured `request_type` being burst xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALDB_LEN                   1
#define HPS_DMA_INSTCH_DMALDB()                     _HPS_DMA_INST_APPEND8(0x04 | HPS_DMA_REQCOND_BURST),
static inline HpsErr_t HPS_DMA_instChDMALDB(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMALDB_LEN, HPS_DMA_INSTCH_DMALDB());
}

// Perform a DMA load from the peripheral conditially on configured `request_type` being single xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALDPS_LEN                  2
#define HPS_DMA_INSTCH_DMALDPS(periph)              _HPS_DMA_INST_APPEND8(0x24 | HPS_DMA_REQCOND_SINGLE), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMALDPS(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMALDPS_LEN, HPS_DMA_INSTCH_DMALDPS(periph));
}

// Perform a DMA load from the peripheral conditially on configured `request_type` being burst xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALDPB_LEN                  2
#define HPS_DMA_INSTCH_DMALDPB(periph)              _HPS_DMA_INST_APPEND8(0x24 | HPS_DMA_REQCOND_BURST), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMALDPB(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMALDPB_LEN, HPS_DMA_INSTCH_DMALDPB(periph));
}

// Start of an execution loop with specified number of iterations (1 to 256) using specified counter (0 or 1)
//  - If bodyLen is != 0, indicates the body is already inserted at end of program, in which case it will be
//    shifted to make room for the loop start instruction.
#define HPS_DMA_INSTCH_DMALP_LEN                    2
#define HPS_DMA_INSTCH_DMALP(iter, ctr)             _HPS_DMA_INST_APPEND8(0x20 | HPS_DMA_INST_CNDFLAG(ctr,1)), _HPS_DMA_INST_APPEND8(iter),
static inline HpsErr_t HPS_DMA_instChDMALP(HPSDmaProgram_t* prog, uint8_t iter, HPSDmaTargetCounter ctr, unsigned int bodyLen) {
    // If we have a body, it must be moved to make room for the loop start instruction
    if (MaskCheck(prog->loopCtrUse, 0x1, ctr)) return ERR_INUSE; // Counter is in use, can't use again until loop is ended
    if (bodyLen > 0) {
        //Ensure there is space to move it
        if (prog->size - prog->len < HPS_DMA_INSTCH_DMALP_LEN) return ERR_NOSPACE;
        //Shift program pointer back to start of body.
        prog->len -= bodyLen;
        //And move the body forward enough bytes to insert our loop instruction.
        memmove(prog->buf + prog->len + HPS_DMA_INSTCH_DMALP_LEN, prog->buf + prog->len, bodyLen);
    }
    // Add or insert the instruction as required
    HpsErr_t status = _HPS_DMA_COPY_INS(prog, (bodyLen > 0), HPS_DMA_INSTCH_DMALP_LEN, HPS_DMA_INSTCH_DMALP(iter, ctr));
    // Shift the program pointer to the end of the body if we had one.
    prog->len += bodyLen;
    // If successfully inserted the loop start, increment our loop depth to ensure we have a matching close.
    if (ERR_IS_SUCCESS(status)) {
        prog->loopLvl++;
        prog->loopCtrUse = MaskSet(prog->loopCtrUse, 0x1, ctr);
    }
    return status;
}

// Start of an execution forever loop. Loop is broken when request_last flag was set during end check.
// This should only be used with peripherals as it is during a DMASTP[S|B] instruction that a peripheral may set this flag.
// This is not a real instruction, its for making it clear where a loop started.
#define HPS_DMA_INSTCH_DMALPFE_LEN                  0
#define HPS_DMA_INSTCH_DMALPFE()                    
static inline HpsErr_t HPS_DMA_instChDMALPFE(HPSDmaProgram_t* prog) {
    prog->loopLvl++; // One more level of loop opened. No counter used by forever loop.
    return HPS_DMA_INSTCH_DMALPFE_LEN;
}

// Common end of an execution loop
// You must specify length of instructions within the loop (loop start included), max 255 bytes worth.
// You must also specify if DMALP or DMALPFE started the loop.
// Can select a condition (DMAPEND, DMAPENDS, or DMAPENDB)
#define _HPS_DMA_INSTCH_DMALPEND_LEN                2
#define _HPS_DMA_INSTCH_DMALPEND(jp, fvr, ctr, cnd) _HPS_DMA_INST_APPEND8(0x28 | HPS_DMA_INST_CNDFLAG(!(fvr), 4) | HPS_DMA_INST_CNDFLAG((ctr) || (fvr), 2) | (cnd)), _HPS_DMA_INST_APPEND8(jp),
static inline HpsErr_t _HPS_DMA_instChDMALPEND(HPSDmaProgram_t* prog, unsigned int lpStart, bool forever, HPSDmaTargetCounter ctr, HPSDmaRequestCondition cond) {
    // Ensure loop exists
    if (!prog->loopLvl) return ERR_NOTFOUND; // Not in a loop. Can't terminate.
    if (!forever && !MaskCheck(prog->loopCtrUse, 0x1, ctr)) return ERR_NOTFOUND; // Not forever loop, but requeseted loop counter is uninitialised.
    // Calculate jump required to get to start of loop
    signed int lpJump = lpStart - prog->len;
    if ((lpJump < -UINT8_MAX) || (lpJump > 0)) return ERR_OUTRANGE; // Loop body is too big.
    HpsErr_t status = _HPS_DMA_COPY(prog, _HPS_DMA_INSTCH_DMALPEND_LEN, _HPS_DMA_INSTCH_DMALPEND(lpJump, forever, ctr, cond));
    // If successful
    if (ERR_IS_SUCCESS(status)) {
        // Update loop level - we've left a loop.
        prog->loopLvl--;
        // And release counter if not forever loop.
        if (!forever) prog->loopCtrUse = MaskClear(prog->loopCtrUse, 0x1, ctr);
    }
    return status;
}

// End of an execution loop. This is kind of manual
#define HPS_DMA_INSTCH_DMALPEND_LEN                 _HPS_DMA_INSTCH_DMALPEND_LEN
#define HPS_DMA_INSTCH_DMALPEND(jump, forevr, ctr)  _HPS_DMA_INSTCH_DMALPEND(jump, forevr, ctr, HPS_DMA_REQCOND_NONE),
static inline HpsErr_t HPS_DMA_instChDMALPEND(HPSDmaProgram_t* prog, unsigned int lpStart, bool forever, HPSDmaTargetCounter ctr) {
    return _HPS_DMA_instChDMALPEND(prog, lpStart, forever, ctr, HPS_DMA_REQCOND_NONE);
}

// End of an execution loop, conditionally on configured `request_type` being single xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALPENDS_LEN                _HPS_DMA_INSTCH_DMALPEND_LEN
#define HPS_DMA_INSTCH_DMALPENDS(jump, forevr, ctr) _HPS_DMA_INSTCH_DMALPEND(jump, forevr, ctr, HPS_DMA_REQCOND_SINGLE),
static inline HpsErr_t HPS_DMA_instChDMALPENDS(HPSDmaProgram_t* prog, unsigned int lpStart, bool forever, HPSDmaTargetCounter ctr) {
    return _HPS_DMA_instChDMALPEND(prog, lpStart, forever, ctr, HPS_DMA_REQCOND_SINGLE);
}

// End of an execution loop, conditionally on configured `request_type` being burst xfer (else is no-op)
#define HPS_DMA_INSTCH_DMALPENDB_LEN                _HPS_DMA_INSTCH_DMALPEND_LEN
#define HPS_DMA_INSTCH_DMALPENDB(jump, forevr, ctr) _HPS_DMA_INSTCH_DMALPEND(jump, forevr, ctr, HPS_DMA_REQCOND_BURST),
static inline HpsErr_t HPS_DMA_instChDMALPENDB(HPSDmaProgram_t* prog, unsigned int lpStart, bool forever, HPSDmaTargetCounter ctr) {
    return _HPS_DMA_instChDMALPEND(prog, lpStart, forever, ctr, HPS_DMA_REQCOND_BURST);
}

// Move immediate value (32-bit) to register (HPSDmaRegTarget)
#define HPS_DMA_INSTCH_DMAMOV_LEN                   6
#define HPS_DMA_INSTCH_DMAMOV(reg, val)             _HPS_DMA_INST_APPEND8(0xBC), _HPS_DMA_INST_APPEND8((reg) & 0x7), _HPS_DMA_INST_APPEND32(val),
static inline HpsErr_t HPS_DMA_instChDMAMOV(HPSDmaProgram_t* prog, HPSDmaTargetReg reg, uint32_t val) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAMOV_LEN, HPS_DMA_INSTCH_DMAMOV(reg, val));
}

// No-Operation
#define HPS_DMA_INSTCH_DMANOP_LEN                   1
#define HPS_DMA_INSTCH_DMANOP()                     _HPS_DMA_INST_APPEND8(_HPS_DMA_NOPVAL),
static inline HpsErr_t HPS_DMA_instChDMANOP(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMANOP_LEN, HPS_DMA_INSTCH_DMANOP());
}

// Read memory barrier forces thread to wait until all DMALDx instructions pending for a channel have completed.
#define HPS_DMA_INSTCH_DMARMB_LEN                   1
#define HPS_DMA_INSTCH_DMARMB()                     _HPS_DMA_INST_APPEND8(0x12),
static inline HpsErr_t HPS_DMA_instChDMARMB(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMARMB_LEN, HPS_DMA_INSTCH_DMARMB());
}

// Assert event interrupt flag (0-31).
//  - Event HPS_DMA_EVENT_ABORTED cannot be used.
#define HPS_DMA_INSTCH_DMASEV_LEN                   2
#define HPS_DMA_INSTCH_DMASEV(event)                _HPS_DMA_INST_APPEND8(0x34), _HPS_DMA_INST_APPEND8((event) << 3),
static inline HpsErr_t HPS_DMA_instChDMASEV(HPSDmaProgram_t* prog, HPSDmaEventId event) {
    if ((event >= HPS_DMA_USER_EVENT_COUNT) || (event < HPS_DMA_EVENT_MIN)) return ERR_BADID;
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASEV_LEN, HPS_DMA_INSTCH_DMASEV(event));
}

// Perform a DMA store to the destination address
// Instructs the DMA controller to copy data from the fifo using a transfer as specified by `request_type`.
#define HPS_DMA_INSTCH_DMAST_LEN                    1
#define HPS_DMA_INSTCH_DMAST()                      _HPS_DMA_INST_APPEND8(0x08 | HPS_DMA_REQCOND_NONE),
static inline HpsErr_t HPS_DMA_instChDMAST(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAST_LEN, HPS_DMA_INSTCH_DMAST());
}

// Perform a DMA store to the destination address conditially on configured `request_type` being single xfer (else is no-op)
// Instructs the DMA controller to copy data from the fifo to the destination address using a single transfer.
#define HPS_DMA_INSTCH_DMASTS_LEN                   1
#define HPS_DMA_INSTCH_DMASTS()                     _HPS_DMA_INST_APPEND8(0x08 | HPS_DMA_REQCOND_SINGLE),
static inline HpsErr_t HPS_DMA_instChDMASTS(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASTS_LEN, HPS_DMA_INSTCH_DMASTS());
}

// Perform a DMA store to the destination address conditially on configured `request_type` being burst xfer (else is no-op)
// Instructs the DMA controller to copy data from the fifo to the destination address using a burst transfer.
#define HPS_DMA_INSTCH_DMASTB_LEN                   1
#define HPS_DMA_INSTCH_DMASTB()                     _HPS_DMA_INST_APPEND8(0x08 | HPS_DMA_REQCOND_BURST),
static inline HpsErr_t HPS_DMA_instChDMASTB(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASTB_LEN, HPS_DMA_INSTCH_DMASTB());
}

// Perform a DMA store and notify to the peripheral conditially on configured `request_type` being single xfer (else is no-op)
// Instructs the DMA controller to copy data from the fifo to the destination address using a single transfer.
// As part of this transfer, the datype[1] flag is asserted to instruct the peripheral the request is complete and to flush the content.
#define HPS_DMA_INSTCH_DMASTPS_LEN                  2
#define HPS_DMA_INSTCH_DMASTPS(periph)              _HPS_DMA_INST_APPEND8(0x28 | HPS_DMA_REQCOND_SINGLE), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMASTPS(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASTPS_LEN, HPS_DMA_INSTCH_DMASTPS(periph));
}

// Perform a DMA store and notify to the peripheral conditially on configured `request_type` being burst xfer (else is no-op)
// Instructs the DMA controller to copy data from the fifo to the destination address using a burst transfer.
// As part of this transfer, the datype[1] flag is asserted to instruct the peripheral the request is complete and to flush the content.
#define HPS_DMA_INSTCH_DMASTPB_LEN                  2
#define HPS_DMA_INSTCH_DMASTPB(periph)              _HPS_DMA_INST_APPEND8(0x28 | HPS_DMA_REQCOND_BURST), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMASTPB(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASTPB_LEN, HPS_DMA_INSTCH_DMASTPB(periph));
}

// Perform a DMA store of zeros to the destination address
#define HPS_DMA_INSTCH_DMASTZ_LEN                   1
#define HPS_DMA_INSTCH_DMASTZ()                     _HPS_DMA_INST_APPEND8(0x0C),
static inline HpsErr_t HPS_DMA_instChDMASTZ(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMASTZ_LEN, HPS_DMA_INSTCH_DMASTZ());
}

// Instruct thread to wait for event (0-31) to occur (e.g. caused by DMASEV from another thread)
#define HPS_DMA_INSTCH_DMAWFE_LEN                   2
#define HPS_DMA_INSTCH_DMAWFE(event, invCache)      _HPS_DMA_INST_APPEND8(0x36), _HPS_DMA_INST_APPEND8(((event) << 3) | HPS_DMA_INST_CNDFLAG(invCache,1)),
static inline HpsErr_t HPS_DMA_instChDMAWFE(HPSDmaProgram_t* prog, HPSDmaEventId event, bool invCache) {
    if ((event >= HPS_DMA_EVENT_COUNT) || (event < HPS_DMA_EVENT_MIN)) return ERR_BADID;
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAWFE_LEN, HPS_DMA_INSTCH_DMAWFE(event, invCache));
}

// Instruct thread to wait for peripheral (0-31) to occur (e.g. caused by DMASEV from another thread)
// This sets the `request_type` flag to nextReqType.
#define HPS_DMA_INSTCH_DMAWFP_LEN                   2
#define HPS_DMA_INSTCH_DMAWFP(periph, nextReqType)  _HPS_DMA_INST_APPEND8(0x30 | (nextReqType)), _HPS_DMA_INST_APPEND8((periph) << 3),
static inline HpsErr_t HPS_DMA_instChDMAWFP(HPSDmaProgram_t* prog, HPSDmaPeripheralId periph, HPSDmaRequestType nextReqType) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAWFP_LEN, HPS_DMA_INSTCH_DMAWFP(periph, nextReqType));
}

// Write memory barrier forces thread to wait until all DMASTx instructions pending for a channel have completed.
#define HPS_DMA_INSTCH_DMAWMB_LEN                   1
#define HPS_DMA_INSTCH_DMAWMB()                     _HPS_DMA_INST_APPEND8(0x13),
static inline HpsErr_t HPS_DMA_instChDMAWMB(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_INSTCH_DMAWMB_LEN, HPS_DMA_INSTCH_DMAWMB());
}

// Pack control parameters into DMOV command targetting CCR
#define HPS_DMA_INSTCH_DMAMOVCCR_LEN                (HPS_DMA_INSTCH_DMAMOV_LEN)
static inline HpsErr_t HPS_DMA_instChDMAMOVCCR(HPSDmaProgram_t* prog, PHPSDmaChCtlParams_t params) {
    uint32_t ccr = (MaskInsert(params->_srcAddrInc,     _HPS_DMA_CHCNTRL_SRCADRINC_MASK, _HPS_DMA_CHCNTRL_SRCADRINC_OFFS) |
                    MaskInsert(params->_srcBurstSize,   _HPS_DMA_CHCNTRL_SRCBRSTSZ_MASK, _HPS_DMA_CHCNTRL_SRCBRSTSZ_OFFS) |
                    MaskInsert(params->_srcBurstLen-1,  _HPS_DMA_CHCNTRL_SRCBRSTLN_MASK, _HPS_DMA_CHCNTRL_SRCBRSTLN_OFFS) |
                    MaskInsert(params->srcProtCtrl,     _HPS_DMA_CHCNTRL_SRCPROT_MASK,   _HPS_DMA_CHCNTRL_SRCPROT_OFFS  ) |
                    MaskInsert(params->srcCacheCtrl,    _HPS_DMA_CHCNTRL_SRCCACHE_MASK,  _HPS_DMA_CHCNTRL_SRCCACHE_OFFS ) |
                    MaskInsert(params->_destAddrInc,    _HPS_DMA_CHCNTRL_DSTADRINC_MASK, _HPS_DMA_CHCNTRL_DSTADRINC_OFFS) |
                    MaskInsert(params->_destBurstSize,  _HPS_DMA_CHCNTRL_DSTBRSTSZ_MASK, _HPS_DMA_CHCNTRL_DSTBRSTSZ_OFFS) |
                    MaskInsert(params->_destBurstLen-1, _HPS_DMA_CHCNTRL_DSTBRSTLN_MASK, _HPS_DMA_CHCNTRL_DSTBRSTLN_OFFS) |
                    MaskInsert(params->destProtCtrl,    _HPS_DMA_CHCNTRL_DSTPROT_MASK,   _HPS_DMA_CHCNTRL_DSTPROT_OFFS  ) |
                    MaskInsert(params->destCacheCtrl,   _HPS_DMA_CHCNTRL_DSTCACHE_MASK,  _HPS_DMA_CHCNTRL_DSTCACHE_OFFS ) |
                    MaskInsert(params->endian,          _HPS_DMA_CHCNTRL_ENDIANSWP_MASK, _HPS_DMA_CHCNTRL_ENDIANSWP_OFFS));
    return HPS_DMA_instChDMAMOV(prog, HPS_DMA_TARGET_CCR, ccr);
}

// Helper to create a loop
//  - Takes a premade program `body` of instructions to form the body of the loop
//    which will be copied `body->len` to the output program wrapped in the necessary
//    loop instructions. The maximum body length is 255 bytes.
//  - The number of iterations is passed as `iters`. This must be in the range
//    0 to 256, where 0 will select looping continuously until `request_last` flag is set.
//  - When using a set number of iterations (`iters` != 0), `cntr` specifies which of
//    the two loop counters to use.
static HpsErr_t HPS_DMA_instChLoop(HPSDmaProgram_t* prog, HPSDmaProgram_t* body, uint16_t iters, HPSDmaTargetCounter ctr, HPSDmaRequestCondition cond) {
    if (iters > 256) return 0;
    if (body->len > UINT8_MAX) return 0;
    // Initial instruction
    unsigned int lpStart = prog->len;
    if (!iters) {
        HPS_DMA_instChDMALPFE(prog);
    } else {
        HPS_DMA_instChDMALP(prog, iters, ctr, 0);
    }
    // Copy in the body
    if (prog->size - prog->len < body->len) return ERR_NOSPACE;
    memcpy(prog->buf + prog->len, body->buf, body->len);
    prog->len += body->len;
    // Add the end instruction
    return _HPS_DMA_instChDMALPEND(prog, lpStart, !iters, ctr, cond);
}

// Helper to insert loop into existing program.
//  - Takes a premade program where the body of the loop is already populated as the
//    final `bodyLen` instructions in the output program. The body of the program will
//    be moved if necessary to insert the loop instruction. The maximum body length is
//    255 bytes.
//  - The number of iterations is passed as `iters`. This must be in the range
//    0 to 256, where 0 will select looping continuously until `request_last` flag is set.
//  - When using a set number of iterations (`iters` != 0), `cntr` specifies which of
//    the two loop counters to use.
static HpsErr_t HPS_DMA_instChLoopInsert(HPSDmaProgram_t* prog, unsigned int bodyLen, uint16_t iters, HPSDmaTargetCounter ctr, HPSDmaRequestCondition cond) {
    if (iters > 256) return 0;
    if (bodyLen > UINT8_MAX) return 0;
    // Initial instruction. Loop start is set to start of body.
    unsigned int lpStart = prog->len - bodyLen;
    if (!iters) {
        HPS_DMA_instChDMALPFE(prog);
    } else {
        // Insert the loop instruction, accounting for existing body
        HPS_DMA_instChDMALP(prog, iters, ctr, bodyLen);
    }
    // Add the end instruction
    return _HPS_DMA_instChDMALPEND(prog, lpStart, !iters, ctr, cond);
}

//
// Manager Thread Instructions
//

// Start a transfer for the specified channel, optionally in non-secure mode, with PC address for channel instructions
#define HPS_DMA_MGRINST_DMAGO_LEN                   6
#define HPS_DMA_MGRINST_DMAGO(ch, nonSec, pcAddr)   _HPS_DMA_INST_APPEND8(0xA0 | HPS_DMA_INST_CNDFLAG(nonSec,1)), _HPS_DMA_INST_APPEND8((ch) & 0x7), _HPS_DMA_INST_APPEND32(pcAddr),
static inline HpsErr_t HPS_DMA_instMgrDMAGO(HPSDmaProgram_t* prog, HPSDmaChannelId ch, bool nonSec, uint32_t pcAddr) {
    return _HPS_DMA_COPY(prog, HPS_DMA_MGRINST_DMAGO_LEN, HPS_DMA_MGRINST_DMAGO(ch, nonSec, pcAddr));
}

// End of sequence
#define HPS_DMA_MGRINST_DMAEND_LEN      HPS_DMA_INSTCH_DMAEND_LEN
#define HPS_DMA_MGRINST_DMAEND          HPS_DMA_INSTCH_DMAEND
static inline HpsErr_t HPS_DMA_instMgrDMAEND(HPSDmaProgram_t* prog) __attribute__((alias("HPS_DMA_instChDMAEND")));

// No-Operation
#define HPS_DMA_MGRINST_DMANOP_LEN      HPS_DMA_INSTCH_DMANOP_LEN
#define HPS_DMA_MGRINST_DMANOP          HPS_DMA_INSTCH_DMANOP
static inline HpsErr_t HPS_DMA_instMgrDMANOP(HPSDmaProgram_t* prog) __attribute__((alias("HPS_DMA_instChDMANOP")));

// Send Event
//  - Event HPS_DMA_EVENT_ABORTED cannot be used.
#define HPS_DMA_MGRINST_DMASEV_LEN      HPS_DMA_INSTCH_DMASEV_LEN
#define HPS_DMA_MGRINST_DMASEV          HPS_DMA_INSTCH_DMASEV
static inline HpsErr_t HPS_DMA_instMgrDMASEV(HPSDmaProgram_t* prog, HPSDmaEventId event) __attribute__((alias("HPS_DMA_instChDMASEV"))); 

// Wait for Event
#define HPS_DMA_MGRINST_DMAWFE_LEN      HPS_DMA_INSTCH_DMAWFE_LEN
#define HPS_DMA_MGRINST_DMAWFE          HPS_DMA_INSTCH_DMAWFE
static inline HpsErr_t HPS_DMA_instMgrDMAWFE(HPSDmaProgram_t* prog, HPSDmaEventId event, bool invCache) __attribute__((alias("HPS_DMA_instChDMAWFE")));


//
// Debug Iface Issued Instructions
//

// Terminate execution of a thread (ch or mgr)
#define HPS_DMA_DBGINST_DMAKILL_LEN     1
#define HPS_DMA_DBGINST_DMAKILL()       _HPS_DMA_INST_APPEND8(0x01),
static inline HpsErr_t HPS_DMA_instDbgDMAKILL(HPSDmaProgram_t* prog) {
    return _HPS_DMA_COPY(prog, HPS_DMA_DBGINST_DMAKILL_LEN, HPS_DMA_DBGINST_DMAKILL());
}


/*
 * Program Helpers
 */

// Re-initialse an existing program structure
static HpsErr_t HPS_DMA_initialiseProgram(PHPSDmaProgram_t prog);

// Allocate a new program structure
static HpsErr_t HPS_DMA_allocateProgram(unsigned int maxSize, bool autoFree, PHPSDmaProgram_t* pProg);

// Free a program structure
//  - if onlyAuto is true, will only perform free operation if *pProg->autoFree is true.
static HpsErr_t HPS_DMA_freeProgram(PHPSDmaProgram_t* pProg, bool onlyAuto);

#endif /* HPS_DMACONTROLLERPROGRAM_H_ */
