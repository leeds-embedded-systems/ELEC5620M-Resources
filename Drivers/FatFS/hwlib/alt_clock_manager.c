/******************************************************************************
*
* Copyright 2013 Altera Corporation. All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

/*
 * $Id: //acds/rel/15.0/embedded/ip/hps/altera_hps/hwlib/src/hwmgr/soc_cv_av/alt_clock_manager.c#1 $
 */

#ifdef __GNUC__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "socal/hps.h"
#include "socal/socal.h"
#include "socal/alt_sysmgr.h"
#include "hwlib.h"
#include "alt_clock_manager.h"
#include "alt_mpu_registers.h"

// NOTE: To enable debugging output, delete the next line and uncomment the
//   line after.
#define dprintf(...)
// #define dprintf(fmt, ...) printf(fmt, ##__VA_ARGS__)

#define UINT12_MAX              (4096)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Useful Structures ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


        /* General structure used to hold parameters of various clock entities, */
typedef struct ALT_CLK_PARAMS_s
{
    alt_freq_t      freqcur;                   // current frequency of the clock
    alt_freq_t      freqmin;                   // minimum allowed frequency for this clock
    alt_freq_t      freqmax;                   // maximum allowed frequency for this clock
    uint32_t        guardband : 7;             // guardband percentage (0-100) if this clock
                                               //    is a PLL, ignored otherwise
    uint32_t        active    : 1;             // current state of activity of this clock
} ALT_CLK_PARAMS_t;


typedef struct ALT_EXT_CLK_PARAMBLOK_s
{
    ALT_CLK_PARAMS_t        clkosc1;        // ALT_CLK_OSC1
    ALT_CLK_PARAMS_t        clkosc2;        // ALT_CLK_OSC2
    ALT_CLK_PARAMS_t        periph;         // ALT_CLK_F2H_PERIPH_REF
    ALT_CLK_PARAMS_t        sdram;          // ALT_CLK_F2H_SDRAM_REF
} ALT_EXT_CLK_PARAMBLOK_t;


        /* Initializes the External Clock Frequency Limits block                        */
        /* The first field is the current external clock frequency, and can be set by   */
        /* alt_clk_ext_clk_freq_set(), the second and third fields are the minimum and  */
        /* maximum frequencies, the fourth field is ignored, and the fifth field        */
        /* contains the current activity state of the clock, 1=active, 0=inactive.      */
        /* Values taken from Section 2.3 and Section 2.7.1 of the HHP HPS-Clocking      */
        /* NPP specification.                                                           */
static ALT_EXT_CLK_PARAMBLOK_t alt_ext_clk_paramblok =
{
    { 25000000, 10000000, 50000000, 0, 1 },
    { 25000000, 10000000, 50000000, 0, 1 },
    {        0, 10000000, 50000000, 0, 1 },
    {        0, 10000000, 50000000, 0, 1 }
};


        /* PLL frequency limits */
typedef struct ALT_PLL_CLK_PARAMBLOK_s
{
    ALT_CLK_PARAMS_t       MainPLL_600;         // Main PLL values for 600 MHz SoC
    ALT_CLK_PARAMS_t       PeriphPLL_600;       // Peripheral PLL values for 600 MHz SoC
    ALT_CLK_PARAMS_t       SDRAMPLL_600;        // SDRAM PLL values for 600 MHz SoC
    ALT_CLK_PARAMS_t       MainPLL_800;         // Main PLL values for 800 MHz SoC
    ALT_CLK_PARAMS_t       PeriphPLL_800;       // Peripheral PLL values for 800 MHz SoC
    ALT_CLK_PARAMS_t       SDRAMPLL_800;        // SDRAM PLL values for 800 MHz SoC
} ALT_PLL_CLK_PARAMBLOK_t;


    /* Initializes the PLL frequency limits block                               */
    /* The first field is the current frequency, the second and third fields    */
    /* are the design limits of the PLLs as listed in Section 3.2.1.2 of the    */
    /* HHP HPS-Clocking NPP document. The fourth field of each line is the      */
    /* guardband percentage, and the fifth field of each line is the current    */
    /* state of the PLL, 1=active, 0=inactive.                                  */
#define     ALT_ORIGINAL_GUARDBAND_VAL      20
#define     ALT_GUARDBAND_LIMIT             20

        /* PLL counter frequency limits */
