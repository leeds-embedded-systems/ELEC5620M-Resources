/*
 * Cyclone V HPS DMA Controller Peripheral IDs
 * -------------------------------------------
 *
 * Peripheral IDs for the HPS DMA controller in
 * the Cyclone V HPS bridge.
 * 
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 04/05/2024 | Creation of header.
 *
 */

#ifndef HPS_DMACONTROLLERPERIPHCV_H_
#define HPS_DMACONTROLLERPERIPHCV_H_

#include "HPS_DMAControllerEnums.h"

typedef enum {
    HPS_DMA_PERIPH_MUX_DEFAULT = 0,  // Reset default mux value
    HPS_DMA_PERIPH_MUX_FPGA    = 1,  // Select FPGA peripheral from mux (All DMA Interfaces)
    HPS_DMA_PERIPH_MUX_CAN     = 2   // Select CAN peripheral from mux (DMA Interface 4/5/6/7 only)
} HPSDmaPeriphMux;

// There are four configuratable muxes, starting with ID 4
#define HPS_DMA_PERMUX_COUNT  4
#define HPS_DMA_PERMUX_OFFSET 4

typedef enum {
    HPS_DMA_PERIPH_FPGA_0             = 0,  // FPGA soft IP interface 0
    HPS_DMA_PERIPH_FPGA_1             = 1,  // FPGA soft IP interface 1
    HPS_DMA_PERIPH_FPGA_2             = 2,  // FPGA soft IP interface 2
    HPS_DMA_PERIPH_FPGA_3             = 3,  // FPGA soft IP interface 3
    
    HPS_DMA_PERIPH_FPGA_4_OR_CAN0_IF1 = 4,  // Selectively MUXed FPGA 4 or CAN 0 interface 1 (Selected by HPSDmaPeriphMux)
    HPS_DMA_PERIPH_FPGA_5_OR_CAN0_IF2 = 5,  // Selectively MUXed FPGA 5 or CAN 0 interface 2 (Selected by HPSDmaPeriphMux)
    HPS_DMA_PERIPH_FPGA_6_OR_CAN1_IF1 = 6,  // Selectively MUXed FPGA 6 or CAN 1 interface 1 (Selected by HPSDmaPeriphMux)
    HPS_DMA_PERIPH_FPGA_7_OR_CAN1_IF2 = 7,  // Selectively MUXed FPGA 7 or CAN 1 interface 2 (Selected by HPSDmaPeriphMux)

    HPS_DMA_PERIPH_FPGA_4             = 4,  // Alias for HPS_DMA_PERIPH_FPGA_4_OR_CAN0_IF1
    HPS_DMA_PERIPH_FPGA_5             = 5,  // Alias for HPS_DMA_PERIPH_FPGA_5_OR_CAN0_IF2
    HPS_DMA_PERIPH_FPGA_6             = 6,  // Alias for HPS_DMA_PERIPH_FPGA_6_OR_CAN1_IF1
    HPS_DMA_PERIPH_FPGA_7             = 7,  // Alias for HPS_DMA_PERIPH_FPGA_7_OR_CAN1_IF2

    HPS_DMA_PERIPH_CAN0_IF1           = 4,  // Alias for HPS_DMA_PERIPH_FPGA_4_OR_CAN0_IF1
    HPS_DMA_PERIPH_CAN0_IF2           = 5,  // Alias for HPS_DMA_PERIPH_FPGA_5_OR_CAN0_IF2
    HPS_DMA_PERIPH_CAN1_IF1           = 6,  // Alias for HPS_DMA_PERIPH_FPGA_6_OR_CAN1_IF1
    HPS_DMA_PERIPH_CAN1_IF2           = 7,  // Alias for HPS_DMA_PERIPH_FPGA_7_OR_CAN1_IF2

    HPS_DMA_PERIPH_I2C0_TX            = 8,  // I2C 0 TX
    HPS_DMA_PERIPH_I2C0_RX            = 9,  // I2C 0 RX
    HPS_DMA_PERIPH_I2C1_TX            = 10, // I2C 1 TX
    HPS_DMA_PERIPH_I2C1_RX            = 11, // I2C 1 RX
    HPS_DMA_PERIPH_I2C2_TX            = 12, // I2C 2 TX
    HPS_DMA_PERIPH_I2C2_RX            = 13, // I2C 2 RX
    HPS_DMA_PERIPH_I2C3_TX            = 14, // I2C 3 TX
    HPS_DMA_PERIPH_I2C3_RX            = 15, // I2C 3 RX
    HPS_DMA_PERIPH_SPI0_MASTER_TX     = 16, // SPI 0 Master TX
    HPS_DMA_PERIPH_SPI0_MASTER_RX     = 17, // SPI 0 Master RX
    HPS_DMA_PERIPH_SPI0_SLAVE_TX      = 18, // SPI 0 Slave TX
    HPS_DMA_PERIPH_SPI0_SLAVE_RX      = 19, // SPI 0 Slave RX
    HPS_DMA_PERIPH_SPI1_MASTER_TX     = 20, // SPI 1 Master TX
    HPS_DMA_PERIPH_SPI1_MASTER_RX     = 21, // SPI 1 Master RX
    HPS_DMA_PERIPH_SPI1_SLAVE_TX      = 22, // SPI 1 Slave TX
    HPS_DMA_PERIPH_SPI1_SLAVE_RX      = 23, // SPI 1 Slave RX
    HPS_DMA_PERIPH_QSPI_FLASH_TX      = 24, // QSPI Flash TX
    HPS_DMA_PERIPH_QSPI_FLASH_RX      = 25, // QSPI Flash RX
    HPS_DMA_PERIPH_STM                = 26, // System Trace Macrocell
    HPS_DMA_PERIPH_RESERVED           = 27, // Reserved
    HPS_DMA_PERIPH_UART0_TX           = 28, // UART 0 TX
    HPS_DMA_PERIPH_UART0_RX           = 29, // UART 0 RX
    HPS_DMA_PERIPH_UART1_TX           = 30, // UART 1 TX
    HPS_DMA_PERIPH_UART1_RX           = 31  // UART 1 RX
} HPSDmaPeripheralId;

