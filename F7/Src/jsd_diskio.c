// ce fichier resulte de la fusion de diskio.c, sd_diskio.c, ff_gen_drv.c  

#include "options.h"
#ifdef USE_SDCARD

/* Includes ------------------------------------------------------------------*/
#include "jsd_diskio.h"
// protos des fonctions de ce module mises a la disposition de FatFs
// et les symboles associes (fourni avec FatFs)
#include "diskio.h"
#include "stm32746g_discovery_lite_sd.h"

/* SD handle declared in "stm32746g_discovery_lite_sd.c" file */
extern SD_HandleTypeDef uSdHandle;

// 3 variables juste pour un petit reverse engineering naif
// on n'a observe aucune variation pour le moment
volatile unsigned int SD_IRQ_cnt = 0;
volatile unsigned int SD_DMA_Tx_cnt = 0;
volatile unsigned int SD_DMA_Rx_cnt = 0;


// STM32F7xx Peripherals Interrupt Handlers
/**
  * @brief  This function handles SDMMC1 global interrupt request.
  */
void BSP_SDMMC_IRQHandler(void)
{
  HAL_SD_IRQHandler(&uSdHandle);
  SD_IRQ_cnt++;
}

/**
* @brief  This function handles DMA2 Stream 6 interrupt request.
*/
void BSP_SDMMC_DMA_Tx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(uSdHandle.hdmatx);
  SD_DMA_Tx_cnt++;
}

/**
* @brief  This function handles DMA2 Stream 3 interrupt request.
*/
void BSP_SDMMC_DMA_Rx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(uSdHandle.hdmarx);
  SD_DMA_Rx_cnt++;
}

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* use the default SD timout as defined in the platform BSP driver*/
#if defined(SDMMC_DATATIMEOUT)
#define SD_TIMEOUT SDMMC_DATATIMEOUT
#elif defined(SD_DATATIMEOUT)
#define SD_TIMEOUT SD_DATATIMEOUT
#else
#define SD_TIMEOUT 30 * 1000
#endif

#define SD_DEFAULT_BLOCK_SIZE 512

/* global permanent Disk status */
volatile DSTATUS zestatus = STA_NOINIT;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static DSTATUS SD_CheckStatus();

/* Private functions ---------------------------------------------------------*/
static DSTATUS SD_CheckStatus()
{
if	( BSP_SD_GetCardState() == MSD_OK )
	zestatus = 0;
else	zestatus = STA_NOINIT;
return zestatus;
}

/** fonctions exposees a FatFs */

DSTATUS disk_initialize( BYTE pdrv )
{
if	( zestatus & STA_NOINIT)
	{
	if	( BSP_SD_Init() == MSD_OK )
		zestatus = SD_CheckStatus();
	}
return zestatus;
}

DSTATUS disk_status( BYTE pdrv )
{
return zestatus;
}

DRESULT disk_read( BYTE pdrv, BYTE* buff, DWORD sector, UINT count )
{
DRESULT res = RES_ERROR;
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;
if	( BSP_SD_ReadBlocks( (uint32_t*)buff,
	  (uint32_t)sector, count, SD_TIMEOUT ) == MSD_OK )
	{
	/* wait until the read operation is finished */
	while	( BSP_SD_GetCardState()!= MSD_OK )
		{}
	res = RES_OK;
	}
return res;
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
DRESULT res = RES_ERROR;
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;
if	( BSP_SD_WriteBlocks( (uint32_t*)buff,
	  (uint32_t)(sector), count, SD_TIMEOUT) == MSD_OK )
	{
	/* wait until the Write operation is finished */
	while	( BSP_SD_GetCardState() != MSD_OK )
		{}
	res = RES_OK;
	}
return res;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff )
{
DRESULT res = RES_ERROR;
BSP_SD_CardInfo CardInfo;
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;

switch	( cmd )
	{
	/* Make sure that no pending write process */
	case CTRL_SYNC :
		res = RES_OK;
	break;
	/* Get number of sectors on the disk (DWORD) */
	case GET_SECTOR_COUNT :
		BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = CardInfo.LogBlockNbr;
		res = RES_OK;
	break;
	/* Get R/W sector size (WORD) */
	case GET_SECTOR_SIZE :
		BSP_SD_GetCardInfo(&CardInfo);
		*(WORD*)buff = CardInfo.LogBlockSize;
		res = RES_OK;
	break;
	/* Get erase block size in unit of sector (DWORD) */
	case GET_BLOCK_SIZE :
		BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
		res = RES_OK;
	break;
	default:
	res = RES_PARERR;
	}
return res;
}

/* 332-bit timestamp, from MSB :
	7 bits : Year from 1980 (0..127)
	4 bits : Month (1..12)
	5 bits : Day of the month(1..31)
	5 bits : Hour (0..23)
	6 bits : Minute (0..59)
	5 bits : Second / 2 (0..29)
yyyy yyym mmmd dddd hhhh hmmm mmms ssss
*/
DWORD get_fattime (void)
{
// yyyy yyym mmmd dddd hhhh hmmm mmms ssss
// 0010 1101 0100 1101 0011 0000 1100 0011
return 0x2D4D30C3;
}
#endif /* #ifdef USE_SDCARD */