typedef struct ALT_PLL_CNTR_FREQMAX_s
{
    alt_freq_t       MainPLL_C0;         // Main PLL Counter 0 parameter block
    alt_freq_t       MainPLL_C1;         // Main PLL Counter 1 parameter block
    alt_freq_t       MainPLL_C2;         // Main PLL Counter 2 parameter block
    alt_freq_t       MainPLL_C3;         // Main PLL Counter 3 parameter block
    alt_freq_t       MainPLL_C4;         // Main PLL Counter 4 parameter block
    alt_freq_t       MainPLL_C5;         // Main PLL Counter 5 parameter block
    alt_freq_t       PeriphPLL_C0;       // Peripheral PLL Counter 0 parameter block
    alt_freq_t       PeriphPLL_C1;       // Peripheral PLL Counter 1 parameter block
    alt_freq_t       PeriphPLL_C2;       // Peripheral PLL Counter 2 parameter block
    alt_freq_t       PeriphPLL_C3;       // Peripheral PLL Counter 3 parameter block
    alt_freq_t       PeriphPLL_C4;       // Peripheral PLL Counter 4 parameter block
    alt_freq_t       PeriphPLL_C5;       // Peripheral PLL Counter 5 parameter block
    alt_freq_t       SDRAMPLL_C0;        // SDRAM PLL Counter 0 parameter block
    alt_freq_t       SDRAMPLL_C1;        // SDRAM PLL Counter 1 parameter block
    alt_freq_t       SDRAMPLL_C2;        // SDRAM PLL Counter 2 parameter block
    alt_freq_t       SDRAMPLL_C5;        // SDRAM PLL Counter 5 parameter block
} ALT_PLL_CNTR_FREQMAX_t;

//
// The following pll max frequency array statically defined must be recalculated each time 
// when powering up, by calling alt_clk_clkmgr_init()
//
// for 14.1 uboot preloader, the following values are calculated dynamically.
//
// Arrial 5
// alt_pll_cntr_maxfreq.MainPLL_C0   = 1050000000
// alt_pll_cntr_maxfreq.MainPLL_C1   =  350000000
// alt_pll_cntr_maxfreq.MainPLL_C2   =  262500000
// alt_pll_cntr_maxfreq.MainPLL_C3   =  350000000
// alt_pll_cntr_maxfreq.MainPLL_C4   =    2050781
// alt_pll_cntr_maxfreq.MainPLL_C5   =  116666666
// alt_pll_cntr_maxfreq.PeriphPLL_C0 =    1953125
// alt_pll_cntr_maxfreq.PeriphPLL_C1 =  250000000
// alt_pll_cntr_maxfreq.PeriphPLL_C2 =    1953125
// alt_pll_cntr_maxfreq.PeriphPLL_C3 =  200000000
// alt_pll_cntr_maxfreq.PeriphPLL_C4 =  200000000
// alt_pll_cntr_maxfreq.PeriphPLL_C5 =    1953125
// alt_pll_cntr_maxfreq.SDRAMPLL_C0  =  533333333
// alt_pll_cntr_maxfreq.SDRAMPLL_C1  = 1066666666
// alt_pll_cntr_maxfreq.SDRAMPLL_C2  =  533333333
// alt_pll_cntr_maxfreq.SDRAMPLL_C5  =  177777777

// Cyclone V 
// alt_pll_cntr_maxfreq.MainPLL_C0   =  925000000
// alt_pll_cntr_maxfreq.MainPLL_C1   =  370000000
// alt_pll_cntr_maxfreq.MainPLL_C2   =  462500000
// alt_pll_cntr_maxfreq.MainPLL_C3   =  370000000
// alt_pll_cntr_maxfreq.MainPLL_C4   =    3613281
// alt_pll_cntr_maxfreq.MainPLL_C5   =  123333333
// alt_pll_cntr_maxfreq.PeriphPLL_C0 =    1953125
// alt_pll_cntr_maxfreq.PeriphPLL_C1 =  250000000
// alt_pll_cntr_maxfreq.PeriphPLL_C2 =    1953125
// alt_pll_cntr_maxfreq.PeriphPLL_C3 =  200000000
// alt_pll_cntr_maxfreq.PeriphPLL_C4 =  200000000
// alt_pll_cntr_maxfreq.PeriphPLL_C5 =    1953125
// alt_pll_cntr_maxfreq.SDRAMPLL_C0  =  400000000
// alt_pll_cntr_maxfreq.SDRAMPLL_C1  =  800000000
// alt_pll_cntr_maxfreq.SDRAMPLL_C2  =  400000000
// alt_pll_cntr_maxfreq.SDRAMPLL_C5  =  133333333



        /* Maximum multiply, divide, and counter divisor values for each PLL */
