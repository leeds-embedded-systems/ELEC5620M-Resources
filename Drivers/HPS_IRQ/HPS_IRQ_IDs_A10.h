/*
 * HPS IRQ IDs
 * ---------------
 * Description:
 * This file contains an enum type for various peripheral
 * interrupt IDs in the Arria 10 HPS.
 *
 * Do NOT include this file directly. Instead include HPS_IRQ.h
 *
 */

#ifndef HPS_IRQ_IDS_H_
#define HPS_IRQ_IDS_H_

#if defined(__ARRIA10__)

typedef enum {

    /* ARM A9 MPCORE devices (0-15 are software, 16-31 are unused except for these three) */
    IRQ_MPCORE_GLOBAL_TIMER         = 27,
    IRQ_MPCORE_PRIVATE_TIMER        = 29,
    IRQ_MPCORE_WATCHDOG             = 30,

/* ARM A9 CPU Interrupts (there are 19 in total; you shouldn't need any of them)*/
    IRQ_DERR_GLOBAL                 = 32,
    IRQ_CPUX_PARITYFAIL             = 33,
    IRQ_SERR_GLOBAL                 = 34,
    IRQ_CPU0_DEFLAGS0               = 35,
    IRQ_CPU0_DEFLAGS1               = 36,
    IRQ_CPU0_DEFLAGS2               = 37,
    IRQ_CPU0_DEFLAGS3               = 38,
    IRQ_CPU0_DEFLAGS4               = 39,
    IRQ_CPU0_DEFLAGS5               = 40,
    IRQ_CPU0_DEFLAGS6               = 41,
    IRQ_CPU1_DEFLAGS0               = 42,
    IRQ_CPU1_DEFLAGS1               = 43,
    IRQ_CPU1_DEFLAGS2               = 44,
    IRQ_CPU1_DEFLAGS3               = 45,
    IRQ_CPU1_DEFLAGS4               = 46,
    IRQ_CPU1_DEFLAGS5               = 47,
    IRQ_CPU1_DEFLAGS6               = 48,
    IRQ_SCU_EV_ABORT                = 49,
    IRQ_L2_COMBINED                 = 50,

/* FPGA interrupts (there are 64 in total) */
    IRQ_FPGA0                       = 51,
    IRQ_FPGA1                       = 52,
    IRQ_FPGA2                       = 53,
    IRQ_FPGA3                       = 54,
    IRQ_FPGA4                       = 55,
    IRQ_FPGA5                       = 56,
    IRQ_FPGA6                       = 57,
    IRQ_FPGA7                       = 58,
    IRQ_FPGA8                       = 59,
    IRQ_FPGA9                       = 60,
    IRQ_FPGA10                      = 61,
    IRQ_FPGA11                      = 62,
    IRQ_FPGA12                      = 63,
    IRQ_FPGA13                      = 64,
    IRQ_FPGA14                      = 65,
    IRQ_FPGA15                      = 66,
    IRQ_FPGA16                      = 67,
    IRQ_FPGA17                      = 68,
    IRQ_FPGA18                      = 69,
    IRQ_FPGA19                      = 70,
    IRQ_FPGA20                      = 71,
    IRQ_FPGA21                      = 72,
    IRQ_FPGA22                      = 73,
    IRQ_FPGA23                      = 74,
    IRQ_FPGA24                      = 75,
    IRQ_FPGA25                      = 76,
    IRQ_FPGA26                      = 77,
    IRQ_FPGA27                      = 78,
    IRQ_FPGA28                      = 79,
    IRQ_FPGA29                      = 80,
    IRQ_FPGA30                      = 81,
    IRQ_FPGA31                      = 82,
    IRQ_FPGA32                      = 83,
    IRQ_FPGA33                      = 84,
    IRQ_FPGA34                      = 85,
    IRQ_FPGA35                      = 86,
    IRQ_FPGA36                      = 87,
    IRQ_FPGA37                      = 88,
    IRQ_FPGA38                      = 89,
    IRQ_FPGA39                      = 90,
    IRQ_FPGA40                      = 91,
    IRQ_FPGA41                      = 92,
    IRQ_FPGA42                      = 93,
    IRQ_FPGA43                      = 94,
    IRQ_FPGA44                      = 95,
    IRQ_FPGA45                      = 96,
    IRQ_FPGA46                      = 97,
    IRQ_FPGA47                      = 98,
    IRQ_FPGA48                      = 99,
    IRQ_FPGA49                      = 100,
    IRQ_FPGA50                      = 101,
    IRQ_FPGA51                      = 102,
    IRQ_FPGA52                      = 103,
    IRQ_FPGA53                      = 104,
    IRQ_FPGA54                      = 105,
    IRQ_FPGA55                      = 106,
    IRQ_FPGA56                      = 107,
    IRQ_FPGA57                      = 108,
    IRQ_FPGA58                      = 109,
    IRQ_FPGA59                      = 110,
    IRQ_FPGA60                      = 111,
    IRQ_FPGA61                      = 112,
    IRQ_FPGA62                      = 113,
    IRQ_FPGA63                      = 114,

/* HPS device interrupts (45 in total) */
    IRQ_DMA0                        = 115,
    IRQ_DMA1                        = 116,
    IRQ_DMA2                        = 117,
    IRQ_DMA3                        = 118,
    IRQ_DMA4                        = 119,
    IRQ_DMA5                        = 120,
    IRQ_DMA6                        = 121,
    IRQ_DMA7                        = 122,
    IRQ_DMA_ABORT                   = 123,
    IRQ_EMAC0                       = 124,
    IRQ_EMAC1                       = 125,
    IRQ_EMAC2                       = 126,
    IRQ_USB0                        = 127,
    IRQ_USB1                        = 128,
    IRQ_HMC_ERROR                   = 129,
    IRQ_SDMMC                       = 130,
    IRQ_NAND                        = 131,
    IRQ_QSPI                        = 132,
    IRQ_SPIM0                       = 133,
    IRQ_SPIM1                       = 134,
    IRQ_SPIS0                       = 135,
    IRQ_SPIS1                       = 136,
    IRQ_I2C0                        = 137,
    IRQ_I2C1                        = 138,
    IRQ_I2C2                        = 139,
    IRQ_I2C3                        = 140,
    IRQ_I2C4                        = 141,
    IRQ_UART0                       = 142,
    IRQ_UART1                       = 143,
    IRQ_GPIO0                       = 144,
    IRQ_GPIO1                       = 145,
    IRQ_GPIO2                       = 146,
    IRQ_TIMER_L4SP_0                = 147, //HPS Timer 0
    IRQ_TIMER_L4SP_1                = 148, //HPS Timer 1
    IRQ_TIMER_OSC1_0                = 149,
    IRQ_TIMER_OSC1_1                = 150,
    IRQ_WDOG0                       = 151, //HPS Watchdog 0
    IRQ_WDOG1                       = 152,
    IRQ_CLKMGR                      = 153,
    IRQ_RESTMGR                     = 154,
    IRQ_FPGA_MAN                    = 155,
    IRQ_NCTIIRQ_0                   = 156,
    IRQ_NCTIIRQ_1                   = 157,
    IRQ_SEC_MGR_INTR                = 158,
    IRQ_DATABWERR                   = 159

} HPSIRQSource;

#endif

#endif /* HPS_IRQ_IDS_H_ */
