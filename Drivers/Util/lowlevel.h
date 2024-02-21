/*
 * Low-Level Macros and Register Accesses for ARMv7
 * ------------------------------------------------
 *
 * Provides low-level accesses to registers which
 * would otherwise require manually adding inline
 * assembly.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 31/01/2024 | Include ISR attributes header
 * 14/01/2024 | Creation of header
 *
 */

#ifndef LOWLEVEL_H_
#define LOWLEVEL_H_

/*
 * Arm Compatibility
 *
 * Provides low level macros such as __disable_irq()
 */
#include <arm_compat.h>

#define __disable_bothirqs() __asm( "CPSID if")

#include "Util/irq_attr.h"

/*
 * Change processor state
 */

typedef enum {
    PROC_STATE_USR = 0x10, //User       Mode
    PROC_STATE_FIQ = 0x11, //FIQ        Mode
    PROC_STATE_IRQ = 0x12, //IRQ        Mode
    PROC_STATE_SVC = 0x13, //Supervisor Mode
    PROC_STATE_MON = 0x16, //Monitor    Mode
    PROC_STATE_ABT = 0x17, //Abort      Mode
    PROC_STATE_HYP = 0x1A, //Hypervisor Mode
    PROC_STATE_UND = 0x1B, //Undefined  Mode
    PROC_STATE_SYS = 0x1F  //System     Mode
} ProcState;

#define __PROC_CPSR_BIT_M   0
#define __PROC_CPSR_MASK_M  0x1F
#define __PROC_CPSR_BIT_T   5
#define __PROC_CPSR_BIT_F   6
#define __PROC_CPSR_BIT_I   7
#define __PROC_CPSR_BIT_A   8
#define __PROC_CPSR_BIT_E   9
#define __PROC_CPSR_BIT_GE  16
#define __PROC_CPSR_MASK_GE 0xF
#define __PROC_CPSR_BIT_Q   27
#define __PROC_CPSR_BIT_V   28
#define __PROC_CPSR_BIT_C   29
#define __PROC_CPSR_BIT_Z   30
#define __PROC_CPSR_BIT_N   31


//Helper APIs
#define __set_proc_state(val) \
  __asm__ __volatile__("CPS %[state]\n" :: [state] "i" (val))

#define __current_cpsr()                                                       \
  __extension__({                                                              \
    register unsigned int current_cpsr;                                        \
    __asm__ __volatile__("mrs %0, cpsr" : "=r"(current_cpsr) : : );            \
    current_cpsr;                                                              \
  })

#define __current_spsr()                                                       \
  __extension__({                                                              \
    register unsigned int current_spsr;                                        \
    __asm__ __volatile__("mrs %0, spsr" : "=r"(current_spsr) : : );            \
    current_spsr;                                                              \
  })

static __inline__ void __attribute__((__always_inline__, __nodebug__))
__set_spsr(unsigned int spsr) {
    __asm__ __volatile__("msr spsr_cxsf, %[spsr]\n" ::[spsr] "r" (spsr):);
}

static __inline__ void __attribute__((__always_inline__, __nodebug__))
__set_vmsr(unsigned int vmsr) {
    __asm__ __volatile__("vmsr fpexc, %[vmsr]\n" ::[vmsr] "r" (vmsr):);
}

static __inline__ ProcState __attribute__((__always_inline__, __nodebug__))
__current_proc_state(void) {
  return (ProcState)((__current_cpsr() >> __PROC_CPSR_BIT_M) & __PROC_CPSR_MASK_M);
}

static __inline__ const char*  __attribute__((__always_inline__, __nodebug__))
__name_proc_state(ProcState state) {
    switch (state) {
        case PROC_STATE_USR: return "User";
        case PROC_STATE_FIQ: return "FIQ";
        case PROC_STATE_IRQ: return "IRQ";
        case PROC_STATE_SVC: return "Suprvsr";
        case PROC_STATE_MON: return "Monitor";
        case PROC_STATE_ABT: return "Abort";
        case PROC_STATE_HYP: return "Hyprvsr";
        case PROC_STATE_UND: return "UndefInstr";
        case PROC_STATE_SYS: return "System";
        default: return "???";
    }
}

