// ce fichier resulte de la fusion de diskio.c, sd_diskio.c, ff_gen_drv.c  

#include "options.h"
#ifdef USE_SDCARD

/* Includes ------------------------------------------------------------------*/
#include "jsd_diskio.h"
#include "s_gpio.h"
// protos des fonctions de ce module mises a la disposition de FatFs
// et les symboles associes (fourni avec FatFs)
#include "diskio.h"
#include "stm32746g_discovery.h"
#include "stm32f7xx_hal_sd.h"

// global storage : HAL style handle */
SD_HandleTypeDef uSdHandle;
/* global permanent Disk status pour FatFs */
volatile DSTATUS zestatus = STA_NOINIT;

//#define USE_SD_INTERRUPT

#ifdef USE_SD_INTERRUPT
/*
HAL_SD_IRQHandler() : Interrupt routine associee qui 
	1) appelle en fonction des flags :
	   SDMMC_CmdStopTransfer
		CMD12 'STOP'
	   SD_Read_IT
		lire 8 mots de 32 bits dans le fifo (qui en contient 32)
		incrementer l'adresse pRxBuffPtr dans la handle
	   SD_Write_IT
		ecrire 8 mots de 32 bits dans le fifo (qui en contient 32)
		incrementer l'adresse pTxBuffPtr dans la handle
	2) collecter les erreurs eventuelles dans hsd->ErrorCode
Cette fonction doit etre appelee par le "vrai" handler SDMMC1_IRQHandler
Note : cette fonction est tres encombree par le traitement DMA
*/
volatile unsigned int SD_IRQ_cnt = 0;
void SDMMC1_IRQHandler(void)
{
HAL_SD_IRQHandler(&uSdHandle);
SD_IRQ_cnt++;
}
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define SD_TIMEOUT 301000	// au pif... c'est pour readblocks et writeblocks
#define SD_DEFAULT_BLOCK_SIZE 512


/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static DSTATUS jSD_CheckStatus()
{
// HAL_SD_CARD_TRANSFER c'est l'etat repos normal quand tout est initialise
if	( HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER )
	zestatus = 0;
else	zestatus = STA_NOINIT;
return zestatus;
}

/** fonctions exposees a FatFs */

