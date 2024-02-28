/*
 * startup.c
 *
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
 * 31/01/2024 | Correct ISR attributes
 * 14/01/2023 | Split startup routines from IRQ
 *
 */

#ifdef __arm__

#include "Util/lowlevel.h"
#include "Util/macros.h"

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
 * Semi-hosting Connected Check
 */

// We initially assume that semi-hosting is and connected. Before this is used
// we will trigger an SVC call to the semi-hosting ID. If the debugger is connected
// this SVC will be caught by it and not us. If it's not connected, our __svc_isr will
// be called which clears this flag. We can then return the flag value to show if
// connected or not.

#define SVC_ID_SEMIHOST_ARM   0x123456
#define SVC_ID_SEMIHOST_THUMB 0xAB

#define SVC_OP_SYS_ERRNO      0x13
#define SVC_OP_SYS_TIME       0x11

#define SVC_NO_DEBUGGER  0
#define SVC_HAS_DEBUGGER 1

__attribute__((used)) static volatile uint32_t __semihostingEnabled = SVC_HAS_DEBUGGER;

// Function to check if semi-hosting is connected to a debugger.
bool checkIfSemihostingConnected(void) {
    __asm volatile (
        "MOV  R1, SP       ;"
        "MOVS R0, %[svcOp] ;"
        "SVC  %[svcId]     ;"
        :
        : [svcId] "i" (SVC_ID_SEMIHOST_ARM),
          [svcOp] "i" (SVC_OP_SYS_TIME)
        : "r0", "r1"
    );
    return __semihostingEnabled;
}

/*
 * Add the default handler for unused ISRs.
 *
 * This does nothing. Will eventually trigger watchdog reset. Can also breakpoint here.
 * Alternatively can be defined to jump to reset vector
 */

#ifdef DEFAULT_ISR_JUMP_TO_ENTRY
void __default_isr (void) __attribute__ ((alias("__reset_isr"))); // @suppress("Unused function declaration")
#else

void __default_isr (void) { // No __isr attribute used as we never return.
    if (checkIfSemihostingConnected()) {
        printf("!EXCEPTION! In %s Mode.\n", __name_proc_state(__current_proc_state()));
    }
    while(1);
}
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
void __svc_handler (unsigned int id, unsigned int* val) __attribute__ ((weak));

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

#define __IRQ_STACK_TOP(x) Image$$##x##$$ZI$$Limit
#define _IRQ_STACK_TOP(x) __IRQ_STACK_TOP(x)

#define IRQ_STACK_TOP_LINK _IRQ_STACK_TOP(IRQ_STACK_SCATTER)
#define IRQ_STACK_TOP (unsigned int)&IRQ_STACK_TOP_LINK

extern unsigned int IRQ_STACK_TOP_LINK;

#else

#define __IRQ_STACK_TOP(x) (unsigned int)x
#define _IRQ_STACK_TOP(x) __IRQ_STACK_TOP(x)

#define IRQ_STACK_TOP _IRQ_STACK_TOP(IRQ_STACK_LIMIT)

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

void __init_stacks (void) {
    // Set the location of the vector table using VBAR register
    __SET_SYSREG(SYSREG_COPROC, VBAR, (unsigned int)&__vector_table);
    // Enable non-aligned access by clearing the A bit (bit 1) of the SCTLR register
    unsigned int sctlr = __GET_SYSREG(SYSREG_COPROC, SCTLR);
    sctlr &= ~(1 << SYSREG_SCTLR_BIT_A);
    __SET_SYSREG(SYSREG_COPROC, SCTLR, sctlr);
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
    //Inline assembly code for enabling FPU
    unsigned int cpacr = __GET_SYSREG(SYSREG_COPROC, CPACR);
    cpacr |= ((3 << 20) |                      // OR in User and Privileged access for CP10
              (3 << 22));                      // OR in User and Privileged access for CP11
    cpacr &= ~(3 << 30);                       // Clear ASEDIS/D32DIS if set
    __SET_SYSREG(SYSREG_COPROC, CPACR, cpacr); // Store new access permissions into CPACR
    __ISB();                                   // Synchronise any branch prediction so that effects are now visible
    __SET_PROC_VMSR(1 << 30);                  // Enable VFP and SIMD extensions
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

__swi void __svc_isr(void) {
    // Store the four registers r0-r3 to the stack. These contain any parameters
    // to be passed to the SVC handler. Also store r12 as we need something we
    // can clobber, as well as the link register which will be popped back to
    // the page counter on return.
    __asm volatile (
        "STMFD   SP!, {r0-r3, r12, LR}         ;"
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
        // If this was not a semi-hosting ID, call user handler (r0 is first parameter [ID], r1 is second parameter [SP])
        "CMP      r0, r3                       ;"
        "BLNE     __svc_handler                ;"
        // If this is a semi-hosting ID, and we are in this handler, the debugger is not connected, so clear connected flag
        "MOVEQ    r1, %[NoDbggr]               ;"
        "LDREQ    r0, =__semihostingEnabled    ;"
        "STREQ    r1, [r0]                     ;"
        // Restore the processor state from before the SVC was triggered. We have this value saved on the stack.
        "POP     {r0, r3}                      ;"
        "MSR     SPSR_cxsf, r0                 ;"
        // For semi-hosting call, return success status (even though we have done nothing)
        "EOREQ    r0, r0                       ;"
        // Remove pushed registers from stack, and return, restoring SPSR to CPSR
        "LDMFD   SP!, {r0-r3, r12, PC}^        ;"
        ::
        [TMask]   "i" (1 << __PROC_CPSR_BIT_T),
        [SvcIdT]  "i" (SVC_ID_SEMIHOST_THUMB),                // Thumb value is 8bit, so can load directly with MOV.
        [SvcIdAL] "i" ((SVC_ID_SEMIHOST_ARM      ) & 0xFFFF), // Arm value is 32bit, so must split into two 16-bit
        [SvcIdAH] "i" ((SVC_ID_SEMIHOST_ARM >> 16) & 0xFFFF), // half words for MOVW/MOVT instructions to work.
        [NoDbggr] "i" (SVC_NO_DEBUGGER)
    );
}

void __svc_handler (unsigned int id, unsigned int* val) {

}

#endif

