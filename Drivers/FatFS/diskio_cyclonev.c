/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for Cyclone V MMC     (C)T Carpenter, 2018  */
/*-----------------------------------------------------------------------*/
/*                                                                       */
/*-----------------------------------------------------------------------*/

#ifdef __GNUC__

// FatFs lower layer API Declarations
#include "diskio.h"
// Minimal Altera HWLib for SD-Card (hwlib/)
#include "hwlib/alt_sdmmc.h"
#include "hwlib/socal/hps.h"
// C Standard Libs
#include <stdlib.h>
#include <stdbool.h>

#undef DEBUG

#ifdef DEBUG
    //Include printf headers if debugging
    #include <stdio.h>
    #include <string.h>
#else
    //Disable printf if not debugging
    #define printf(...) (0)
    #pragma push
    #pragma diag_suppress 174
#endif

/*-----------------------------------------------------------------------*/
/* Global Variables                                                      */
/*-----------------------------------------------------------------------*/

// Card information
ALT_SDMMC_CARD_INFO_t Card_Info;

// SD/MMC Device size - hardcode max supported size for now
uint64_t Sdmmc_Device_Size;

// SD/MMC Block size
uint32_t Sdmmc_Block_Size;

// SD/MMC Block size
uint32_t Sdmmc_Sector_Size;

// Card Initialised
bool Sdmmc_Initialised = false;

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

    if(alt_sdmmc_card_identify(&Card_Info) != ALT_E_SUCCESS) {
        stat = stat + STA_NODISK; //Set the no-disk flag if we failed to identify.
        goto error;
    }

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
    
    if(alt_sdmmc_card_bus_width_set(&Card_Info, ALT_SDMMC_BUS_WIDTH_4) != ALT_E_SUCCESS) {
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
    Sdmmc_Sector_Size = 512;
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
    unsigned int address = sector * Sdmmc_Sector_Size; //Convert sector to byte address
    unsigned int size = count * Sdmmc_Sector_Size; //Convert sector count to byte size

    //Ensure aligned data buffer.
    uint32_t alignedBuff[size/sizeof(uint32_t)];
    BYTE* alignedBuffBytePtr;
    if (((uint32_t)buff) & 3) {
        //If the memory buffer is non-aligned to the 32bit boundary
        //Our aligned buffer is the temporary buffer
        alignedBuffBytePtr = (BYTE*)alignedBuff;
    } else {
        //Otherwise we can save some time by using the already aligned buffer.
        alignedBuffBytePtr = buff;
    }
    
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }
    printf("INFO: Reading from address 0x%08x, size = %d bytes using block I/O.\n", (int)address, (int)size);
    if (alt_sdmmc_read(&Card_Info, (void*)alignedBuffBytePtr, (void*)address, size) != ALT_E_SUCCESS) {
        return RES_ERROR;
    } else {
        if (((uint32_t)buff) & 3) {
            uint32_t i;
            //If the memory buffer is non-aligned to the 32bit boundary
            //We need to copy the data back from our temporary buffer to the real buffer.
            for(i = 0; i < size; i++) {
                buff[i] = alignedBuffBytePtr[i];
            }
        }
        return RES_OK;
    }
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{    
    //Convert sector sizes to bytes
    unsigned int address = sector * Sdmmc_Sector_Size; //Convert sector to byte address
    unsigned int size = count * Sdmmc_Sector_Size; //Convert sector count to byte size
    
    //Ensure aligned data buffer.
    uint32_t alignedBuff[size/sizeof(uint32_t)];
    const BYTE* alignedBuffBytePtr;
    if (((uint32_t)buff) & 3) {
        uint32_t i;
        //If the memory buffer is non-aligned to the 32bit boundary
        //Our aligned buffer is the temporary buffer
        alignedBuffBytePtr = (BYTE*)alignedBuff;
        //Into which our data must be copied
        for(i = 0; i < size; i++) {
            ((BYTE*)alignedBuff)[i] = buff[i];
        }
    } else {
        //Otherwise we can save some time by using the already aligned buffer.
        alignedBuffBytePtr = buff;
    }
    
    if (pdrv != 0) {
        return RES_PARERR; //Don't try if out of range.
    }
    if (!Sdmmc_Initialised) {
        return RES_NOTRDY; //Not ready.
    }
    if (alt_sdmmc_card_is_write_protected()) {
        return RES_WRPRT; //Write protected. Error.
    }
    printf("INFO: Writing to address 0x%08x, size = %d bytes using block I/O.\n", (int)address, (int)size);
    if (alt_sdmmc_write(&Card_Info, (void*)address, (void*)alignedBuffBytePtr, size) != ALT_E_SUCCESS) {
        return RES_ERROR;
    } else {
        return RES_OK;
    }
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
#ifndef DEBUG
#pragma pop
#endif

#endif //__GNUC__