// cette fontion ne travaille que si le bit STA_NOINIT est a 1 dans zestatus
// on peut donc la re-appeller sans risque (customisation de BSP_SD_Init())
DSTATUS disk_initialize( BYTE pdrv )
{
if	( ( zestatus & STA_NOINIT) == 0 )
return zestatus;				// status style FatFs

__HAL_RCC_SDMMC1_CLK_ENABLE();	// remplace la partie CLK de BSP_SD_MspInit()

GPIO_config_SDCard();		// remplace la partie GPIO de BSP_SD_MspInit()

/* Check if SD card is present */
if	( !GPIO_SDCARD_present() )
	{ zestatus |= STA_NODISK; return zestatus; }	// status style FatFs

// pointeur d'instance style HAL
uSdHandle.Instance = SDMMC1;

// on remplit prematurement une structure type SDMMC_InitTypeDef,
// destinee a etre codee dans le registre CLKCR par SDMMC_Init(uSdHandle.Instance, Init);
// ce registre va etre mis a jour 3 fois :
// 	1) pour la phase d'initialisation (bus 1 bit, 400 kHz max)
//	2) pour changer de frequence d'horloge (bus 1 bit, 25 MHz max)
//	3) pour passer en bus 4 bits
uSdHandle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
uSdHandle.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;
uSdHandle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
uSdHandle.Init.BusWide             = SDMMC_BUS_WIDE_1B;
uSdHandle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
uSdHandle.Init.ClockDiv            = SDMMC_TRANSFER_CLK_DIV;

// BEGIN mise a plat de HAL_SD_Init pour eviter :
//	un dangereux appel a weak HAL_SD_MspInit()
//	un BUG : ne teste pas la valeur de retour de HAL_SD_InitCard()

uSdHandle.State = HAL_SD_STATE_BUSY; // !! ce n'est pas le state de la carte mais du driver  !!

// la fonction suivante HAL_SD_InitCard() :
//	1) cree une structure temporaire type SDMMC_InitTypeDef pour la phase d'initialisation
//	   et l'injecte dans le registre CLKCR
//	2) demarre perif SDMMC avec precaution en appelant SDMMC_PowerState_ON(uSdHandle.Instance)
//	   puis __SDMMC_ENABLE(uSdHandle.Instance) qui demarre l'horloge vers la carte
//	3) attend un delai arbitraire (2ms?)
//	4) appelle SD_PowerON(&uSdHandle)
//	   contrairement aux apparences cette fonction n'applique pas la puissance mais dialogue
//	   avec la carte SD pour acquerir certains parametres et les stocker dans uSdHandle
//		a) CMD8 pour identifier carte SDSC aka V1 vs SDHC aka V2 !! c'est sauvage :
//		   echec ==> SDSC, Ok ==> SDHC
//		b) ACMD41 (get OCR) pour signaler a la carte que host supporte SDHC et envoie 3.2:3.3V
//		   une boucle attend le bit SDHC
//		   interpretation erronnee (selon JL) du bit CCS pour detecter SDXC vs SDHC
//	5) appelle SD_InitCard(&uSdHandle), qui fait :
//		a) CMD2 acquisition de CID
//		b) CMD3 acquisition de RCA (carte passe a l'etat standby)
//		c) CMD9 acquisition de CSD, qui sera stocke en raw et en interprete dans uSdHandle
//		b) CMD7 = "selection" de la carte au sens du bus SD (carte passe a l'etat transfer)
//		b) recharge CLKCR (2eme fois) avec le contenu de la structure uSdHandle.Init
//		   essentiellement pour changer de frequence
//	6) met a jour les infos de contexte
//		uSdHandle.ErrorCode = HAL_SD_ERROR_NONE;
//		uSdHandle.Context = SD_CONTEXT_NONE;
//		uSdHandle.State = HAL_SD_STATE_READY; !! ce n'est pas le state de la carte HAL_SD_CARD_READY mais du driver  !!
// en cas d'echec, un code d'erreur peut etre retrouve dans uSdHandle.ErrorCode
if	( HAL_SD_InitCard(&uSdHandle) )
	{ zestatus |= STA_NOINIT; return zestatus; }	// status style FatFs

// END mise a plat de HAL_SD_Init

// passage en 4-bit bus :
// la fonction suivante HAL_SD_ConfigWideBusOperation() :
//	1) appelle SD_WideBus_Enable, qui fait :
//		a) appel SD_FindSCR() qui utilise ACMD51 pour recuperer SCR pour savoir
//		   si la carte supporte 4-bit bus (ridicule, toutes les SD supportent)
//		b) ACMD6 pour demander le passage en 4-bit
//		c) recharger CLKCR (3eme fois) avec une copie temporaire de la structure uSdHandle.Init
//		   dans laquelle BusWide a ete change pour SDMMC_BUS_WIDE_4B
if	( HAL_SD_ConfigWideBusOperation( &uSdHandle, SDMMC_BUS_WIDE_4B ) != HAL_OK )
	{ zestatus |= STA_NOINIT; return zestatus; }	// status style FatFs

#ifdef USE_SD_INTERRUPT
HAL_NVIC_SetPriority(SDMMC1_IRQn, 4, 0);
HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
#endif

zestatus = jSD_CheckStatus();

// Note : pour le moment le registre SSR n'est pas lu, mais la fonctions SD_SendSDStatus()
// existe dans stm32f7xx_hal_sd.c, static et inutilisee (c'est ballot)

return zestatus;
}

DSTATUS disk_status( BYTE pdrv )
{
return zestatus;
}

