/*
 * Nios PIO Controller Registers
 * -----------------------------
 *
 * Register map for writing to generic PIO controller
 * core (e.g. avmm_pio_hw).
 *
 * The map has been intentionally separated to allow
 * use with other drivers which are based on a PIO
 * controller under the hood.
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 21/02/2024 | Creation of register map file.
 *
 */

/*
 * Register Map
 */

#define GPIO_OUTPUT     (0x00 / sizeof(unsigned int))   // Write output if in _OUT or _INOUT or  _BIDIR mode
#define GPIO_INPUT      (0x00 / sizeof(unsigned int))   // Read input if in _IN or _INOUT or _BIDIR modes. Read output if in _OUT mode, or if uses split interfaces.
#define GPIO_DIRECTION  (0x04 / sizeof(unsigned int))   // Set direction if in _BIDIR mode
#define GPIO_SPLITINPUT (0x04 / sizeof(unsigned int))   // Read input if has split interfaces
#define GPIO_INTR_MASK  (0x08 / sizeof(unsigned int))   // IRQ mask if hardware supports
#define GPIO_INTR_FLAGS (0x0C / sizeof(unsigned int))   // IRQ flags if hardware supports
#define GPIO_OUT_SET    (0x10 / sizeof(unsigned int))   // Set individual output bits if hardware supports
#define GPIO_OUT_CLEAR  (0x14 / sizeof(unsigned int))   // Clear individual output bits if hardware supports
