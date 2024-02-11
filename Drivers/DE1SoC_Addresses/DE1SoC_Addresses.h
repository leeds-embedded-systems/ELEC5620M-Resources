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
 * -----------+----------------------------------
 * 10/02/2024 | Create Header
 */

#ifndef DE1SOC_ADDRESSES_H_
#define DE1SOC_ADDRESSES_H_

// List of Common Addresses
//      Peripheral             Base Address           Description                                           Driver
#define LSC_BASE_DDR_RAM       (void*)0x01000040   // 1GB DDR3 Memory 
#define LSC_BASE_FPGA_OCRAM0   (void*)0xC0000000   // 64kB FPGA On-chip SRAM for ARM Instructions
#define LSC_BASE_FPGA_SDRAM    (void*)0xC4000000   // 64MB DDR SDRAM (Untested)
#define LSC_BASE_FPGA_OCRAM1   (void*)0xC8000000   // 16kB FPGA On-chip SRAM for ARM Stack/Heap
#define LSC_BASE_VGA_CHAR_BUFF (void*)0xC9000000   // VGA Character Buffer
#define LSC_BASE_RED_LEDS      (void*)0xFF200000   // 10x Red LEDs                                          [FPGA_PIO]
#define LSC_BASE_7SEG_0to3     (void*)0xFF200020   // 7-segment HEX[3], HEX[2], HEX[1] & HEX[0] Displays    [FPGA_PIO]
#define LSC_BASE_7SEG_4to5     (void*)0xFF200030   // 7-segment HEX[5] & HEX[4] Displays                    [FPGA_PIO]
#define LSC_BASE_SLIDE_SWITCH  (void*)0xFF200040   // 10x Slide Switches                                    [FPGA_PIO]
#define LSC_BASE_KEYS          (void*)0xFF200050   // 4x Push Buttons                                       [FPGA_PIO]
#define LSC_BASE_GPIO_JP1      (void*)0xFF200060   // GPIO0 (JP1) Connector for LT24 LCD and Touchscreen    [FPGA_PIO] | [DE1SoC_LT24 + DE1SoC_Servo]
#define LSC_BASE_GPIO_JP2      (void*)0xFF200070   // GPIO1 (JP2) General Purpose I/O Expansion             [FPGA_PIO]
#define LSC_BASE_MANDELBROT    (void*)0xFF200090   // Mandelbrot Pattern Animation Generator                [DE1SoC_Mandelbrot]
#define LSC_BASE_PS2_PRIMARY   (void*)0xFF200100   // PS/2 (Primary Port)
#define LSC_BASE_PS2_SECONDARY (void*)0xFF200108   // PS/2 (Secondary Port)
#define LSC_BASE_JTAG_UART     (void*)0xFF201000   // 2x JTAG UART
#define LSC_BASE_INFRARED      (void*)0xFF201020   // Infrared (IrDA)
#define LSC_BASE_INTERVAL_TMR1 (void*)0xFF202000   // Interval Timer
#define LSC_BASE_INTERVAL_TMR2 (void*)0xFF202020   // Second Interval Timer
#define LSC_BASE_AV_CONFIG     (void*)0xFF203000   // Audio/video Configuration
#define LSC_BASE_PIXEL_BUFF    (void*)0xFF203020   // Pixel Buffer Control
#define LSC_BASE_CHAR_BUFF     (void*)0xFF203030   // Character Buffer Control
#define LSC_BASE_AUDIOCODEC    (void*)0xFF203040   // Audio System                                          [DE1SoC_WM8731 + HPS_I2C + HPS_GPIO]
#define LSC_BASE_ADC           (void*)0xFF204000   // 8-Channel 12-bit Analog-to-Digital Converter
#define LSC_BASE_ARM_GPIO      (void*)0xFF709000   // ARM A9 GPIO 1                                         [HPS_GPIO]
#define LSC_BASE_I2C_GENERAL   (void*)0xFFC04000   // HPS I2C Master (Accelerometer/VGA/Audio/ADC)          [HPS_I2C]
#define LSC_BASE_I2C_LT14HDR   (void*)0xFFC05000   // HPS I2C Master (LT 14-pin Header)                     [HPS_I2C]
#define LSC_BASE_WATCHDOG      (void*)0xFFD0200C   // ARM A9 Watchdog Timer (CPU 0)                         [HPS_Watchdog]
#define LSC_BASE_PRIV_TIM      (void*)0xFFFEC600   // ARM A9 Private Timer
#define LSC_BASE_PROC_OCRAM    (void*)0xFFFF0000   // ARM A9 64kB On-chip Memory used by Preloader

// Bitmap of used pins in LSC_BASE_ARM_GPIO
#define ARM_GPIO_HPS_KEY          (1 << 25) //GPIO54. HPS Key (Bottom Right Corner)
#define ARM_GPIO_HPS_LED          (1 << 24) //GPIO53. HPS LED (Bottom Right Corner)
#define ARM_GPIO_I2C_GENERAL_MUX  (1 << 19) //GPIO48. Must set to output-high to use I2C_GENERAL
#define ARM_GPIO_I2C_LT14HDR_MUX  (1 << 11) //GPIO40. Must set to output-high to use I2C_LT14HDR

#endif /* DE1SOC_ADDRESSES_H_ */