/*
HAL_SD_ReadBlocks
	1) appel SDMMC_CmdBlockLength() CMD16 (superflu pour SDHC)
	2) preparer une struct type SDMMC_DataInitTypeDefde 5 params pour la DPSM (Data Path State Machine)
	   l'appliquer avec SDMMC_ConfigData() qui injecte :
		- DataTimeOut dans registre DTIMER		en card bus clock periods, max 172s
		- DataLength dans DLEN				en bytes
		- DataBlockSize, TransferMode dans DCTRL	fixes pour SDHC
		- TransferDir dans DCTRL			read ou write
		- transfer enable dans DCTRL
	3) appeler SDMMC_CmdReadMultiBlock() ou SDMMC_CmdReadSingleBlock()
	   --> CMD18 ou CMD17 en passant le num. de block de depart sur la carte
	4) repeter :
		a) tester les flags RXOVERR | DCRCFAIL | DTIMEOUT | DATAEND du registre STA,
		   eventuellement sortir de la boucle
		b) si RXFIFOHF lire 8 mots de 32 bits dans le fifo (qui en contient 32)
		   Note : "Receive FIFO half full: there are at least 8 words in the FIFO"
	5) envoyer CMD12 'STOP' si on etait en multibloc
	6) collecter les erreurs eventuelles dans hsd->ErrorCode
	7) recuperer les data dans le fifo s'il en reste
OBJECTION #1 : aucun controle de quantite, i.e. si on loupe le flag DATAEND on peut deborder
la zone RAM destination !!! Par ailleurs si la carte tarde a reagir au stop, on peut supposer que le perif
va ignorer les data (ne pas les mettre dans le fifo - c'est l'hypothese optimiste
OBJECTION #2 : cette fonction gere un timeout soft en parallele avec le hard, en unites systick, soit
25000 fois plus long que celui du perif... 1200h sert a rien

HAL_SD_ReadBlocks_IT
	1) mettre adresse RAM et nombre de bytes dans la handle pour un acces global 
	2) enabler les interrupts DCRCFAIL | DTIMEOUT | RXOVERR | DATAEND | RXFIFOHF
	3) preparer une struct type SDMMC_DataInitTypeDefde 5 params pour la DPSM (Data Path State Machine)
	   l'appliquer avec SDMMC_ConfigData() (idem mode polling)
	4) appeler SDMMC_CmdBlockLength() CMD16 (superflu pour SDHC)
	5) appeler SDMMC_CmdReadMultiBlock() ou SDMMC_CmdReadSingleBlock()
	   --> CMD18 ou CMD17 en passant le num. de block de depart sur la carte
*/
DRESULT disk_read( BYTE pdrv, BYTE* buff, DWORD sector, UINT count )
{
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;
#ifdef USE_SD_INTERRUPT
if	( HAL_SD_ReadBlocks_IT( &uSdHandle, (uint8_t *)buff, (uint32_t)sector, count ) != HAL_OK )
	return RES_ERROR;
// ici on ne cherche pas a profiter du temps libre mais seulement valider le mode interrupt, alors on attend
while	( uSdHandle.State != HAL_SD_STATE_READY )	// la derniere interrupt met HAL_SD_STATE_READY (meme en cas d'erreur !?)
	{}
if	( uSdHandle.ErrorCode != 0 )
	return RES_ERROR;
#else
if	( HAL_SD_ReadBlocks( &uSdHandle, (uint8_t *)buff, (uint32_t)sector, count, SD_TIMEOUT ) != HAL_OK )
//if	( BSP_SD_ReadBlocks(             (uint32_t*)buff, (uint32_t)sector, count, SD_TIMEOUT ) != MSD_OK )
	return RES_ERROR;
#endif
/* wait until the operation is finished */
// CMD13 --> CSR --> 4 bits de state, 'tran' = repos, 'rcv' = receiving (write), 'data' = transmitting (read)
while	( HAL_SD_GetCardState(&uSdHandle) != HAL_SD_CARD_TRANSFER )
//while	( BSP_SD_GetCardState()!= MSD_OK )
	{}
return RES_OK;
}

