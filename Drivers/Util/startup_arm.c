/*
 * ARMv7 HPS Startup Routines
 * --------------------------
 * 
 * Provides startup code including vector table, interrupt
 * stack initialisation, and various other initial routines
 * for the ARMv7 processors on Altera HPS FPGA Devices.
 *
 * IRQ Stacks
 * ----------
 *
 * To initialise the IRQ stacks, the driver must know where
 * to place them in memory. This is achieved by globally
 * defining either the macro IRQ_STACK_LIMIT or macro
 * IRQ_STACK_SCATTER. The former indicates the exact
 * memory address to place the top of the IRQ stack at:
 *
 *      -D IRQ_STACK_LIMIT=0xFFFFFFF8
 *
 * The latter indicates the name of a region in the scatter
 * file to use, for example if you have a region called
 * IRQ_STACKS in your scatter file, then define:
 *
 *      -D IRQ_STACK_SCATTER=IRQ_STACKS
 *
 * Additionally the size of each IRQ stack (there are five
 * in total) is set by globally defining IRQ_STACK_SIZE
 * to be the size of each individual stack. The region in
 * which the stack is initialised must contain enough room
 * for 5*IRQ_STACK_SIZE bytes to be allocated.
 *
 *      -D IRQ_STACK_SIZE=0x100
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+------------------------------------
 * 31/03/2024 | Split out semi-hosting handler
 * 31/01/2024 | Correct ISR attributes
 * 14/01/2023 | Split startup routines from IRQ
 *
 */

#ifdef __arm__

#include "Util/lowlevel.h"
#include "Util/macros.h"
#include "Util/bit_helpers.h"
#include "Util/semihosting.h"
#include "Util/watchdog.h"

#include <stdio.h>
#include <stdbool.h>

//Default size of HPS IRQ stacks. Must be power of two.
//There are five such stacks, one for each IRQ mode (excluding app mode)
#ifndef IRQ_STACK_SIZE
#define IRQ_STACK_SIZE 0x800
#endif

//Location of HPS IRQ stacks in memory.
//Could be exact address (e.g  0xFFFFFFF8  ), or scatter file entry (e.g. IRQ_STACKS  )
#if !defined(IRQ_STACK_LIMIT) && !defined(IRQ_STACK_SCATTER)
#define IRQ_STACK_SCATTER IRQ_STACKS
#endif

/*
 * Add the default handler for unused ISRs.
 *
 * This does nothing. Will eventually trigger watchdog reset. Can also breakpoint here.
 * Alternatively can be defined to jump to reset vector
 */

#ifdef DEFAULT_ISR_JUMP_TO_ENTRY
void __default_isr (void) __attribute__ ((alias("__reset_isr"))); // @suppress("Unused function declaration")
#else

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra"
void __irq __default_isr (void) {
    if (checkIfSemihostingConnected()) {
        printf("!EXCEPTION! In %s Mode.\n", __name_proc_state(__current_proc_state()));
    }
    while(1);
}
#pragma clang diagnostic pop

#endif

/* Exception Vector Handlers
 *
 * The "weak, alias" directive means that unless defined, calls to these will
 * actually end up calling the "default_isr" function. To make use of these
 * handlers, you simply have to define them, for example, adding anywhere in
 * your code:
 *
 *    __fiq void fiq_isr (void) {
 *        //My handler code
 *    }
 *
 * And this will automatically override the default vector.
 */

// Reset Handler
void __reset_isr (void) __attribute__ ((noreturn, naked));
// SWI Handler
__swi   void __svc_isr   (void) __attribute__ ((naked));
// Other Interrupt Handlers. Can be overridden
__undef void __undef_isr (void) __attribute__ ((weak, alias("__default_isr")));
__abort void __pftcAb_isr(void) __attribute__ ((weak, alias("__default_isr")));
__abort void __dataAb_isr(void) __attribute__ ((weak, alias("__default_isr")));
__irq   void __irq_isr   (void) __attribute__ ((weak, alias("__default_isr")));
__fiq   void __fiq_isr   (void) __attribute__ ((weak, alias("__default_isr")));
// User software interrupt handler. Can be overridden
HpsErr_t   __svc_handler (unsigned int id, unsigned int argc, unsigned int* argv) __attribute__ ((weak));
// User semi-host handler.
//  - Id is either SVC_ID_SEMIHOST_ARM or SVC_ID_SEMIHOST_THUMB.
//  - val[0] is Op ID, and also status return value.
//  - val[1-3] are operation specific arguments.
signed int __svc_semihost(unsigned int id, unsigned int* val) __attribute__ ((weak));
// Semihosting enabled global flag. "See Util/semihosting.c"
extern volatile unsigned int __semihostingEnabled;

/*
 * Here we create a vector table using an inline assembly function. This
 * function is not compiled using the C compiler, but fed directly to the
 * Assembler. In the table we have entries of:
 *   LDR PC,=<functionName> ; Sets Page Counter equal to function address.
 */