#define     ALT_CLK_PLL_MULT_MAX        4096
#define     ALT_CLK_PLL_DIV_MAX         64
#define     ALT_CLK_PLL_CNTR_MAX        511


        /* Definitions for the reset request and reset acknowledge bits    */
        /* for each of the output counters for each of the PLLS            */
#define     ALT_CLK_PLL_RST_BIT_C0      0x00000001
#define     ALT_CLK_PLL_RST_BIT_C1      0x00000002
#define     ALT_CLK_PLL_RST_BIT_C2      0x00000004
#define     ALT_CLK_PLL_RST_BIT_C3      0x00000008
#define     ALT_CLK_PLL_RST_BIT_C4      0x00000010
#define     ALT_CLK_PLL_RST_BIT_C5      0x00000020


        /* These are the bits that deal with PLL lock and this macro */
        /* defines a mask to test for bits outside of these */
#define ALT_CLK_MGR_PLL_LOCK_BITS  (ALT_CLKMGR_INTREN_MAINPLLACHIEVED_CLR_MSK \
                                  & ALT_CLKMGR_INTREN_PERPLLACHIEVED_CLR_MSK \
                                  & ALT_CLKMGR_INTREN_SDRPLLACHIEVED_CLR_MSK \
                                  & ALT_CLKMGR_INTREN_MAINPLLLOST_CLR_MSK \
                                  & ALT_CLKMGR_INTREN_PERPLLLOST_CLR_MSK \
                                  & ALT_CLKMGR_INTREN_SDRPLLLOST_CLR_MSK)


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Utility functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/****************************************************************************************/
/* alt_clk_mgr_wait() introduces a delay, not very exact, but very light in             */
/* implementation. Depending upon the optimization level, it will wait at least the     */
/* number of clock cycles specified in the cnt parameter, sometimes many more. The      */
/* reg parameter is set to a register or a memory location that was recently used (so   */
/* as to avoid accidently evicting a register and a recently-used cache line in favor   */
/* of one whose values are not actually needed.). The cnt parameter sets the number of  */
/* repeated volatile memory reads and so sets a minimum time delay measured in          */
/* mpu_clk cycles. If mpu_clk = osc1 clock (as in bypass mode), then this gives a       */
/* minimum osc1 clock cycle delay.                                                      */
/****************************************************************************************/

    /* Wait time constants */
    /* These values came from Section 4.9.4 of the HHP HPS-Clocking NPP document */
#define ALT_SW_MANAGED_CLK_WAIT_CTRDIV          30      /* 30 or more MPU clock cycles */
#define ALT_SW_MANAGED_CLK_WAIT_HWCTRDIV        40
#define ALT_SW_MANAGED_CLK_WAIT_BYPASS          30
#define ALT_SW_MANAGED_CLK_WAIT_SAFEREQ         30
#define ALT_SW_MANAGED_CLK_WAIT_SAFEEXIT        30
#define ALT_SW_MANAGED_CLK_WAIT_NANDCLK         8       /* 8 or more MPU clock cycles */


#define ALT_BYPASS_TIMEOUT_CNT      50
        // arbitrary number until i find more info
#define ALT_TIMEOUT_PHASE_SYNC      300
        // how many loops to wait for the SDRAM clock to come around
        // to zero and allow for writing a new divisor ratio to it

ALT_STATUS_CODE alt_clk_plls_settle_wait(void)
{
    int32_t     i = ALT_BYPASS_TIMEOUT_CNT;
    bool        nofini;

    do
    {
        nofini = alt_read_word(ALT_CLKMGR_STAT_ADDR) & ALT_CLKMGR_STAT_BUSY_SET_MSK;
    } while (nofini && i--);
            // wait until clocks finish transitioning and become stable again
    return (i > 0) ? ALT_E_SUCCESS : ALT_E_ERROR;
}

        /* Useful utility macro for checking if two values  */
        /* are within a certain percentage of each other    */
