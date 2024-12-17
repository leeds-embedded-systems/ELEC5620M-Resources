/*
 * DE1-SoC Address Map
 * -------------------
 *
 * Provides addresses for various peripherals within
 * the Leeds SoC computer. These can be used when
 * initialising drivers or for performing direct access.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+--------------------------------------
 * 12/11/2024 | Update for 2024/25 Leeds SoC Computer
 * 10/02/2024 | Create Header
 */

#ifndef DE1SOC_ADDRESSES_H_
#define DE1SOC_ADDRESSES_H_

#include <stdint.h>

// HPS Base Addresses
#define HPS_AXIMASTER_BASE ((uint32_t *)0xC0000000U)
#define HPS_LWMASTER_BASE  ((uint32_t *)0xFF200000U)

// List of Common Peripherals
//      Peripheral             Base Address                   Description                                           Driver
#define LSC_BASE_BOOTLDR_RAM   ((uint8_t  *)0x01000040U)   // ~16MB Reserved DDR RAM for bootloader
#define LSC_BASE_DDR_RAM       ((uint8_t  *)0x02000040U)   // ~1GB DDR3 Memory
#define LSC_BASE_FPGA_SDRAM    ((uint8_t  *)0xC0000000U)   // 64MB DDR SDRAM (Untested)
#define LSC_BASE_FPGA_OCRAM    ((uint8_t  *)0xC8000000U)   // 256kB FPGA On-chip SRAM
#define LSC_BASE_VGA_CHAR_BUFF ((uint8_t  *)0xC9000000U)   // VGA Character Buffer
#define LSC_BASE_BOOTLDR_CACHE ((uint8_t  *)0xCA000000U)   // 64kB Reserved On-chip SRAM for bootloader
#define LSC_BASE_RED_LEDS      ((uint8_t  *)0xFF200000U)   // 10x Red LEDs                                          [FPGA_PIO]
#define LSC_BASE_7SEG_0to3     ((uint8_t  *)0xFF200020U)   // 7-segment HEX[3], HEX[2], HEX[1] & HEX[0] Displays    [FPGA_PIO]
#define LSC_BASE_7SEG_4to5     ((uint8_t  *)0xFF200030U)   // 7-segment HEX[5] & HEX[4] Displays                    [FPGA_PIO]
#define LSC_BASE_SLIDE_SWITCH  ((uint8_t  *)0xFF200040U)   // 10x Slide Switches                                    [FPGA_PIO]
#define LSC_BASE_KEYS          ((uint8_t  *)0xFF200050U)   // 4x Push Buttons                                       [FPGA_PIO]
#define LSC_BASE_GPIO_JP1      ((uint8_t  *)0xFF200060U)   // GPIO0 (JP1) Connector for LT24 LCD and Touchscreen    [FPGA_PIO] | [DE1SoC_LT24]
#define LSC_BASE_GPIO_JP2      ((uint8_t  *)0xFF200070U)   // GPIO1 (JP2) General Purpose I/O Expansion             [FPGA_PIO]
#define LSC_BASE_LT24HWDATA    ((uint32_t *)0xFF200080U)   // LT24 Hardware Optimised Data Transfer                 [DE1SoC_LT24]
#define LSC_BASE_MANDELBROT    ((uint8_t  *)0xFF200090U)   // Mandelbrot Pattern Animation Generator                [DE1SoC_Mandelbrot]
#define LSC_BASE_SERVO         ((uint8_t  *)0xFF2000C0U)   // 4-channel Servo PWM Controller                        [DE1SoC_Servo]
#define LSC_BASE_PS2_PRIMARY   ((uint32_t *)0xFF200100U)   // PS/2 (Primary Port)
#define LSC_BASE_PS2_SECONDARY ((uint32_t *)0xFF200108U)   // PS/2 (Secondary Port)
#define LSC_BASE_JTAG_UART     ((uint32_t *)0xFF201000U)   // 2x JTAG UART
#define LSC_BASE_INFRARED      ((uint8_t  *)0xFF201020U)   // Infrared (IrDA)                                       [FPGA_IrDAController]
#define LSC_BASE_INTERVAL_TMR  ((uint32_t *)0xFF202000U)   // Interval Timer
#define LSC_BASE_SYSTEM_ID     ((uint32_t *)0xFF202020U)   // System ID                                             [Util/sysid]
#define LSC_BASE_AV_CONFIG     ((uint32_t *)0xFF203000U)   // Audio/video Configuration
#define LSC_BASE_PIXEL_BUFF    ((uint32_t *)0xFF203020U)   // Pixel Buffer Control
#define LSC_BASE_CHAR_BUFF     ((uint32_t *)0xFF203030U)   // Character Buffer Control
#define LSC_BASE_AUDIOCODEC    ((uint8_t  *)0xFF203040U)   // Audio System                                          [DE1SoC_WM8731 + HPS_I2C + HPS_GPIO]
#define LSC_BASE_ADC           ((uint32_t *)0xFF204000U)   // 8-Channel 12-bit Analog-to-Digital Converter
#define LSC_BASE_ARM_GPIO      ((uint32_t *)0xFF709000U)   // ARM A9 GPIO 1                                         [HPS_GPIO]
#define LSC_BASE_I2C_GENERAL   ((uint32_t *)0xFFC04000U)   // HPS I2C Master (Accelerometer/VGA/Audio/ADC)          [HPS_I2C]
#define LSC_BASE_I2C_LT14HDR   ((uint32_t *)0xFFC05000U)   // HPS I2C Master (LT 14-pin Header)                     [HPS_I2C]
#define LSC_BASE_HPS_TIMERSP0  ((uint32_t *)0xFFC08000U)   // HPS SP Timer 0 (runs at 100MHz)
#define LSC_BASE_HPS_TIMERSP1  ((uint32_t *)0xFFC09000U)   // HPS SP Timer 1 (runs at 100MHz)
#define LSC_BASE_WATCHDOG      ((uint32_t *)0xFFD02000U)   // ARM A9 Watchdog Timer (CPU 0)                         [HPS_Watchdog]
#define LSC_BASE_PRIV_TIM      ((uint32_t *)0xFFFEC600U)   // ARM A9 Private Timer
#define LSC_BASE_PROC_OCRAM    ((uint8_t  *)0xFFFF0000U)   // ARM A9 64kB On-chip Memory

