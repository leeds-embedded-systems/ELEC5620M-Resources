/*
 * HPS DMA Controller Registers
 * ----------------------------
 *
 * Register map for an ARM AMBA DMA-330
 * controller as is available in the Cyclone V
 * and Arria 10 HPS bridges.
 * 
 * This is not intended as a user header. It is
 * only to be included by HPS_DMAController.c
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

#ifndef HPS_DMACONTROLLERREGS_H_
#define HPS_DMACONTROLLERREGS_H_

#include "HPS_DMAController/HPS_DMAControllerProgram.h"

typedef enum {
    HPS_DMA_MGRSTAT_STOPPED   = 0x0,
    HPS_DMA_MGRSTAT_EXECUTING = 0x1,
    HPS_DMA_MGRSTAT_CACHEMISS = 0x2,
    HPS_DMA_MGRSTAT_UPDATEPC  = 0x3,
    HPS_DMA_MGRSTAT_EVNTWAIT  = 0x4,
    HPS_DMA_MGRSTAT_FAULTING  = 0xF
} HPSDmaMgrStatus;

typedef enum {
    HPS_DMA_CHSTAT_STOPPED   = 0x0,
    HPS_DMA_CHSTAT_EXECUTING = 0x1,
    HPS_DMA_CHSTAT_CACHEMISS = 0x2,
    HPS_DMA_CHSTAT_UPDATEPC  = 0x3,
    HPS_DMA_CHSTAT_EVNTWAIT  = 0x4,
    HPS_DMA_CHSTAT_ATBARRIER = 0x5,
    HPS_DMA_CHSTAT_PERIPWAIT = 0x7,
    HPS_DMA_CHSTAT_KILLING   = 0x8,
    HPS_DMA_CHSTAT_CMPLETNG  = 0x9,
    HPS_DMA_CHSTAT_FLTCPLTNG = 0xE,
    HPS_DMA_CHSTAT_FAULTING  = 0xF
} HPSDmaChStatus;


/*
 * Register Maps
 */

// Main Register Map Offsets
enum {
    HPS_DMA_REG_CNTRL      = 0x0,
    HPS_DMA_REG_THREADSTAT = 0x100,
    HPS_DMA_REG_AXISTAT    = 0x400,
    HPS_DMA_REG_DEBUG      = 0xD00,
    HPS_DMA_REG_CONFIG     = 0xE00,
    //Total size
    HPS_DMA_REG_SIZE = 0x1000
};

/* DMA Control Registers */

// [RO] Status of the DMA controller manager
#define HPSDMA_REG_CTRL_MGRSTAT(base)         (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x00) / sizeof(uint32_t))))
// [RO] Program counter for the controller manager
#define HPSDMA_REG_CTRL_DMAPC(base)           (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x04) / sizeof(uint32_t))))
// [RW] Whether event triggers IRQ. One bit for each of the 32 events
#define HPSDMA_REG_CTRL_IRQEN(base)           (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x20) / sizeof(uint32_t))))
// [RO] Unmasked status flags. One bit per event
#define HPSDMA_REG_CTRL_IRQSTATRAW(base)      (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x24) / sizeof(uint32_t))))
// [RO] Whether event was source of IRQ. One bit per event
#define HPSDMA_REG_CTRL_IRQFLAGS(base)        (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x28) / sizeof(uint32_t))))
// [WO] Clear IRQ flag for event. One bit per event.
#define HPSDMA_REG_CTRL_IRQCLEAR(base)        (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x2C) / sizeof(uint32_t))))
// [RO] Whether DMA manager is in fault state
#define HPSDMA_REG_CTRL_MGRFAULTSTAT(base)    (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x30) / sizeof(uint32_t))))
// [RO] Whether channels are in fault state. One bit for each of the 8 channels
#define HPSDMA_REG_CTRL_CHFAULTSTAT(base)     (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x34) / sizeof(uint32_t))))
// [RO] Information about fault that occurred in DMA manager thread
#define HPSDMA_REG_CTRL_MGRFAULTTYPE(base)    (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x38) / sizeof(uint32_t))))
// [RO] Information about fault that occurred in DMA channel thread (one per ch)
#define _HPSDMA_REG_CTRL_CHFAULTTYPE_LEN (1 * sizeof(uint32_t))
#define HPSDMA_REG_CTRL_CHFAULTTYPE(base, ch) (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CNTRL + 0x40) / sizeof(uint32_t))) + ((ch) * _HPSDMA_REG_CTRL_CHFAULTTYPE_LEN / sizeof(uint32_t)))

