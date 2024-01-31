/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for Cyclone V MMC     (C)T Carpenter, 2018  */
/*-----------------------------------------------------------------------*/
/*                                                                       */
/*-----------------------------------------------------------------------*/

// FatFs lower layer API Declarations
#ifdef __ARRIA10__
#include "hwlib/a10/socal/hps.h"
#else
#include "hwlib/cv/socal/hps.h"
#endif
#include "diskio.h"
#include "ff.h"         /* Declarations of sector size */
// Minimal Altera HWLib for SD-Card (hwlib/)
#include "hwlib/alt_sdmmc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "HPS_Watchdog/HPS_Watchdog.h"


#ifdef FF_DEBUG
    //Include printf headers if debugging
    #include <stdio.h>
#else
    //Disable printf if not debugging
    #define printf(...) (0)
#endif

#ifdef FF_MULTI_PARTITION
PARTITION VolToPart[] __attribute__((weak)) = {};
#endif

#ifndef FF_SDMMC_BUS_WIDTH
    #ifdef __ARRIA10__
    #define FF_SDMMC_BUS_WIDTH ALT_SDMMC_BUS_WIDTH_1
    #else
    #define FF_SDMMC_BUS_WIDTH ALT_SDMMC_BUS_WIDTH_4
    #endif
#endif

/*-----------------------------------------------------------------------*/
/* Global Variables                                                      */
/*-----------------------------------------------------------------------*/

// Card information
static ALT_SDMMC_CARD_INFO_t Card_Info;

// SD/MMC Device size - hardcode max supported size for now
static uint64_t Sdmmc_Device_Size;

// SD/MMC Block size
static uint32_t Sdmmc_Block_Size;

// SD/MMC Block size
static uint32_t Sdmmc_Sector_Size;

