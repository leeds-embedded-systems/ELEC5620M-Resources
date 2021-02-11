/*
 * Cyclone V HPS Vector Floating Point Enable Stub
 * -----------------------------------------------
 * Description:
 * Before the vector floating point unit can be used, it must be enabled by the CPU
 * using a specific set of ARM CPU registers. You are not expected to understand
 * what this code is doing, however when using any of the hardware floating point
 * compiler modes, this function must be included in your project
 *
 * The name of this function should *never* be changed. It is a special name used
 * reserved by armcc for a specific purpose. It will always be run before any
 * initialisation of the stack and heap, and before any functions in the C standard
 * library are called. This is the perfect time to be initialising the VFP unit
 *
 * Company: University of Leeds
 * Author(s): T Carpenter
 *            D Cowell
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 05/12/2018 | Creation of driver
 * 11/02/2021 | Add __TARGET_FPU_VFP guard to prevent compilation if no VFP enabled.
 *
 */

#if __TARGET_FPU_VFP

__asm void _platform_pre_stackheap_init(void) {
    //Inline assembly code for enabling FPU
    MRC  p15,0,r0,c1,c0,2  ; // Read CPACR into r0
    ORR  r0,r0,#(3<<20)    ; // OR in User and Privileged access for CP10
    ORR  r0,r0,#(3<<22)    ; // OR in User and Privileged access for CP11
    BIC  r0,r0,#(3<<30)    ; // Clear ASEDIS/D32DIS if set
    MCR  p15,0,r0,c1,c0,2  ; // Store new access permissions into CPACR
    ISB                    ; // Ensure side-effect of CPACR is visible
    MOV  r0,#(1<<30)       ; // Create value with FPEXC (bit 30) set in r0
    VMSR FPEXC,r0          ; // Enable VFP and SIMD extensions
    BX   LR                ; // Return from function
}

#endif