__attribute__((naked, section("vector_table"))) void __vector_table(void) {
    __asm volatile (
        //Vector Table Entries
        "LDR  PC,=__reset_isr  ;" // 0x00 Reset
        "LDR  PC,=__undef_isr  ;" // 0x04 Undefined Instructions
        "LDR  PC,=__svc_isr    ;" // 0x08 Software Interrupts (SWI)
        "LDR  PC,=__pftcAb_isr ;" // 0x0C Prefetch Abort
        "LDR  PC,=__dataAb_isr ;" // 0x10 Data Abort
        "NOP                   ;" // 0x14 RESERVED
        "LDR  PC,=__irq_isr    ;" // 0x18 IRQ
        "LDR  PC,=__fiq_isr    ;" // 0x1C FIQ
    );
}

#if defined(IRQ_STACK_SCATTER)

#define IRQ_STACK_TOP_LINK SCATTER_REGION_LIMIT(IRQ_STACK_SCATTER,ZI)
#define IRQ_STACK_TOP (unsigned int)&IRQ_STACK_TOP_LINK

extern unsigned int IRQ_STACK_TOP_LINK;

#else

#define IRQ_STACK_TOP (unsigned int)IRQ_STACK_LIMIT

#endif

/*
 * Main Reset Entry Point
 *
 * This does some initialisation of the vector table address, and then calls
 * the programs main entry point.
 */

void __init_stacks (void) __attribute__ ((noreturn));
void __main(void) __attribute__((noreturn));

void __reset_isr (void) {
    // Mask fast and normal interrupts
    __disable_bothirqs();
    // Move to system mode (don't use USR mode as bare-metal not OS)
    __SET_PROC_STATE(PROC_STATE_SYS);
    // Initialise a temporary stack needed by the __init_stacks function
    __INIT_SP_SYS(IRQ_STACK_TOP);
    // And continue
    __BRANCH(__init_stacks);
}

// Masks for checking/configuring Co-Proc registers
#define SCTLR_REQCLR_MASK   ((1 << SYSREG_SCTLR_BIT_A) | (1 << SYSREG_SCTLR_BIT_V))  // Need V and A bits clear
#define CPACR_REQCLR_MASK   ((1 << SYSREG_CPACR_BIT_D32DIS) | (1 << SYSREG_CPACR_BIT_ASEDIS)) // Must have ASEDIS/D32DIS clear
#define CPACR_REQSET_MASK   ((SYSREG_CPACR_MASK_NS << SYSREG_CPACR_BIT_CP(10)) | /* Must enable User and Privileged access for CP10 */ \
                             (SYSREG_CPACR_MASK_NS << SYSREG_CPACR_BIT_CP(11)))  /* Must enable User and Privileged access for CP11*/

void __init_stacks (void) {
    // Set the location of the vector table using VBAR register
    __SET_SYSREG(SYSREG_COPROC, VBAR, (unsigned int)&__vector_table);
    // Enable VBAR and non-aligned access by clearing the V (bit 13) and
    // A bit (bit 1) of the SCTLR register if set.
    unsigned int sctlr = __GET_SYSREG(SYSREG_COPROC, SCTLR);
    if (sctlr & SCTLR_REQCLR_MASK) {
        sctlr &= ~SCTLR_REQCLR_MASK;
        __SET_SYSREG(SYSREG_COPROC, SCTLR, sctlr);
    }
    // Enable access to the CP10/CP11 co-processors. These are used
    // for some SIMD and VFP type instructions
    unsigned int cpacr = __GET_SYSREG(SYSREG_COPROC, CPACR);
    if ((cpacr & CPACR_REQCLR_MASK) || (~cpacr & CPACR_REQSET_MASK)) {
        cpacr |= CPACR_REQSET_MASK;
        cpacr &= ~CPACR_REQCLR_MASK;
        __SET_SYSREG(SYSREG_COPROC, CPACR, cpacr); // Store new access permissions into CPACR
        __ISB();  // Synchronise any branch prediction so that effects are now visible
    }
    // Reset the debugger check flag
    __semihostingEnabled = SVC_HAS_DEBUGGER;
    // Initialise all IRQ stack pointers
    unsigned int stackTop = IRQ_STACK_TOP;
    // FIQ
    __INIT_SP_MODE(PROC_STATE_FIQ, stackTop);
    // IRQ
    __INIT_SP_MODE(PROC_STATE_IRQ, stackTop - IRQ_STACK_SIZE);
    // SVC
    __INIT_SP_MODE(PROC_STATE_SVC, stackTop - 2*IRQ_STACK_SIZE);
    // Abort
    __INIT_SP_MODE(PROC_STATE_ABT, stackTop - 3*IRQ_STACK_SIZE);
    // Undefined
    __INIT_SP_MODE(PROC_STATE_UND, stackTop - 4*IRQ_STACK_SIZE);
    // If program is compiled targeting a hardware VFP, we must
    // enable the floating point unit to prevent run-time errors
    // in the C standard library.
#if defined(__ARM_PCS_VFP) || defined(__TARGET_FPU_VFP)
    __SET_PROC_FPEXC(1 << __PROC_FPEXC_BIT_EN);  // Enable VFP and SIMD extensions
#endif
    // Launch the C entry point
    __main();
}