#define HPS_DMA_PERIPH_COUNT  32

/*
 * Internal Macros
 * Wrapped in #ifdef to avoid polluting other files including this header
 */
#ifdef HPS_DMACONTROLLER_C

/*
 * HW config mappings
 */


#include "HPS_DMAControllerEnums.h"

// Register definitions from HWLib for system manager
#include "Util/hwlib/cv/socal/hps.h"
#include "Util/hwlib/cv/socal/alt_sysmgr.h"
#include "Util/hwlib/cv/socal/alt_rstmgr.h"

// System Manager DMA Register
#define HPS_DMA_SYSMGR_DMAREG       (volatile unsigned int *)ALT_SYSMGR_DMA_CTL_ADDR

// System Manager DMA Peripheral Register
#define HPS_DMA_SYSMGR_DMAPERIPHREG (volatile unsigned int *)ALT_SYSMGR_DMA_PERSECURITY_ADDR

// Reset Manager DMA Control Register
#define HPS_DMA_RSTMGR_DMAREG       (volatile unsigned int *)ALT_RSTMGR_PERMODRST_ADDR
#define HPS_DMA_RSTMGR_DMAMASK      ALT_RSTMGR_PERMODRST_DMA_SET_MSK

// Map peripheral ID to system manager DMA peripheral register
static HpsErr_t _HPS_DMA_periphSecToSysMgrPeriph(HPSDmaPeripheralId periphId, HPSDmaSecurity secVal, unsigned int* sysMgrPeriphReg) {
    switch (secVal) {
        case HPS_DMA_SECURITY_DEFAULT:
        case HPS_DMA_SECURITY_SECURE:
            return ERR_SUCCESS;
        case HPS_DMA_SECURITY_NONSEC:
            *sysMgrPeriphReg |= MaskInsert((secVal) == HPS_DMA_SECURITY_NONSEC, 0x1, periphId);
            return ERR_SUCCESS;
        default:
            return ERR_WRONGMODE;
    }
}

// Map event ID to system manager DMA register
static HpsErr_t _HPS_DMA_eventSecToSysMgr(HPSDmaEventId eventId, HPSDmaSecurity secVal, unsigned int* sysMgrReg) {
    if ((eventId >= HPS_DMA_USER_EVENT_COUNT) || (eventId < HPS_DMA_EVENT_MIN)) return ERR_BADID;
    switch (secVal) {
        case HPS_DMA_SECURITY_DEFAULT:
        case HPS_DMA_SECURITY_SECURE:
            return ERR_SUCCESS;
        case HPS_DMA_SECURITY_NONSEC:
            *sysMgrReg |= MaskCreate(0x1, (eventID) + ALT_SYSMGR_DMA_CTL_IRQNONSECURE_LSB);
            return ERR_SUCCESS;
        default:
            return ERR_WRONGMODE;
    }
}

// Map DMA manager to system manager DMA register
static HpsErr_t _HPS_DMA_mgrSecToSysMgr(HPSDmaSecurity secVal, unsigned int* sysMgrReg) {
    switch (secVal) {
        case HPS_DMA_SECURITY_DEFAULT:
        case HPS_DMA_SECURITY_SECURE:
            return ERR_SUCCESS;
        case HPS_DMA_SECURITY_NONSEC:
            *sysMgrReg |= ALT_SYSMGR_DMA_CTL_MGRNONSECURE_SET_MSK;
            return ERR_SUCCESS;
        default:
            return ERR_WRONGMODE;
    }
}
// Map peripheral mux settings to system manager DMA register
//  - Maps all multiplexer groups
static HpsErr_t _HPS_DMA_periphMuxToSysMgr(HPSDmaPeriphMux src[HPS_DMA_PERMUX_COUNT], unsigned int* sysMgrReg) {
    for (unsigned int srcIdx = 0; srcIdx < HPS_DMA_PERMUX_COUNT; srcIdx++) {
        switch (src[srcIdx]) {
            case HPS_DMA_PERIPH_MUX_DEFAULT:
            case HPS_DMA_PERIPH_MUX_FPGA:
                continue;
            case HPS_DMA_PERIPH_MUX_CAN:
                *sysMgrReg |= MaskCreate(ALT_SYSMGR_DMA_CTL_CHANSEL_0_SET_MSK, srcIdx);
                return ERR_SUCCESS;
            default:
                return ERR_WRONGMODE;
        }
    }
}

#endif

#endif