#define HPSDMA_CTRL_MGRSTAT_DMASTAT_MASK       0xF  // Current status of DMA manager thread
#define HPSDMA_CTRL_MGRSTAT_DMASTAT_OFFS       0
#define HPSDMA_CTRL_MGRSTAT_WAKEUP_MASK        0x1F // Which event the DMA manager is waiting for
#define HPSDMA_CTRL_MGRSTAT_WAKEUP_OFFS        4
#define HPSDMA_CTRL_MGRSTAT_ISSEC_MASK         0x1  // Whether this is the secure DMA controller
#define HPSDMA_CTRL_MGRSTAT_ISSEC_OFFS         9
#define HPSDMA_CTRL_IRQ_MASK                   HPS_DMA_IRQ_ALL
#define HPSDMA_CTRL_IRQ_OFFS                   0


/* Thread Status Registers */

#define _HPSDMA_REG_THREADSTAT_CH_LEN (2 * sizeof(uint32_t))
// [RO] Channel status registers (see defines below)
#define HPSDMA_REG_THREADSTAT_CHSTAT(base, ch)   (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_THREADSTAT + 0x00) / sizeof(uint32_t)) + ((ch) * _HPSDMA_REG_THREADSTAT_CH_LEN / sizeof(uint32_t))))
// [RO] Current program counter value for this channel
#define HPSDMA_REG_THREADSTAT_CHPC(base, ch)     (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_THREADSTAT + 0x04) / sizeof(uint32_t)) + ((ch) * _HPSDMA_REG_THREADSTAT_CH_LEN / sizeof(uint32_t))))

#define HPSDMA_THREADSTAT_CHSTAT_DMASTAT_MASK   0xF  // Current status of DMA channel thread
#define HPSDMA_THREADSTAT_CHSTAT_DMASTAT_OFFS   0
#define HPSDMA_THREADSTAT_CHSTAT_WAKEUP_MASK    0x1F // Which event the DMA channel is waiting for
#define HPSDMA_THREADSTAT_CHSTAT_WAKEUP_OFFS    4
#define HPSDMA_THREADSTAT_CHSTAT_ISBURST_MASK   0x1  // Whether the burst bit was set when executing DMAWFP
#define HPSDMA_THREADSTAT_CHSTAT_ISBURST_OFFS   14
#define HPSDMA_THREADSTAT_CHSTAT_ISPERIPH_MASK  0x1  // Whether a peripheral was targetted when executing DMAWFP
#define HPSDMA_THREADSTAT_CHSTAT_ISPERIPH_OFFS  15
#define HPSDMA_THREADSTAT_CHSTAT_NONSEC_MASK    0x1  // Whether this channel opperates in non-secure state
#define HPSDMA_THREADSTAT_CHSTAT_NONSEC_OFFS    21


/* AXI Status Registers */

#define _HPSDMA_REG_AXISTAT_CH_LEN (8 * sizeof(uint32_t))
// [RO] Current source address in use by channel
#define HPSDMA_REG_AXISTAT_SRCADDR(base, ch)         (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_AXISTAT + 0x00) / sizeof(uint32_t)) + ((ch) * _HPSDMA_REG_AXISTAT_CH_LEN / sizeof(uint32_t))))
// [RO] Corrent destination address in use by channel
#define HPSDMA_REG_AXISTAT_DESTADDR(base, ch)        (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_AXISTAT + 0x04) / sizeof(uint32_t)) + ((ch) * _HPSDMA_REG_AXISTAT_CH_LEN / sizeof(uint32_t))))
// [RO] Channel control register for this channel (see defines below)
#define HPSDMA_REG_AXISTAT_CHCNTRL(base, ch)         (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_AXISTAT + 0x08) / sizeof(uint32_t)) + ((ch) * _HPSDMA_REG_AXISTAT_CH_LEN / sizeof(uint32_t))))
// [RO] Loop counter 0 and 1 iteration values if used
#define _HPSDMA_REG_AXISTAT_LOOPCNTR_LEN (1 * sizeof(uint32_t))
#define HPSDMA_REG_AXISTAT_LOOPCNTR(base, ch, ctr)   (*(((volatile uint8_t *)(base)) + ((HPS_DMA_REG_AXISTAT + 0x0C) / sizeof(uint8_t )) + ((ch) * _HPSDMA_REG_AXISTAT_CH_LEN / sizeof(uint8_t )) + ((ctr) * _HPSDMA_REG_AXISTAT_LOOPCNTR_LEN / sizeof(uint8_t))))