#define  alt_within_delta(ref, neu, prcnt)  (((((neu) * 100)/(ref)) < (100 + (prcnt))) \
                                            && ((((neu) * 100)/(ref)) > (100 - (prcnt))))


        /* Flags to include or omit code sections */
// There are four cases where there is a small possibility of producing clock
// glitches. Code has been added from an abundance of caution to prevent
// these glitches. If further testing shows that this extra code is not necessary
// under any conditions, it may be easily eliminated by clearing these flags.

#define ALT_PREVENT_GLITCH_BYP              true
// for PLL entering or leaving bypass
#define ALT_PREVENT_GLITCH_EXSAFE           true
// for PLL exiting safe mode
#define ALT_PREVENT_GLITCH_CNTRRST          true
// resets counter phase
#define ALT_PREVENT_GLITCH_CHGC1            true
// for changing Main PLL C1 counter


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/****************************************************************************************/
/* alt_clk_lock_status_clear() clears assertions of one or more of the PLL lock status  */
/* conditions.                                                                          */
/****************************************************************************************/

ALT_STATUS_CODE alt_clk_lock_status_clear(ALT_CLK_PLL_LOCK_STATUS_t lock_stat_mask)
{
    if (lock_stat_mask & (  ALT_CLKMGR_INTER_MAINPLLACHIEVED_CLR_MSK
                          & ALT_CLKMGR_INTER_PERPLLACHIEVED_CLR_MSK
                          & ALT_CLKMGR_INTER_SDRPLLACHIEVED_CLR_MSK
                          & ALT_CLKMGR_INTER_MAINPLLLOST_CLR_MSK
                          & ALT_CLKMGR_INTER_PERPLLLOST_CLR_MSK
                          & ALT_CLKMGR_INTER_SDRPLLLOST_CLR_MSK)
        )
    {
        return ALT_E_BAD_ARG;
    }
    else
    {
        alt_setbits_word(ALT_CLKMGR_INTER_ADDR, lock_stat_mask);
        return ALT_E_SUCCESS;
    }
}


/****************************************************************************************/
/* alt_clk_lock_status_get() returns the value of the PLL lock status conditions.       */
/****************************************************************************************/

uint32_t alt_clk_lock_status_get(void)
{
    return alt_read_word(ALT_CLKMGR_INTER_ADDR) & (  ALT_CLKMGR_INTER_MAINPLLACHIEVED_SET_MSK
                                                   | ALT_CLKMGR_INTER_PERPLLACHIEVED_SET_MSK
                                                   | ALT_CLKMGR_INTER_SDRPLLACHIEVED_SET_MSK
                                                   | ALT_CLKMGR_INTER_MAINPLLLOST_SET_MSK
                                                   | ALT_CLKMGR_INTER_PERPLLLOST_SET_MSK
                                                   | ALT_CLKMGR_INTER_SDRPLLLOST_SET_MSK
                                                   | ALT_CLKMGR_INTER_MAINPLLLOCKED_SET_MSK
                                                   | ALT_CLKMGR_INTER_PERPLLLOCKED_SET_MSK
                                                   | ALT_CLKMGR_INTER_SDRPLLLOCKED_SET_MSK );
}


/****************************************************************************************/
/* alt_clk_pll_is_locked() returns ALT_E_TRUE if the designated PLL is currently        */
/* locked and ALT_E_FALSE if not.                                                       */
/****************************************************************************************/

