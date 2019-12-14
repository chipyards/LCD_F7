#include "stm32746g_discovery.h"
#include "jrtc.h"

// ---------------------- acces RTC-------------------------

#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */ // 128
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */ // 256 * 128 = 32k, Ok

// juste pour pouvoir se passer de stm32f7xx_hal_rtc_ex.c
// finalement on en a besoin pour BKP registers alors...
// void HAL_RTCEx_AlarmBEventCallback( RTC_HandleTypeDef *hrtc )
// { UNUSED(hrtc);/* Prevent unused argument(s) compilation warning */ }

// fonction qui est weak dans le HAL et qu'on doit redefinir !
// sera appelee par HAL_RTC_Init();
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
  RCC_OscInitTypeDef        RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  /*##-1- Enables the PWR Clock and Enables access to the backup domain ###################################*/
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  
  /*##-2- Configure LSE as RTC clock source ###################################*/
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
 
  /*##-3- Enable RTC peripheral Clocks #######################################*/
  __HAL_RCC_RTC_ENABLE();
}

// mettre la RTC a l'heure a partir de notre structure a nous
void jrtc_set_day_time( DAY_TIME * d )
{
RTC_HandleTypeDef hrtc;
RTC_TimeTypeDef stimestructure;
RTC_DateTypeDef sdatestructure;
hrtc.Instance = RTC;
stimestructure.Hours   = d->hh;		// tout en binaire
stimestructure.Minutes = d->mn;		// HAL fera la conversion en BCD...
stimestructure.Seconds = d->ss;
stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;	// seulement pour daylightsave handshake
HAL_RTC_SetTime( &hrtc, &stimestructure, RTC_FORMAT_BIN );

// sdatestructure.Year    = d->yy;
sdatestructure.Month   = d->mm;
sdatestructure.Date    = d->md;
sdatestructure.WeekDay = ( d->wd + 1 ) & 7;	// chez RTC, monday = 1
HAL_RTC_SetDate( &hrtc, &sdatestructure, RTC_FORMAT_BIN );
}


void jrtc_get_day_time( DAY_TIME * d )
{
int reg;
reg = RTC->TR;		// version simplifiee de HAL_RTC_GetTime()
d->ss = RTC_Bcd2ToByte(   reg & ( RTC_TR_ST  | RTC_TR_SU  ) );
d->mn = RTC_Bcd2ToByte( ( reg & ( RTC_TR_MNT | RTC_TR_MNU ) ) >> 8  );
d->hh = RTC_Bcd2ToByte( ( reg & ( RTC_TR_HT  | RTC_TR_HU  ) ) >> 16 );
d->day_seconds = d->ss + 60 * ( d->mn + 60 * d->hh );
reg = RTC->DR;		// version simplifiee de HAL_RTC_GetDate()
d->wd = ( ( reg & RTC_DR_WDU ) >> 13 ) - 1;
// d->yy = RTC_Bcd2ToByte( ( reg & ( RTC_DR_YT | RTC_DR_YU ) ) >> 16 );
d->mm = RTC_Bcd2ToByte( ( reg & ( RTC_DR_MT | RTC_DR_MU ) ) >> 8  );
d->md = RTC_Bcd2ToByte(   reg & ( RTC_DR_DT | RTC_DR_DU ) );
}

// indique si nous avons un demarrage a froid
#define RTC_MAGIC 0xCA84
int jrtc_is_cold_poweron( void )
{
RTC_HandleTypeDef hrtc;
hrtc.Instance = RTC;
if	( HAL_RTCEx_BKUPRead( &hrtc, RTC_BKP_DR1 ) == RTC_MAGIC )
	return 0;
HAL_RTCEx_BKUPWrite( &hrtc, RTC_BKP_DR1, RTC_MAGIC );
return 1;
}

void jrtc_init( void )
{
RTC_HandleTypeDef hrtc;

// les 4 lignes suivantes font double emploi avec la fonction HAL_RTC_MspInit() qui est
// preferee car semble plus complete
//__HAL_RCC_PWR_CLK_ENABLE();
//HAL_PWR_EnableBkUpAccess();
//__HAL_RCC_RTC_CONFIG( RCC_RTCCLKSOURCE_LSE );
//__HAL_RCC_RTC_ENABLE();

hrtc.State = HAL_RTC_STATE_RESET;
hrtc.Instance = RTC;
hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
hrtc.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
hrtc.Init.SynchPrediv = RTC_SYNCH_PREDIV;
hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;		// les 2 suivants sont inutiles mais il ne faut pas les laisser indefinis
hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

// N.B. HAL_RTC_Init() appelle notre HAL_RTC_MspInit() definie ci-dessus !
HAL_RTC_Init( &hrtc );
}