#define HPSDMA_AXISTAT_CHCNTRL_SRCADRINC_MASK  _HPS_DMA_CHCNTRL_SRCADRINC_MASK  // Whether to increment source address during burst
#define HPSDMA_AXISTAT_CHCNTRL_SRCADRINC_OFFS  _HPS_DMA_CHCNTRL_SRCADRINC_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_SRCBRSTSZ_MASK  _HPS_DMA_CHCNTRL_SRCBRSTSZ_MASK  // Size of source burst data. clog2 of number of bytes per beat. Max 16 bytes.
#define HPSDMA_AXISTAT_CHCNTRL_SRCBRSTSZ_OFFS  _HPS_DMA_CHCNTRL_SRCBRSTSZ_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_SRCBRSTLN_MASK  _HPS_DMA_CHCNTRL_SRCBRSTLN_MASK  // Length of burst. Number of read transfers to perform in a burst. Max 16 xfers.
#define HPSDMA_AXISTAT_CHCNTRL_SRCBRSTLN_OFFS  _HPS_DMA_CHCNTRL_SRCBRSTLN_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_SRCPROT_MASK    _HPS_DMA_CHCNTRL_SRCPROT_MASK    // Protocol setting (ARPROT) for source AXI transfer.
#define HPSDMA_AXISTAT_CHCNTRL_SRCPROT_OFFS    _HPS_DMA_CHCNTRL_SRCPROT_OFFS  
#define HPSDMA_AXISTAT_CHCNTRL_SRCCACHE_MASK   _HPS_DMA_CHCNTRL_SRCCACHE_MASK   // Cache setting (ARCACHE) for source AXI transfer.
#define HPSDMA_AXISTAT_CHCNTRL_SRCCACHE_OFFS   _HPS_DMA_CHCNTRL_SRCCACHE_OFFS 
#define HPSDMA_AXISTAT_CHCNTRL_DSTADRINC_MASK  _HPS_DMA_CHCNTRL_DSTADRINC_MASK  // Whether to increment destination address during burst
#define HPSDMA_AXISTAT_CHCNTRL_DSTADRINC_OFFS  _HPS_DMA_CHCNTRL_DSTADRINC_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_DSTBRSTSZ_MASK  _HPS_DMA_CHCNTRL_DSTBRSTSZ_MASK  // Size of destination burst data. clog2 of number of bytes per beat. Max 16 bytes.
#define HPSDMA_AXISTAT_CHCNTRL_DSTBRSTSZ_OFFS  _HPS_DMA_CHCNTRL_DSTBRSTSZ_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_DSTBRSTLN_MASK  _HPS_DMA_CHCNTRL_DSTBRSTLN_MASK  // Length of burst. Number of write transfers to perform in a burst. Max 16 xfers.
#define HPSDMA_AXISTAT_CHCNTRL_DSTBRSTLN_OFFS  _HPS_DMA_CHCNTRL_DSTBRSTLN_OFFS
#define HPSDMA_AXISTAT_CHCNTRL_DSTPROT_MASK    _HPS_DMA_CHCNTRL_DSTPROT_MASK    // Protocol setting (AWPROT) for destination AXI transfer.
#define HPSDMA_AXISTAT_CHCNTRL_DSTPROT_OFFS    _HPS_DMA_CHCNTRL_DSTPROT_OFFS  
#define HPSDMA_AXISTAT_CHCNTRL_DSTCACHE_MASK   _HPS_DMA_CHCNTRL_DSTCACHE_MASK   // Cache setting (AWCACHE) for destination AXI transfer.
#define HPSDMA_AXISTAT_CHCNTRL_DSTCACHE_OFFS   _HPS_DMA_CHCNTRL_DSTCACHE_OFFS 
#define HPSDMA_AXISTAT_CHCNTRL_ENDIANSWP_MASK  _HPS_DMA_CHCNTRL_ENDIANSWP_MASK  // Swap data endianness if source and destination use different byte order.
#define HPSDMA_AXISTAT_CHCNTRL_ENDIANSWP_OFFS  _HPS_DMA_CHCNTRL_ENDIANSWP_OFFS