/*
 * Software Interrupt Handler
 *
 * The SVC vector is used by the debugger to run semi-hosting
 * commands which allow IO commands to send data to the debugger
 * e.g. using printf.
 *
 * When the debugger is not connected, we still need to handle
 * this SVC call otherwise the processor will hang. As we have no
 * other SVC calls by default, we can handle by simply returning.
 */

// Argument remapping for custom SVC handler
//  - maps args to argc/argv used by handler routine.
HpsErr_t  __svc_handlerMap(unsigned int id, unsigned int* args) {
    return __svc_handler(id, args[0], args+1);
}

__swi void __svc_isr(void) {
    // Store the four registers r0-r3 to the stack. These contain any parameters
    // to be passed to the SVC handler. Also store r12 as we use that to hold the
    // return value temporarily during context cleanup. We push the link register
    // which will be popped back to the page counter on return.
    // SVC may have up to four inputs, passed in via r0-r3 (could be used as pointers
    // if more is required). They may return a value via r0 which should be considered
    // clobbered by the caller.
    __asm volatile (
        "STMFD   SP!, {r12, LR}                ;"
        "PUSH    {r0-r3}                       ;"
        // Grab the stack pointer as this is the address in RAM where our four parameters have been saved
        "MOV      r1, SP                       ;"
        // Grab SPSR. We will restore this at the end, but also need it to see if
        // the caller was in thumb mode. We store it to the stack to save for later
        "MRS      r0, SPSR                     ;"
        "PUSH    {r0, r3}                      ;"
        // Extract the SVC ID. This is embedded in the SVC instruction itself which
        // is located one instruction before the value of the current banked link register.
        "TST      r0, %[TMask]                 ;"
        // If caller was in thumb, then SVC instruction is 2-byte, with lower byte being ID. Load also the thumb semi-hosting ID to r3.
        "LDRHNE   r0, [LR,#-2]                 ;"
        "BICNE    r0, r0, #0xFF00              ;"
        "MOVNE    r3, %[SvcIdT]                ;"
        // Otherwise caller was in arm mode, then SVC instruction is 4-byte, with lower three bytes being ID. Load also the ARM semi-hosting ID to r3.
        "LDREQ    r0, [LR,#-4]                 ;"
        "BICEQ    r0, r0, #0xFF000000          ;"
        "MOVWEQ   r3, %[SvcIdAL]               ;"
        "MOVTEQ   r3, %[SvcIdAH]               ;"
        // Check if this was the semihosting ID
        "CMP      r0, r3                       ;"
        // If this was not a semi-hosting ID, call user handler (r0 is first parameter [ID], r1 is second parameter [SP])
        "BLNE     __svc_handlerMap             ;"
        // If this is a semi-hosting ID, and we are in this handler, the debugger is not connected, so call fake semi-hosting 
        // handler (r0 is first parameter [ID], r1 is second parameter [SP])
        "BLEQ     __svc_semihost               ;"
        // Backup return value to r12 as r0 will get clobbered during stack cleanup.
        "MOV     r12, r0                       ;"
        // Clear the debugger connected flag if this was a semi-hosting ID.
        "MOVEQ    r1, %[NoDbggr]               ;"
        "LDREQ    r0, =__semihostingEnabled    ;"
        "STREQ    r1, [r0]                     ;"
        // Restore the processor state from before the SVC was triggered. We have this value saved on the stack.
        "POP     {r0, r3}                      ;"
        "MSR     SPSR_cxsf, r0                 ;"
        // Remove pushed registers from stack
        "POP     {r0-r3}                       ;"
        // Restore the return status value to r0.
        "MOV      r0, r12                      ;"
        // Remove pushed registers from stack, and return, restoring SPSR to CPSR
        "LDMFD   SP!, {r12, PC}^               ;"
        ::
        [TMask]   "i" (1 << __PROC_CPSR_BIT_T),
        [SvcIdT]  "i" (SVC_ID_SEMIHOST_THUMB),                // Thumb value is 8bit, so can load directly with MOV.
        [SvcIdAL] "i" ((SVC_ID_SEMIHOST_ARM      ) & 0xFFFF), // Arm value is 32bit, so must split into two 16-bit
        [SvcIdAH] "i" ((SVC_ID_SEMIHOST_ARM >> 16) & 0xFFFF), // half words for MOVW/MOVT instructions to work.
        [NoDbggr] "i" (SVC_NO_DEBUGGER)
    );
}

// Default SVC semi-host handler returns success
signed int __svc_semihost(unsigned int id, unsigned int* val) {
    return SEMIHOST_SUCCESS;
}

// Default user SVC handler is No-Op
HpsErr_t __svc_handler(unsigned int id, unsigned int argc, unsigned int* argv) {
    return ERR_SUCCESS;
}

#endif