ALT_STATUS_CODE alt_clk_pll_is_locked(ALT_CLK_t pll)
{
    ALT_STATUS_CODE status = ALT_E_BAD_ARG;

    if (pll == ALT_CLK_MAIN_PLL)
    {
        status = (alt_read_word(ALT_CLKMGR_INTER_ADDR) & ALT_CLKMGR_INTER_MAINPLLLOCKED_SET_MSK)
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    else if (pll == ALT_CLK_PERIPHERAL_PLL)
    {
        status = (alt_read_word(ALT_CLKMGR_INTER_ADDR) & ALT_CLKMGR_INTER_PERPLLLOCKED_SET_MSK)
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    else if (pll == ALT_CLK_SDRAM_PLL)
    {
        status = (alt_read_word(ALT_CLKMGR_INTER_ADDR) & ALT_CLKMGR_INTER_SDRPLLLOCKED_SET_MSK)
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    return status;
}







/****************************************************************************************/
/* alt_clk_pll_is_bypassed() returns whether the specified PLL is in bypass or not.     */
/* Bypass is a special state where the PLL VCO and the C0-C5 counters are bypassed      */
/* and not in the circuit. Either the Osc1 clock input or the input chosen by the       */
/* input mux may be selected to be operational in the bypass state. All changes to      */
/* the PLL VCO must be made in bypass mode to avoid the potential of producing clock    */
/* glitches which may affect downstream clock dividers and peripherals.                 */
/****************************************************************************************/

ALT_STATUS_CODE alt_clk_pll_is_bypassed(ALT_CLK_t pll)
{
    ALT_STATUS_CODE status = ALT_E_BAD_ARG;

    if (pll == ALT_CLK_MAIN_PLL)
    {
        status = (ALT_CLKMGR_CTL_SAFEMOD_GET(alt_read_word(ALT_CLKMGR_CTL_ADDR))
                || ALT_CLKMGR_BYPASS_MAINPLL_GET(alt_read_word(ALT_CLKMGR_BYPASS_ADDR)))
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    else if (pll == ALT_CLK_PERIPHERAL_PLL)
    {
        status = (ALT_CLKMGR_CTL_SAFEMOD_GET(alt_read_word(ALT_CLKMGR_CTL_ADDR))
                || ALT_CLKMGR_BYPASS_PERPLL_GET(alt_read_word(ALT_CLKMGR_BYPASS_ADDR)))
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    else if (pll == ALT_CLK_SDRAM_PLL)
    {
        status = (ALT_CLKMGR_CTL_SAFEMOD_GET(alt_read_word(ALT_CLKMGR_CTL_ADDR))
                || ALT_CLKMGR_BYPASS_SDRPLL_GET(alt_read_word(ALT_CLKMGR_BYPASS_ADDR)))
                ? ALT_E_TRUE : ALT_E_FALSE;
    }
    return status;
}


/****************************************************************************************/
/* alt_clk_pll_source_get() returns the current input of the specified PLL.             */
/****************************************************************************************/

ALT_CLK_t alt_clk_pll_source_get(ALT_CLK_t pll)
{
    ALT_CLK_t      ret = ALT_CLK_UNKNOWN;
    uint32_t       temp;


    if (pll == ALT_CLK_MAIN_PLL)
    {
        ret = ALT_CLK_IN_PIN_OSC1;
    }
    else if (pll == ALT_CLK_PERIPHERAL_PLL)
    {
        // three possible clock sources for the peripheral PLL
        temp = ALT_CLKMGR_PERPLL_VCO_PSRC_GET(alt_read_word(ALT_CLKMGR_PERPLL_VCO_ADDR));
        if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_EOSC1)
        {
            ret = ALT_CLK_IN_PIN_OSC1;
        }
        else if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_EOSC2)
        {
            ret = ALT_CLK_IN_PIN_OSC2;
        }
        else if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_F2S_PERIPH_REF)
        {
            ret = ALT_CLK_F2H_PERIPH_REF;
        }
    }
    else if (pll == ALT_CLK_SDRAM_PLL)
    {
        // three possible clock sources for the SDRAM PLL
        temp = ALT_CLKMGR_SDRPLL_VCO_SSRC_GET(alt_read_word(ALT_CLKMGR_SDRPLL_VCO_ADDR));
        if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_EOSC1)
        {
            ret = ALT_CLK_IN_PIN_OSC1;
        }
        else if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_EOSC2)
        {
            ret = ALT_CLK_IN_PIN_OSC2;
        }
        else if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_F2S_SDRAM_REF)
        {
            ret = ALT_CLK_F2H_SDRAM_REF;
        }
    }
    return ret;
}

//
// alt_clk_clock_disable() disables the specified clock. Once the clock is disabled,
// its clock signal does not propagate to its clocked elements.
//
ALT_STATUS_CODE alt_clk_clock_disable(ALT_CLK_t clk)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    switch (clk)
    {
    case ALT_CLK_SDMMC:
        alt_clrbits_word(ALT_CLKMGR_PERPLL_EN_ADDR, ALT_CLKMGR_PERPLL_EN_SDMMCCLK_SET_MSK);
        break;

    default:
        status = ALT_E_BAD_ARG;
        break;
    }

    return status;
}