/* Debug Registers */

// [RO] Whether idle (see defines below)
#define HPSDMA_REG_DEBUG_STATUS(base)     (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_DEBUG + 0x00) / sizeof(uint32_t))))
// [WO] Debug command source (see defines below)
#define HPSDMA_REG_DEBUG_COMMAND(base)    (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_DEBUG + 0x04) / sizeof(uint32_t))))
// [WO] Debug instruction (see defines below)
#define HPSDMA_REG_DEBUG_INSTR_OFFS       sizeof(uint16_t)
#define HPSDMA_REG_DEBUG_INSTR_LEN        (2 * sizeof(uint32_t) - HPSDMA_REG_DEBUG_INSTR_OFFS)
#define HPSDMA_REG_DEBUG_INSTR_WORD(x)    (((x) + sizeof(uint16_t)) / sizeof(uint32_t))
#define HPSDMA_REG_DEBUG_INSTR(base, wrd) (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_DEBUG + 0x08) / sizeof(uint32_t)) + (wrd)))

#define HPSDMA_DEBUG_STATUS_ISBUSY_MASK  0x1  // Whether idle (1) or busy (1)
#define HPSDMA_DEBUG_STATUS_ISBUSY_OFFS  0

#define HPSDMA_DEBUG_COMMAND_EXECUTE     0    // Write this to the command register to execute INSTR[0:1] content

#define HPSDMA_DEBUG_INSTR0_THREAD_MASK  0x1  // Whether to debug manager (0) or channel (1)
#define HPSDMA_DEBUG_INSTR0_THREAD_OFFS  0
#define HPSDMA_DEBUG_INSTR0_CHANNEL_MASK 0x7  // When debugging channels, which channel is selected (0 - 7)
#define HPSDMA_DEBUG_INSTR0_CHANNEL_OFFS 8
#define HPSDMA_DEBUG_INSTR_BYTE_MASK     UINT8_MAX
#define HPSDMA_DEBUG_INSTR_BYTE_OFFS(x)  ((((x) + sizeof(uint16_t)) % sizeof(uint32_t)) * 8)


/* Config Registers */

// [RO] Config CR0-CR4 (see defines below)
//       - CR 0: Config Resource Counts
//       - CR 1: Config Instr Cache Size
//       - CR 2: Config Boot Address
//       - CR 3: Config Event Security
//       - CR 4: Config Peripheral Security
#define _HPSDMA_REG_CONFIG_CR_LEN  (1 * sizeof(uint32_t))
#define HPSDMA_REG_CONFIG_CR(base, cfg)    (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CONFIG + 0x00) / sizeof(uint32_t)) + ((cfg) * _HPSDMA_REG_CONFIG_CR_LEN / sizeof(uint32_t))))z
// [RO] DMA Data Configuration Register (see defines below)
#define HPSDMA_REG_DMACONFIG(base)         (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CONFIG + 0x14) / sizeof(uint32_t))))
// [RW] Watchdog Behaviour Register (see defines below)
#define HPSDMA_REG_WATCHDOG(base)          (*(((volatile uint32_t*)(base)) + ((HPS_DMA_REG_CONFIG + 0x80) / sizeof(uint32_t))))

#define HPSDMA_CONFIG_CR0 0
#define HPSDMA_CONFIG_CR0_PERIPHREQ_MASK     0x1  // Whether DMAC supports peripheral requests
#define HPSDMA_CONFIG_CR0_PERIPHREQ_OFFS     0
#define HPSDMA_CONFIG_CR0_BOOTEN_MASK        0x1  // Indicates the status of the boot_from_pc signal when the DMAC exited from reset
#define HPSDMA_CONFIG_CR0_BOOTEN_OFFS        1
#define HPSDMA_CONFIG_CR0_MGRNSATRST_MASK    0x1  // Indicates the status of the boot_manager_ns signal when the DMAC exited from reset
#define HPSDMA_CONFIG_CR0_MGRNSATRST_OFFS    2
#define HPSDMA_CONFIG_CR0_NUMCH_MASK         0x7  // Number of DMA channels (minus one, 0 = 1ch, ..., 7 = 8ch)
#define HPSDMA_CONFIG_CR0_NUMCH_OFFS         4
#define HPSDMA_CONFIG_CR0_NUMPERIPHREQ_MASK  0x1F // If (HPSDMA_CONFIG_CR0_PERIPHREQ) flag is set, then number of peripheral request interfaces (minus one, 0 = 1ch, ..., 31 = 32ch)
#define HPSDMA_CONFIG_CR0_NUMPERIPHREQ_OFFS  12
#define HPSDMA_CONFIG_CR0_NUMEVENTS_MASK     0x1F // Number of interrupt outputs (events) supported (minus one, 0 = 1 irq, ..., 31 = 32 irqs)
#define HPSDMA_CONFIG_CR0_NUMEVENTS_OFFS     17