// List of memory sizes
#define LSC_SIZE_DDR_RAM       0xBDFFFFC0U
#define LSC_SIZE_FPGA_SDRAM    0x04000000U
#define LSC_SIZE_FPGA_OCRAM    0x00040000U
#define LSC_SIZE_BOOTLDR_CACHE 0x00010000U
#define LSC_SIZE_PROC_OCRAM    0x00010000U

// Bitmap of used pins in LSC_BASE_ARM_GPIO
#define ARM_GPIO_HPS_KEY          (1 << 25) //GPIO54. HPS Key (Bottom Right Corner)
#define ARM_GPIO_HPS_LED          (1 << 24) //GPIO53. HPS LED (Bottom Right Corner)
#define ARM_GPIO_I2C_GENERAL_MUX  (1 << 19) //GPIO48. Must set to output-high to use I2C_GENERAL
#define ARM_GPIO_I2C_LT14HDR_MUX  (1 << 11) //GPIO40. Must set to output-high to use I2C_LT14HDR

// Pin directions for ARM GPIO (pins which are outputs)
#define ARM_GPIO_DIR              (ARM_GPIO_HPS_LED | ARM_GPIO_I2C_GENERAL_MUX | ARM_GPIO_I2C_LT14HDR_MUX)
#define ARM_GPIO_POLARITY         (ARM_GPIO_HPS_KEY)  // Key is active low, so invert for consistency

// Masks for Keys/Switches
#define LSC_KEYS_MASK             0xFU
#define LSC_SLIDE_SWITCH_MASK     0x3FFU
#define LSC_RED_LEDS_MASK         0x3FFU

// Init values for FPGA_PIO driver
//   e.g. FPGA_PIO_initialise(LSC_BASE_7SEG_0to3, LSC_CONFIG_7SEG, &drivers.hex0to3);
#define LSC_CONFIG_KEYS           FPGA_PIO_DIRECTION_IN, false, false, true, true, 0, 0
#define LSC_CONFIG_SLIDE_SWITCH   FPGA_PIO_DIRECTION_IN, false, false, false, false, 0, 0
#define LSC_CONFIG_GPIO           FPGA_PIO_DIRECTION_BIDIR, false, false, true, true, 0, 0
#define LSC_CONFIG_7SEG           FPGA_PIO_DIRECTION_OUT, false, false, false, false, 0, 0
#define LSC_CONFIG_RED_LEDS       FPGA_PIO_DIRECTION_OUT, false, false, false, false, 0, 0

// System IDs
#define LSC_SYSID_SOCPC           0x50C1EED5
#define LSC_SYSID_HPSWRAPPER      0x50CDECAF

#endif /* DE1SOC_ADDRESSES_H_ */