/*
HAL_SD_WriteBlocks
	1) appel SDMMC_CmdBlockLength() CMD16 (superflu pour SDHC)
	2) appeler SDMMC_CmdWriteMultiBlock() ou SDMMC_CmdWriteSingleBlock()
	   --> CMD25 ou CMD24 en passant le num. de block de depart sur la carte
	3) preparer une struct type SDMMC_DataInitTypeDefde 5 params pour la DPSM (Data Path State Machine)
	   l'appliquer avec SDMMC_ConfigData() (voir ci-dessus)
	4) repeter :
		a) tester les flags TXUNDERR | DCRCFAIL | DTIMEOUT | DATAEND du registre STA,
		   eventuellement sortir de la boucle
		b) si TXFIFOHE ecrire 8 mots de 32 bits dans le fifo (qui en contient 32)
		   Note : "Transmit FIFO half empty: at least 8 words can be written into the FIFO"
	5) envoyer CMD12 'STOP' si on etait en multibloc
	6) collecter les erreurs eventuelles dans hsd->ErrorCode
OBJECTION #1 : a quoi sert le flag DCRCFAIL en ecriture ?
OBJECTION #2 : cf read ci-dessus

Chaque tour de boucle ecluse 8*4 = 32 bytes, @ 10MB/s on a 10/32MHz soit 312 kHz de frequence boucle,
les retards sont cumulatifs car a chaque tour on traite 8 mots meme si on pourrait faire plus
==> vulnerabilite a toute interrupt > 3.2 us
Solutions :
- Hardware flow control
- reduire horloge pour brider la carte au debit souhaite
- interrupt

HAL_SD_WriteBlocks_IT
	1) mettre adresse RAM et nombre de bytes dans la handle pour un acces global 
	2) enabler les interrupts DCRCFAIL | DTIMEOUT | TXUNDERR | DATAEND | TXFIFOHE
	3) appeler SDMMC_CmdBlockLength() CMD16 (superflu pour SDHC)
	4) appeler SDMMC_CmdWriteMultiBlock() ou SDMMC_CmdWriteSingleBlock()
	   --> CMD25 ou CMD24 en passant le num. de block de depart sur la carte
	5) preparer une struct type SDMMC_DataInitTypeDefde 5 params pour la DPSM (Data Path State Machine)
	   l'appliquer avec SDMMC_ConfigData() (idem mode polling)
*/
DRESULT disk_write( BYTE pdrv, const BYTE* buff, DWORD sector, UINT count )
{
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;
#ifdef USE_SD_INTERRUPT
if	( HAL_SD_WriteBlocks_IT(&uSdHandle, (uint8_t *)buff,(uint32_t)(sector), count ) != HAL_OK )
	return RES_ERROR;
// ici on ne cherche pas a profiter du temps libre mais seulement valider le mode interrupt, alors on attend
while	( uSdHandle.State != HAL_SD_STATE_READY )	// la derniere interrupt met HAL_SD_STATE_READY (meme en cas d'erreur !?)
	{}
if	( uSdHandle.ErrorCode != 0 )
	return RES_ERROR;
#else
if	( HAL_SD_WriteBlocks(&uSdHandle, (uint8_t *)buff,(uint32_t)(sector), count, SD_TIMEOUT) != HAL_OK)
//if	( BSP_SD_WriteBlocks(            (uint32_t*)buff,(uint32_t)(sector), count, SD_TIMEOUT) != MSD_OK )
	return RES_ERROR;
#endif
/* wait until the operation is finished */
while	( HAL_SD_GetCardState(&uSdHandle) != HAL_SD_CARD_TRANSFER )
//while	( BSP_SD_GetCardState() != MSD_OK )
	{}
return RES_OK;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff )
{
DRESULT res = RES_ERROR;
// BSP_SD_CardInfo CardInfo;
if	( zestatus & STA_NOINIT )
	return RES_NOTRDY;

switch	( cmd )
	{
	/* Make sure that no pending write process */
	case CTRL_SYNC :
		res = RES_OK;
	break;
	// 8 params de la carte sont supposes etre deja dans la structure
	// uSdHandle.SdCard type HAL_SD_CardInfoTypeDef
	//	uSdHandle.SdCard.CardType
	//	uSdHandle.SdCard.CardVersion
	//	uSdHandle.SdCard.Class
	//	uSdHandle.SdCard.RelCardAdd
	//	uSdHandle.SdCard.BlockNbr
	//	uSdHandle.SdCard.BlockSize
	//	uSdHandle.SdCard.LogBlockNbr
	//	uSdHandle.SdCard.LogBlockSize
	/* Get number of sectors on the disk (DWORD) */
	case GET_SECTOR_COUNT :
		// BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = uSdHandle.SdCard.BlockNbr;
		res = RES_OK;
	break;
	/* Get R/W sector size (WORD) */
	case GET_SECTOR_SIZE :
		// BSP_SD_GetCardInfo(&CardInfo);
		*(WORD*)buff = uSdHandle.SdCard.LogBlockSize;
		res = RES_OK;
	break;
	/* Get erase block size in unit of sector (DWORD) */
	case GET_BLOCK_SIZE :
		// BSP_SD_GetCardInfo(&CardInfo);
		*(DWORD*)buff = uSdHandle.SdCard.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
		res = RES_OK;
	break;
	default:
	res = RES_PARERR;
	}
return res;
}

/* FAT 32-bit timestamp, from MSB :
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
