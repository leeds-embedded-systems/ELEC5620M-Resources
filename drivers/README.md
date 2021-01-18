# DE1-SoC Drivers

C source files which enable support for various hardware peripherals on the development board.

### DE1SoC_LT24, BasicFont

Support for the LT24 LCD module. BasicFont allows printing of pixel generated characters to the screen.

### DE1SoC_Mandelbrot

Driver for the Leeds SoC Computer Hardware Mandelbrot Controller. Allows generating and display of a visualisation of the Mandelbrot set for display testing.

### DE1SoC_Servo

Driver for the Servo Controller in the DE1-SoC, allowing PWM control over servo motors.

### DE1SoC_WM8731

Driver for the WM8731 Audio Controller, which is a hardware audio interface allowing input and output of stereo audio signals.

### HPS_I2C

Driver for the HPS embedded I2C controller, for communicating with other devices.

### HPS_IRQ

Driver for enabling and using the General Interrupt Controller (GIC). The driver includes code to create a vector table, and register interrupts.

### HPS_Watchdog

Simple inline functions for resetting watchdog and returning the current watchdog timer value.

### HPS_usleep

The POSIX usleep() function does not exist for bare metal applications in DS-5. 
This library adds a similar function which allows the processor to be stalled for x microseconds.
Delays up to ~2.09 seconds are supported. This is to ensure the watchdog won't time out before the delay is finished.
