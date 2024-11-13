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

// List of Common Addresses
//      Peripheral             Base Address                     Description                                           Driver
#define LSC_BASE_BOOTLDR_RAM   ((unsigned char*)0x01000040)   // ~16MB Reserved DDR RAM for bootloader
#define LSC_BASE_DDR_RAM       ((unsigned char*)0x02000040)   // ~1GB DDR3 Memory
#define LSC_BASE_FPGA_SDRAM    ((unsigned char*)0xC0000000)   // 64MB DDR SDRAM (Untested)
#define LSC_BASE_FPGA_OCRAM    ((unsigned char*)0xC8000000)   // 256kB FPGA On-chip SRAM
#define LSC_BASE_VGA_CHAR_BUFF ((unsigned char*)0xC9000000)   // VGA Character Buffer
#define LSC_BASE_BOOTLDR_CACHE ((unsigned char*)0xCA000000)   // 64kB Reserved On-chip SRAM for bootloader
#define LSC_BASE_RED_LEDS      ((unsigned char*)0xFF200000)   // 10x Red LEDs                                          [FPGA_PIO]
#define LSC_BASE_7SEG_0to3     ((unsigned char*)0xFF200020)   // 7-segment HEX[3], HEX[2], HEX[1] & HEX[0] Displays    [FPGA_PIO]
#define LSC_BASE_7SEG_4to5     ((unsigned char*)0xFF200030)   // 7-segment HEX[5] & HEX[4] Displays                    [FPGA_PIO]
#define LSC_BASE_SLIDE_SWITCH  ((unsigned char*)0xFF200040)   // 10x Slide Switches                                    [FPGA_PIO]
#define LSC_BASE_KEYS          ((unsigned char*)0xFF200050)   // 4x Push Buttons                                       [FPGA_PIO]
#define LSC_BASE_GPIO_JP1      ((unsigned char*)0xFF200060)   // GPIO0 (JP1) Connector for LT24 LCD and Touchscreen    [FPGA_PIO] | [DE1SoC_LT24]
#define LSC_BASE_GPIO_JP2      ((unsigned char*)0xFF200070)   // GPIO1 (JP2) General Purpose I/O Expansion             [FPGA_PIO]
#define LSC_BASE_LT24HWDATA    ((unsigned char*)0xFF200080)   // LT24 Hardware Optimised Data Transfer                 [DE1SoC_LT24]
#define LSC_BASE_MANDELBROT    ((unsigned char*)0xFF200090)   // Mandelbrot Pattern Animation Generator                [DE1SoC_Mandelbrot]
#define LSC_BASE_SERVO         ((unsigned char*)0xFF2000C0)   // 4-channel Servo PWM Controller                        [DE1SoC_Servo]
#define LSC_BASE_PS2_PRIMARY   ((unsigned char*)0xFF200100)   // PS/2 (Primary Port)
#define LSC_BASE_PS2_SECONDARY ((unsigned char*)0xFF200108)   // PS/2 (Secondary Port)
#define LSC_BASE_JTAG_UART     ((unsigned char*)0xFF201000)   // 2x JTAG UART
#define LSC_BASE_INFRARED      ((unsigned char*)0xFF201020)   // Infrared (IrDA)
#define LSC_BASE_INTERVAL_TMR1 ((unsigned char*)0xFF202000)   // Interval Timer
#define LSC_BASE_INTERVAL_TMR2 ((unsigned char*)0xFF202020)   // Second Interval Timer
#define LSC_BASE_AV_CONFIG     ((unsigned char*)0xFF203000)   // Audio/video Configuration
#define LSC_BASE_PIXEL_BUFF    ((unsigned char*)0xFF203020)   // Pixel Buffer Control
#define LSC_BASE_CHAR_BUFF     ((unsigned char*)0xFF203030)   // Character Buffer Control
#define LSC_BASE_AUDIOCODEC    ((unsigned char*)0xFF203040)   // Audio System                                          [DE1SoC_WM8731 + HPS_I2C + HPS_GPIO]
#define LSC_BASE_ADC           ((unsigned char*)0xFF204000)   // 8-Channel 12-bit Analog-to-Digital Converter
#define LSC_BASE_ARM_GPIO      ((unsigned char*)0xFF709000)   // ARM A9 GPIO 1                                         [HPS_GPIO]
#define LSC_BASE_I2C_GENERAL   ((unsigned char*)0xFFC04000)   // HPS I2C Master (Accelerometer/VGA/Audio/ADC)          [HPS_I2C]
#define LSC_BASE_I2C_LT14HDR   ((unsigned char*)0xFFC05000)   // HPS I2C Master (LT 14-pin Header)                     [HPS_I2C]
#define LSC_BASE_HPS_TIMERSP0  ((unsigned char*)0xFFC08000)   // HPS SP Timer 0 (runs at 100MHz)
#define LSC_BASE_HPS_TIMERSP1  ((unsigned char*)0xFFC09000)   // HPS SP Timer 1 (runs at 100MHz)
#define LSC_BASE_WATCHDOG      ((unsigned char*)0xFFD02000)   // ARM A9 Watchdog Timer (CPU 0)                         [HPS_Watchdog]
#define LSC_BASE_PRIV_TIM      ((unsigned char*)0xFFFEC600)   // ARM A9 Private Timer
#define LSC_BASE_PROC_OCRAM    ((unsigned char*)0xFFFF0000)   // ARM A9 64kB On-chip Memory used by Preloader


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

#endif /* DE1SOC_ADDRESSES_H_ */