// Access macros
#define __SET_PROC_STATE(state)     __set_proc_state(state)  // Must be constant value
#define __GET_PROC_STATE()          __current_proc_state()
#define __GET_PROC_STATE_NAME(sate) __name_proc_state(state)
#define __GET_PROC_CPSR()           __current_cpsr()
#define __GET_PROC_SPSR()           __current_spsr()
#define __SET_PROC_SPSR(val)        __set_spsr(val)
#define __SET_PROC_VMSR(val)        __set_vmsr(val)

/*
 * Stack pointer/program counter
 */

//Helper APIs
#define __current_lr()                                                         \
  __extension__({                                                              \
    register unsigned int current_lr;                                          \
    __asm__ __volatile__("mov %0, lr" : "=r"(current_lr) : : );                \
    current_lr;                                                                \
  })

#define __MOV_REG_GEN(src) \
static __inline__ void __attribute__((__always_inline__, __nodebug__)) \
__set_##src##_reg(unsigned int val) { \
    __asm__ __volatile__("MOV " #src ", %[val]\n" ::[val] "r" (val):); \
}
__MOV_REG_GEN(sp)
__MOV_REG_GEN(lr)
__MOV_REG_GEN(pc)

// Access macros
#define __GET_PC()    __current_pc()
#define __SET_PC(val) __set_pc_reg(val)

#define __GET_LR()    __current_lr()
#define __SET_LR(val) __set_lr_reg(val)

#define __GET_SP()    __current_sp()
#define __SET_SP(val) __set_sp_reg(val)

// Simple branch
#define __BRANCH(to) __asm__ __volatile__ ("B " #to " \n");

// Stack Init Functions
#define __INIT_SP_SYS(top) __asm__ __volatile__("MOV SP, %[sp]\n" ::[sp] "r" (top):)
#define __INIT_SP_MODE(mode, top)  \
    __asm volatile ( \
		"CPS %[state]    \n\t" \
		"MOV SP, %[sp]   \n\t" \
		"CPS %[sysstate] \n\t" \
		:: [state] "i" (mode), [sp] "r" (top), [sysstate] "i" (PROC_STATE_SYS) \
	);

/*
 * System Register Access
 */

// System Registers Co-processor
#define SYSREG_COPROC 15

// Registers below must follow the exact naming convention:
//
//    SYSREG_<reg name>_[CP|CP_OP|CPA|CPA_OP]
//
// Where <reg name> can be accessed with:
//
//    __SET_SYSREG(SYSREG_COPROC, <reg name>, val)
//    __GET_SYSREG(SYSREG_COPROC, <reg name>)
//

// SCTLR Register
#define SYSREG_SCTLR_CP        1
#define SYSREG_SCTLR_CP_OP     0
#define SYSREG_SCTLR_CPA       0
#define SYSREG_SCTLR_CPA_OP    0

#define SYSREG_SCTLR_BIT_M     0
#define SYSREG_SCTLR_BIT_A     1
#define SYSREG_SCTLR_BIT_C     2
#define SYSREG_SCTLR_BIT_SW    10
#define SYSREG_SCTLR_BIT_Z     11
#define SYSREG_SCTLR_BIT_I     12

// VBAR Register
#define SYSREG_VBAR_CP         12
#define SYSREG_VBAR_CP_OP      0
#define SYSREG_VBAR_CPA        0
#define SYSREG_VBAR_CPA_OP     0

// CPACR Register
#define SYSREG_CPACR_CP         1
#define SYSREG_CPACR_CP_OP      0
#define SYSREG_CPACR_CPA        0
#define SYSREG_CPACR_CPA_OP     2

// Access macros
//   Converts to MCR/MRC instructions
#define __SET_SYSREG(coProc, regName, val) __arm_mcr(coProc, SYSREG_##regName##_CP_OP, (val), SYSREG_##regName##_CP, SYSREG_##regName##_CPA, SYSREG_##regName##_CPA_OP)
#define __GET_SYSREG(coProc, regName) __arm_mrc(coProc, SYSREG_##regName##_CP_OP, SYSREG_##regName##_CP, SYSREG_##regName##_CPA, SYSREG_##regName##_CPA_OP)


#endif /* LOWLEVEL_H_ */
