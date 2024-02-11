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

#include <stdio.h>

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
void __default_isr (void) { // No __isr attribute used as we never return.
    printf("!EXCEPTION! In %s Mode.\n", __name_proc_state(__current_proc_state()));
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
// Other Interrupt Handlers. Can be overridden
__undef void __undef_isr (void) __attribute__ ((weak, alias("__default_isr")));
__swi   void __svc_isr   (void) __attribute__ ((weak, alias("__default_isr")));
__abort void __pftcAb_isr(void) __attribute__ ((weak, alias("__default_isr")));
__abort void __dataAb_isr(void) __attribute__ ((weak, alias("__default_isr")));
__irq   void __irq_isr   (void) __attribute__ ((weak, alias("__default_isr")));
__fiq   void __fiq_isr   (void) __attribute__ ((weak, alias("__default_isr")));

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
    // Initialise a temporary stack
    __INIT_SP_SYS(IRQ_STACK_TOP);
    // And continue
    __asm__ __volatile__ ("B __init_stacks \n");
}

void __init_stacks (void) {
    // Initialise all IRQ stack pointers
    unsigned int stackTop = IRQ_STACK_TOP;
    // FIQ
    __INIT_SP_MODE(PROC_STATE_FIQ, stackTop);
    // IRQ
    __INIT_SP_MODE(PROC_STATE_IRQ, stackTop - IRQ_STACK_SIZE);
    // SVC
    __INIT_SP_MODE(PROC_STATE_SVC, stackTop - 2*IRQ_STACK_SIZE);
    // Abrt
    __INIT_SP_MODE(PROC_STATE_ABT, stackTop - 3*IRQ_STACK_SIZE);
    // Undef
    __INIT_SP_MODE(PROC_STATE_UND, stackTop - 4*IRQ_STACK_SIZE);
    // Set the location of the vector table using VBAR register
    __SET_SYSREG(SYSREG_COPROC, VBAR, (unsigned int)&__vector_table);
    // Enable non-aligned access by clearing the A bit (bit 1) of the SCTLR register
    unsigned int sctlr = __GET_SYSREG(SYSREG_COPROC, SCTLR);
    sctlr &= ~(1 << SYSREG_SCTLR_BIT_A);
    __SET_SYSREG(SYSREG_COPROC, SCTLR, sctlr);
    // If program is compiled targetting a hardware VFP, we must
    // enable the floating point unit to prevent run-time errors
    // in the C standard library.
#if __TARGET_FPU_VFP
    //Inline assembly code for enabling FPU
    unsigned int cpacr = __GET_SYSREG(SYSREG_COPROC, CPACR);
    cpacr = cparc |
    		(3 << 20) |                 // OR in User and Privileged access for CP10
			(3 << 22);                  // OR in User and Privileged access for CP11
    cpacr &= ~(3 << 30);                // Clear ASEDIS/D32DIS if set
	__SET_SYSREG(SYSREG_COPROC, CPACR); // Store new access permissions into CPACR
	__isb();                            // Synchronise any branch prediction so that effects are now visible
	__SET_PROC_VMSR(1 << 30);           // Enable VFP and SIMD extensions
#endif
    // Launch the C entry point
    __main();
}


#endif