//
// alt_clk_clock_enable() enables the specified clock. Once the clock is enabled, its
// clock signal propagates to its elements.
//
ALT_STATUS_CODE alt_clk_clock_enable(ALT_CLK_t clk)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    switch (clk)
    {
    case ALT_CLK_SDMMC:
        alt_setbits_word(ALT_CLKMGR_PERPLL_EN_ADDR, ALT_CLKMGR_PERPLL_EN_SDMMCCLK_SET_MSK);
        break;
    default:
        status = ALT_E_BAD_ARG;
        break;
    }

    return status;
}

//
// alt_clk_is_enabled() returns whether the specified clock is enabled or not.
//
ALT_STATUS_CODE alt_clk_is_enabled(ALT_CLK_t clk)
{
    ALT_STATUS_CODE status = ALT_E_BAD_ARG;

    switch (clk)
    {
        // For PLLs, this function checks if the PLL is bypassed or not.
    case ALT_CLK_MAIN_PLL:
    case ALT_CLK_PERIPHERAL_PLL:
    case ALT_CLK_SDRAM_PLL:
        status = (alt_clk_pll_is_bypassed(clk) != ALT_E_TRUE);
        break;
        // Clocks that may originate at the Main PLL, the Peripheral PLL, or the FPGA.
    case ALT_CLK_SDMMC:
        status = (ALT_CLKMGR_PERPLL_EN_SDMMCCLK_GET(alt_read_word(ALT_CLKMGR_PERPLL_EN_ADDR)))
            ? ALT_E_TRUE : ALT_E_FALSE;
        break;
    default:
        status = ALT_E_BAD_ARG;
        break;

    }

    return status;
}



/****************************************************************************************/
/* This enum enumerates a set of possible change methods that are available for use by  */
/* alt_clk_pll_vco_cfg_set() to change VCO parameter settings.                          */
/****************************************************************************************/

typedef enum ALT_CLK_PLL_VCO_CHG_METHOD_e
{
    ALT_VCO_CHG_NONE_VALID      = 0,            /*  No valid method to  change PLL
                                                 *  VCO was found                       */
    ALT_VCO_CHG_NOCHANGE        = 0x00000001,   /*  Proposed new VCO values are the
                                                 *  same as the old values              */
    ALT_VCO_CHG_NUM             = 0x00000002,   /*  Can change the VCO multiplier
                                                 *  alone                               */
    ALT_VCO_CHG_NUM_BYP         = 0x00000004,   /*  A VCO multiplier-only change will
                                                 *  require putting the PLL in bypass   */
    ALT_VCO_CHG_DENOM           = 0x00000008,   /*  Can change the VCO divider
                                                 *  alone                               */
    ALT_VCO_CHG_DENOM_BYP       = 0x00000010,   /*  A VCO divider-only change will
                                                 *  require putting the PLL in bypass   */
    ALT_VCO_CHG_NUM_DENOM       = 0x00000020,   /*  Can change the clock multiplier
                                                 *  first. then the clock divider       */
    ALT_VCO_CHG_NUM_DENOM_BYP   = 0x00000040,   /*  Changing the clock multiplier first.
                                                 *  then the clock divider will
                                                 *  require putting the PLL in bypass   */
    ALT_VCO_CHG_DENOM_NUM       = 0x00000080,   /*  Can change the clock divider first.
                                                 *  then the clock multiplier           */
    ALT_VCO_CHG_DENOM_NUM_BYP   = 0x00000100    /*  Changing the clock divider first.
                                                 *  then the clock multiplier will
                                                 *  require putting the PLL in bypass   */
} ALT_CLK_PLL_VCO_CHG_METHOD_t;



/****************************************************************************************/
/* alt_clk_pll_vco_chg_methods_get() determines which possible methods to change the    */
/* VCO are allowed within the limits set by the maximum PLL multiplier and divider      */
/* values and by the upper and lower frequency limits of the PLL, and also determines   */
/* whether each of these changes can be made without the PLL losing lock, which         */
/* requires the PLL to be bypassed before making changes, and removed from bypass state */
/* afterwards.                                                                          */
/****************************************************************************************/


#define ALT_CLK_PLL_VCO_CHG_METHOD_TEST_MODE        false
    // used for testing writes to the PLL VCOs