// Card Initialised
static bool Sdmmc_Initialised = false;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = 0;
    if (pdrv != 0) {
        return STA_NOINIT; //No status if out of range.
    }
    
    if (!Sdmmc_Initialised) {
        return STA_NOINIT; //Not initialised yet.
    }
    
    if (alt_sdmmc_card_is_write_protected()) {
        //If the disk is write protected, set the STA_PROTECT flag.
        stat = stat + STA_PROTECT;
    }
    
    if (!alt_sdmmc_card_is_detected()) {
        //Check if a card is detected, if not, set the STA_NODISK flag.
        stat = stat + STA_NODISK;
    }
    
	return stat; //Return status.
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = 0;
    ALT_SDMMC_CARD_MISC_t card_misc_cfg;
    
    if (pdrv != 0) {
        printf("ERROR: Invalid Drive.\n");
        return STA_NOINIT; //Don't try and initialise if out of range.
    }
    
    printf("INFO: System Initialization.\n");

    // Setting up SD/MMC
    printf("INFO: Setting up SDMMC.\n");
    if(alt_sdmmc_init() != ALT_E_SUCCESS) {
        goto error;
    }

    if(alt_sdmmc_card_pwr_on() != ALT_E_SUCCESS) {
        goto error;
    }

    HPS_ResetWatchdog();
    if(alt_sdmmc_card_identify(&Card_Info) != ALT_E_SUCCESS) {
        stat = stat + STA_NODISK; //Set the no-disk flag if we failed to identify.
        goto error;
    }
    HPS_ResetWatchdog();

    switch(Card_Info.card_type)
    {
        case ALT_SDMMC_CARD_TYPE_MMC:
            printf("INFO: MMC Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SD:
            printf("INFO: SD Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDIOIO:
            printf("INFO: SDIO Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDIOCOMBO:
            printf("INFO: SDIOCOMBO Card detected.\n");
            break;
        case ALT_SDMMC_CARD_TYPE_SDHC:
            printf("INFO: SDHC Card detected.\n");
            break;
        default:
            printf("INFO: Card type unknown.\n");
            stat = stat + STA_NODISK; //Set the no-disk flag if unknown
            goto error;
    }
    
    if(alt_sdmmc_card_bus_width_set(&Card_Info, FF_SDMMC_BUS_WIDTH) != ALT_E_SUCCESS) {
        goto error;
    }

    if(alt_sdmmc_fifo_param_set((ALT_SDMMC_FIFO_NUM_ENTRIES >> 3) - 1,
                                (ALT_SDMMC_FIFO_NUM_ENTRIES >> 3), 
                                (ALT_SDMMC_MULT_TRANS_TXMSIZE1)
                                ) != ALT_E_SUCCESS) {
        goto error;
    }

    if (alt_sdmmc_card_misc_get(&card_misc_cfg) != ALT_E_SUCCESS) {
        goto error;
    }

    printf("INFO: Card width = %d.\n", card_misc_cfg.card_width);
    printf("INFO: Card block size = %d.\n", (int)card_misc_cfg.block_size);
    Sdmmc_Block_Size = card_misc_cfg.block_size;
    Sdmmc_Device_Size = ((uint64_t)Card_Info.blk_number_high << 32) + Card_Info.blk_number_low;
    Sdmmc_Device_Size *= Card_Info.max_r_blkln;
    Sdmmc_Sector_Size = (Card_Info.max_r_blkln > 512) ? 512 : Card_Info.max_r_blkln;
    printf("INFO: Card size = %lld.\n", Sdmmc_Device_Size);

    if(alt_sdmmc_dma_enable() != ALT_E_SUCCESS) {
        goto error;
    }
    
    if (alt_sdmmc_card_is_write_protected()) {
        printf("WARN: Card is write protected.\n");
        //If the disk is write protected, set the STA_PROTECT flag (this is not an error).
        stat = stat + STA_PROTECT;
    }
    
    printf("\n");
    
    Sdmmc_Initialised = true; //Now we are initialised.
    return stat;
    
error:
    Sdmmc_Initialised = false; //We are not initialised
    stat = stat + STA_NOINIT; //Ensure the NOINIT flag is set.
    return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    // Validate disk condition
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }
    // Must have a read buffer
    if (!buff) {
        return RES_PARERR;
    }

    // 32-bit aligned data buffer for sector reads used if incoming buffer is not aligned.
    uint32_t alignedBuff[Sdmmc_Sector_Size/sizeof(uint32_t)];

    // Work through each sector to be read
    UINT remain = count;
    UINT start = sector;
    ALT_STATUS_CODE sdmmcStat;
    printf("FatFS: Block Read %u Sectors. Start at %u (@ 0x%08x).\n", (UINT)count, (UINT)sector, (UINT)(sector * Sdmmc_Sector_Size));
    while (remain--) {
        //Convert current sector to byte address
        unsigned int address = sector * Sdmmc_Sector_Size;

        //Ensure aligned data buffer.
        const BYTE* readBuff;
        if (((uint32_t)buff) & 3) {
            //If the memory buffer is non-aligned to the 32bit boundary, so use our
            //internal aligned buffer for the read.
            readBuff = (BYTE*)alignedBuff;
        } else {
            //Otherwise we can save some time by using the already aligned buffer.
            readBuff = buff;
        }

        sdmmcStat = alt_sdmmc_read(&Card_Info, (void*)readBuff, (void*)address, Sdmmc_Sector_Size);
        if (sdmmcStat != ALT_E_SUCCESS) {
            printf("FatFS: Sec %u/%u (@ 0x%08x) Read Err %d.\n", (UINT)(sector - start + 1), (UINT)count, (UINT)address, sdmmcStat);
            return RES_ERROR;
        }

        if (((uint32_t)buff) & 3) {
            //If it was a non-aligned read, copy from our internal buffer to the user
            memcpy(buff, alignedBuff, Sdmmc_Sector_Size);
        }

        // Move on to the next sector
        sector++;
        buff += Sdmmc_Sector_Size;
        HPS_ResetWatchdog();
    }
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
// Special write case: if `buff == NULL`, will zero out each sector.

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    // Validate disk condition
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }
    if (alt_sdmmc_card_is_write_protected()) {
        return RES_WRPRT; //Write protected. Error.
    }

    // 32-bit aligned data buffer for sector writes used if incoming buffer is not aligned.
    uint32_t alignedBuff[Sdmmc_Sector_Size/sizeof(uint32_t)];
    if (!buff) {
        // If no write buffer, we are going to write 0's, so zero out the aligned buffer
        memset(alignedBuff, 0, Sdmmc_Sector_Size);
    }
    
    // Work through each sector to be written
    UINT remain = count;
    UINT start = sector;
    ALT_STATUS_CODE sdmmcStat;
    printf("FatFS: Block Write %u Sectors. Start at %u (@ 0x%08x).\n", (UINT)count, (UINT)sector, (UINT)(sector * Sdmmc_Sector_Size));
    while (remain--) {
        //Convert current sector to byte address
        unsigned int address = sector * Sdmmc_Sector_Size;

        //Ensure aligned data buffer.
        const BYTE* writeBuff;
        if (!buff) {
            //If no write buffer, use aligned buffer (zero filled)
            writeBuff = (BYTE*)alignedBuff;
        } else if (((uint32_t)buff) & 3) {
            //If the memory buffer is non-aligned to the 32bit boundary, copy it
            //into our internal correctly aligned buffer
            memcpy(alignedBuff, buff, Sdmmc_Sector_Size);
            //Our aligned buffer is the one we want to write
            writeBuff = (BYTE*)alignedBuff;
        } else {
            //Otherwise we can save some time by using the already aligned buffer.
            writeBuff = buff;
        }

        // Write the sector
        sdmmcStat = alt_sdmmc_write(&Card_Info, (void*)address, (void*)writeBuff, Sdmmc_Sector_Size);
        if (sdmmcStat != ALT_E_SUCCESS) {
            printf("FatFS: Sec %u/%u (@ 0x%08x) Write Err %d.\n", (UINT)(sector - start + 1), (UINT)count, (UINT)address, sdmmcStat);
            return RES_ERROR;
        }

        // Move on to the next sector
        sector++;
        if (buff) {
            buff += Sdmmc_Sector_Size;
        }
        HPS_ResetWatchdog();
    }
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Verify Sector(s)                                                      */
/*-----------------------------------------------------------------------*/
// Special verify case: if `buff == NULL`, will check that each sector is zeros.

DRESULT disk_verify (
    BYTE pdrv,          /* Physical drive number to identify the drive */
    const BYTE *buff,   /* Data buffer that was written */
    DWORD sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to verify */
)
{
    // Validate disk condition
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }

    // 32-bit aligned data buffer for sector reads used if incoming buffer is not aligned.
    uint32_t alignedBuff[Sdmmc_Sector_Size/sizeof(uint32_t)];

    // Work through each sector to be read
    UINT remain = count;
    UINT start = sector;
    ALT_STATUS_CODE sdmmcStat;
    printf("FatFS: Block Verify %u Sectors. Start at %u (@ 0x%08x).\n", (UINT)count, (UINT)sector, (UINT)(sector * Sdmmc_Sector_Size));
    while (remain--) {
        //Convert current sector to byte address
        unsigned int address = sector * Sdmmc_Sector_Size;

        //Read the sector into the alignedBuff which we will compare against the input buff
        const BYTE* verifyBuff = (BYTE*)alignedBuff;

        sdmmcStat = alt_sdmmc_read(&Card_Info, (void*)verifyBuff, (void*)address, Sdmmc_Sector_Size);
        if (sdmmcStat != ALT_E_SUCCESS) {
            printf("FatFS: Sec %u/%u (@ 0x%08x) Read Err %d.\n", (UINT)(sector - start + 1), (UINT)count, (UINT)address, sdmmcStat);
            return RES_ERROR;
        }
        if (!buff) {
            // No buffer, verify against being all zeros.
            for (unsigned int idx = 0; idx < (Sdmmc_Sector_Size/sizeof(uint32_t)); idx++) {
                if (alignedBuff[idx]) {
                    goto verifyError;
                }
            }
        } else {
            if (memcmp(buff, verifyBuff, Sdmmc_Sector_Size)) {
verifyError:
                printf("FatFS: Sec %u/%u (@ 0x%08x) Verify Err %d.\n", (UINT)(sector - start + 1), (UINT)count, (UINT)address, sdmmcStat);
                return RES_ERROR;
            }
        }

        // Move on to the next sector
        sector++;
        if (buff) {
            buff += Sdmmc_Sector_Size;
        }
        HPS_ResetWatchdog();
    }
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
// assumes (FF_USE_TRIM == 0)

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }
    
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD*)buff) = Sdmmc_Device_Size / Sdmmc_Sector_Size;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(DWORD*)buff = Sdmmc_Sector_Size;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = Sdmmc_Block_Size;
            return RES_OK;
    };
    return RES_PARERR; //Invalid parameter
}

