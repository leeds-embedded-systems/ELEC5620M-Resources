/*
 * DE1-SoC Customised Variant of the HPS_IRQ Driver
 * ------------------------------------------------
 *
 * Provides named interrupts IDs for the Leeds SoC
 * computer. Please refer to HPS_IRQ.h for the full
 * interrupt driver.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 30/01/2024 | Create Header
 */

#ifndef DE1SOC_IRQ_H_
#define DE1SOC_IRQ_H_

#include "HPS_IRQ/HPS_IRQ.h"

enum {
    IRQ_INTERVAL_TIMER              = IRQ_FPGA0,
    IRQ_LSC_KEYS                    = IRQ_FPGA1,
    IRQ_LSC_AUDIO                   = IRQ_FPGA6,
    IRQ_LSC_PS2_PRIMARY             = IRQ_FPGA7,
    IRQ_LSC_JTAG                    = IRQ_FPGA8,
    IRQ_LSC_IrDA                    = IRQ_FPGA9,
    IRQ_LSC_GPIO_JP1                = IRQ_FPGA11,
    IRQ_LSC_GPIO_JP2                = IRQ_FPGA12,
    IRQ_LSC_PS2_SECONDARY           = IRQ_FPGA17,
};

#endif /* DE1SOC_IRQ_H_ */