/****************************************************************************************/
/* alt_clk_pll_vco_cfg_set() sets the PLL VCO frequency configuration using the         */
/* supplied multiplier and divider arguments. alt_clk_pll_vco_chg_methods_get()         */
/* determines which methods are allowed by the limits set by the maximum multiplier     */
/* and divider values and by the upper and lower frequency limits of the PLL, and also  */
/* determines whether these changes can be made without requiring the PLL to be         */
/* bypassed. alt_clk_pll_vco_cfg_set() then carries out the actions required to effect  */
/* the method chosen to change the VCO settings.                                        */
/****************************************************************************************/


//
// alt_clk_pll_vco_freq_get() gets the VCO frequency of the specified PLL.
// Note that since there is at present no known way for software to obtain the speed
// bin of the SoC or MPU that it is running on, the function below only deals with the
// 800 MHz part. This may need to be revised in the future.
//
ALT_STATUS_CODE alt_clk_pll_vco_freq_get(ALT_CLK_t pll, alt_freq_t * freq)
{
    uint64_t            temp1 = 0;
    uint32_t            temp;
    uint32_t            numer;
    uint32_t            denom;
    ALT_STATUS_CODE     ret = ALT_E_BAD_ARG;

    if (freq == NULL)
    {
        return ret;
    }

    if (pll == ALT_CLK_MAIN_PLL)
    {
        temp = alt_read_word(ALT_CLKMGR_MAINPLL_VCO_ADDR);
        numer = ALT_CLKMGR_MAINPLL_VCO_NUMER_GET(temp);
        denom = ALT_CLKMGR_MAINPLL_VCO_DENOM_GET(temp);
        temp1 = (uint64_t) alt_ext_clk_paramblok.clkosc1.freqcur;
        temp1 *= (numer + 1);
        temp1 /= (denom + 1);

        if (temp1 <= UINT32_MAX)
        {
            temp = (alt_freq_t) temp1;
            // store this value in the parameter block table
            *freq = temp;
            // should NOT check value against PLL frequency limits
            ret = ALT_E_SUCCESS;
        }
        else
        {
            ret = ALT_E_ERROR;
        }
    }
    else if (pll == ALT_CLK_PERIPHERAL_PLL)
    {
        temp = alt_read_word(ALT_CLKMGR_PERPLL_VCO_ADDR);
        numer = ALT_CLKMGR_PERPLL_VCO_NUMER_GET(temp);
        denom = ALT_CLKMGR_PERPLL_VCO_DENOM_GET(temp);
        temp = ALT_CLKMGR_PERPLL_VCO_PSRC_GET(temp);
        if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_EOSC1)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.clkosc1.freqcur;
        }
        else if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_EOSC2)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.clkosc2.freqcur;
        }
        else if (temp == ALT_CLKMGR_PERPLL_VCO_PSRC_E_F2S_PERIPH_REF)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.periph.freqcur;
        }

        if (temp1 != 0)
        {
            temp1 *= (numer + 1);
            temp1 /= (denom + 1);
            if (temp1 <= UINT32_MAX)
            {
                temp = (alt_freq_t) temp1;
                // store this value in the parameter block table

                *freq = temp;
                ret = ALT_E_SUCCESS;
            }
            else
            {
                ret = ALT_E_ERROR;
            }
        }       // this returns ALT_BAD_ARG if the source isn't known
    }
    else if (pll == ALT_CLK_SDRAM_PLL)
    {
        temp = alt_read_word(ALT_CLKMGR_SDRPLL_VCO_ADDR);
        numer = ALT_CLKMGR_SDRPLL_VCO_NUMER_GET(temp);
        denom = ALT_CLKMGR_SDRPLL_VCO_DENOM_GET(temp);
        temp = ALT_CLKMGR_SDRPLL_VCO_SSRC_GET(temp);
        if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_EOSC1)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.clkosc1.freqcur;
        }
        else if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_EOSC2)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.clkosc2.freqcur;
        }
        else if (temp == ALT_CLKMGR_SDRPLL_VCO_SSRC_E_F2S_SDRAM_REF)
        {
            temp1 = (uint64_t) alt_ext_clk_paramblok.sdram.freqcur;
        }

        if (temp1 != 0)
        {
            temp1 *= (numer + 1);
            temp1 /= (denom + 1);
            if (temp1 <= UINT32_MAX)
            {
                temp = (alt_freq_t) temp1;
                // store this value in the parameter block table

                *freq = temp;
                ret = ALT_E_SUCCESS;
            }
            else
            {
                ret = ALT_E_ERROR;
            }
        }
    }       // which returns ALT_BAD_ARG if the source isn't known

    return ret;
}

