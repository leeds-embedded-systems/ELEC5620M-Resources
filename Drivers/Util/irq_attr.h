/*
 * ISR Attributes
 * --------------
 *
 * Attributes applied to interrupt handlers based on
 * the type of interrupt being handled.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 31/01/2024 | Creation of Header
 */

#ifndef IRQ_ATTR_H_
#define IRQ_ATTR_H_

// Generic Interrupt Handler
#define __isr   __attribute__((interrupt))
// Specific Source Handler
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
#define __irq   __attribute__((interrupt("IRQ")))
#endif
#define __fiq   __attribute__((interrupt("FIQ")))
#define __swi   __attribute__((interrupt("SWI")))
#define __abort __attribute__((interrupt("ABORT")))
#define __undef __attribute__((interrupt("UNDEF")))

#endif /* IRQ_ATTR_H_ */