#define HPSDMA_CONFIG_CR1 1
#define HPSDMA_CONFIG_CR1_ICACHELEN_MASK     0x7  // The length of an i-cache line (2 = 4 bytes, ..., 5 = 32 bytes)
#define HPSDMA_CONFIG_CR1_ICACHELEN_OFFS     0
#define HPSDMA_CONFIG_CR1_ICACHELINES_MASK   0xF  // Number of i-cache lines (minus one, 0 = 1 line, ..., 15 = 16 lines)
#define HPSDMA_CONFIG_CR1_ICACHELINES_OFFS   4

#define HPSDMA_CONFIG_CR2 2
#define HPSDMA_CONFIG_CR2_BOOTADDR_MASK      UINT32_MAX // Provides the value of boot_addr[31:0] when the DMAC exited from reset
#define HPSDMA_CONFIG_CR2_BOOTADDR_OFFS      0

#define HPSDMA_CONFIG_CR3 3
#define HPSDMA_CONFIG_CR3_EVTNS_MASK         0x1  // Provides the security state of an event-interrupt resource (0 = secure, 1 = insecure)
#define HPSDMA_CONFIG_CR3_EVTNS_OFFS(evt)    (evt)

#define HPSDMA_CONFIG_CR4 4
#define HPSDMA_CONFIG_CR4_PERIPHNS_MASK      0x1  // Provides the security state of the peripheral request interfaces: (0 = secure, 1 = insecure)
#define HPSDMA_CONFIG_CR4_PERIPHNS_OFFS(pr)  (pr)

#define HPSDMA_DMACONFIG_DATAWIDTH_MASK      0x7   // The bus width of the AXI master (2 = 64 bit, ..., 4 = 128 bit)
#define HPSDMA_DMACONFIG_DATAWIDTH_OFFS      0
#define HPSDMA_DMACONFIG_WRITECAP_MASK       0x7   // Max number of outstanding write transactions (minus one, 0 = 1 xfer, ..., 7 = 8 xfers)
#define HPSDMA_DMACONFIG_WRITECAP_OFFS       4
#define HPSDMA_DMACONFIG_WRITEQUEUE_MASK     0xF   // The depth of the write queue (minus one, 0 = 1 line, ..., 15 = 16 lines)
#define HPSDMA_DMACONFIG_WRITEQUEUE_OFFS     8
#define HPSDMA_DMACONFIG_READCAP_MASK        0x7   // Max number of outstanding read transactions (minus one, 0 = 1 xfer, ..., 7 = 8 xfers)
#define HPSDMA_DMACONFIG_READCAP_OFFS        12
#define HPSDMA_DMACONFIG_READQUEUE_MASK      0xF   // The depth of the read queue (minus one, 0 = 1 line, ..., 15 = 16 lines)
#define HPSDMA_DMACONFIG_READQUEUE_OFFS      16
#define HPSDMA_DMACONFIG_DATABUFSIZE_MASK    0x3FF // The number of lines that the data buffer contains (minus one, 0 = 1 line, ..., 1023 = 1024 lines)
#define HPSDMA_DMACONFIG_DATABUFSIZE_OFFS    20

#define HPSDMA_WATCHDOG_IRQONLY_MASK         0x1  // Controls how the DMAC responds when it detects a lock-up condition (0 = abort all channels and set IRQ flag, 1 = set IRQ flag only)
#define HPSDMA_WATCHDOG_IRQONLY_OFFS         0

#endif /* HPS_DMACONTROLLERREGS_H_ */