//
// alt_clk_divider_get() gets configured divider value for the specified clock.
//
ALT_STATUS_CODE alt_clk_divider_get(ALT_CLK_t clk, uint32_t * div)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;

    if (div == NULL)
    {
        return ALT_E_BAD_ARG;
    }

    switch (clk)
    {
        // Main PLL outputs
    case ALT_CLK_MAIN_PLL_C4:
        *div = (ALT_CLKMGR_MAINPLL_MAINNANDSDMMCCLK_CNT_GET(alt_read_word(ALT_CLKMGR_MAINPLL_MAINNANDSDMMCCLK_ADDR))) + 1;
        break;

    case ALT_CLK_PERIPHERAL_PLL_C3:
        *div = (ALT_CLKMGR_PERPLL_PERNANDSDMMCCLK_CNT_GET(alt_read_word(ALT_CLKMGR_PERPLL_PERNANDSDMMCCLK_ADDR))) + 1;
        break;

    default:
        status = ALT_E_BAD_ARG;
        break;
    }

    return status;
}

/////

#define ALT_CLK_WITHIN_FREQ_LIMITS_TEST_MODE        false
    // used for testing writes to the the full range of counters without
    // regard to the usual output frequency upper and lower limits

//
// alt_clk_freq_get() returns the output frequency of the specified clock.
//
ALT_STATUS_CODE alt_clk_freq_get(ALT_CLK_t clk, alt_freq_t* freq)
{
    ALT_STATUS_CODE ret = ALT_E_BAD_ARG;
    uint32_t        temp = 0;
    uint64_t        numer = 0;
    uint64_t        denom = 1;

    if (freq == NULL)
    {
        return ret;
    }

    switch (clk)
    {

        /* Clocks That Can Switch Between Different Clock Groups */
    case ALT_CLK_SDMMC:
        temp = ALT_CLKMGR_PERPLL_SRC_SDMMC_GET(alt_read_word(ALT_CLKMGR_PERPLL_SRC_ADDR));
        if (temp == ALT_CLKMGR_PERPLL_SRC_SDMMC_E_F2S_PERIPH_REF_CLK)
        {
            numer = (uint64_t) alt_ext_clk_paramblok.periph.freqcur;
            // denom = 1 by default
            ret = ALT_E_SUCCESS;
        }
        else if (temp == ALT_CLKMGR_PERPLL_SRC_SDMMC_E_MAIN_NAND_CLK)
        {
            ret = alt_clk_pll_vco_freq_get(ALT_CLK_MAIN_PLL, &temp);
            if (ret == ALT_E_SUCCESS)
            {
                numer = (uint64_t) temp;
                ret = alt_clk_divider_get(ALT_CLK_MAIN_PLL_C4, &temp);
                denom = (uint64_t) temp;
            }
        }
        else if (temp == ALT_CLKMGR_PERPLL_SRC_SDMMC_E_PERIPH_NAND_CLK)
        {
            ret = alt_clk_pll_vco_freq_get(ALT_CLK_PERIPHERAL_PLL, &temp);
            if (ret == ALT_E_SUCCESS)
            {
                numer = (uint64_t) temp;
                ret = alt_clk_divider_get(ALT_CLK_PERIPHERAL_PLL_C3, &temp);
                denom = (uint64_t) temp;
            }
        }
        else
        {
            ret = ALT_E_ERROR;
        }
        break;

    default:
        ret = ALT_E_BAD_ARG;
        break;

    }   // end of switch-case construct

    if (ret == ALT_E_SUCCESS)
    {
        // will not get here if none of above cases match
        if (denom > 0)
        {
            numer /= denom;
            if (numer <= UINT32_MAX)
            {
                *freq = (uint32_t) numer;
            }
            else
            {
                ret = ALT_E_ERROR;
            }
        }
        else
        {
            ret = ALT_E_ERROR;
        }
    }

    return ret;
}

#endif // __GNUC__
