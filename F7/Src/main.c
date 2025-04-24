
/* Includes ------------------------------------------------------------------*/
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_ts.h"
#include "system_misc.h"
#include "options.h"
#include "jlcd.h"
#include "jrtc.h"
#include "idrag.h"
#include "demo.h"
#include "trans.h"
#include "menu.h"
#include "adju.h"
#include "s_gpio.h"
#include "stm32f7xx_ll_usart.h"
#include "uarts.h"
#include "logfifo.h"
#include "param.h"
#include <stdio.h>
#include <stdlib.h>

void USB_PhyEnterLowPowerMode(void);
void ETH_PhyEnterPowerDownMode(void);

// #define GREEN_CPU	// deporte dans options.h
// #define PROFILER_PI2	// pin PI2 aka D8 deporte dans options.h

// ---------------------- contexte global ------------------

// flags d'affichage pour show_flags
#define DEMO_FLAG	1	// zone de scroll
#define TRANS_FLAG	2
#define MENU_FLAG	4
#define PARAM_FLAG	8

#define BURG_FLAG	0x10	// zone fixe

#define LOCPIX_FLAG	0x10000

// N.B. cette appli utilise :
//	- l'interruption du LTDC (vertical blank) pour cadener la boucle principale
//	- la RTC, pour les operations cadencees a la seconde
// Elle ne depend pas de systick, qu'on pourrait deasactiver

int show_flags;			// flags d'affichage
int touch_occur_cnt = 0;	// anti-rebond pour single touch
int old_touch_cnt = 0;		// detection retour de double touch
int unscroll_timout = 10;	// tempo pour unscrolling auto, en s.
DAY_TIME daytime;		// le temps lu dans la rtc, utilise pour les tempos "a la seconde"

#define SHORT_TOUCH_DELAY	3	// en frames de 16ms
#define MED_TOUCH_DELAY		80	// en frames
#define LONG_TOUCH_DELAY	200	// en frames

int kmenu = 0;

// quelques parametres geometriques de GUI (ecran 480*272)
// Axe X
#ifdef LEFT_FIX
#define LEFT_BURG	// justification du burger dans la zone fix
#define FIX_ZONE_X0 	0
#define FIX_ZONE_DX	60	// 180
#define SCROLL_ZONE_X0	FIX_ZONE_DX
#define SCROLL_ZONE_DX	(LCD_DX-FIX_ZONE_DX)
#else
#define SCROLL_ZONE_X0	0
#define SCROLL_ZONE_DX	300
#define FIX_ZONE_X0 	SCROLL_ZONE_DX
#define FIX_ZONE_DX	(LCD_DX-SCROLL_ZONE_DX)
#endif

#define MBURG 8		// marge burger
#define WBURG 50	// taille burger (w ou h) pour clic

#ifdef FLASH_THE_FONTS
int flash_bytes = 0;
int flash_errs = 0;
#endif

// ----------------------  Interrupts ----------------------

void SysTick_Handler(void)
{
  HAL_IncTick();
}

#ifdef USE_UART6
/* RX : necessite USE_LOGFIFO
	- les messages recus sur UART6 (termines par \n) sont envoyes a LOGFIFO (sans \n) pour affichage,
	  et dupliques sur CDC (avec \n ajoute)
	- les messages de plus de trans.qcharvis chars sont coupes proprement
	  (ils sont continues sur une ligne supplementaire ou pluieurs)
   TX : si ( tx6index == -1 ), remplir le buffer sans deborder et enable TX interrupt
   (demo : message periodiques de timestamp)
   NOTE sur thread-safety :
   	USART6_IRQHandler appelle LOGline(), qui peut appeler UART1_TX_INT_enable(),
   	alors si UART1 est prioritaire le handler de UART6 va etre interrompu (sans de danger evident)
*/
char tx6buf[64];
volatile int tx6index = -1;	// -1 <==> buffer available, fill it, zero terminated, then enable TX interrupt
char rx6buf[64];	// buffer size doit etre > trans.qcharvis (actuellement 42)
volatile int rx6index = 0;

void USART6_IRQHandler( void )
{
if	(
	( LL_USART_IsActiveFlag_TXE( USART6 ) ) &&
	( LL_USART_IsEnabledIT_TXE( USART6 ) )
	)
	{
	char c = tx6buf[tx6index++];
	if	( ( c ) && ( tx6index <= sizeof(tx6buf) ) )
		{
		LL_USART_TransmitData8( USART6, c );
		}
	else	{
		UART6_TX_INT_disable();
		tx6index = -1;		// unlock
		}
	}
if	(
	( LL_USART_IsActiveFlag_RXNE( USART6 ) ) &&
	( LL_USART_IsEnabledIT_RXNE( USART6 ) )
	)
	{
	char c = LL_USART_ReceiveData8( USART6 );
	if	( rx6index >= trans.qcharvis )    // ligne pleine
		{
		rx6buf[rx6index] = 0;   // usual terminator for a C string
		LOGline( rx6buf );
		rx6index = 0;
		if	( c > ' ' )
			rx6buf[rx6index++] = c;	// save c for next log line !
		}
	else if	( c == 10 )		// line terminator
          	{
		rx6buf[rx6index] = 0;   // usual terminator for a C string
		if	( rx6index )
			LOGline( rx6buf );
		rx6index = 0;		// get ready for next message
		}
	else	{
        	rx6buf[rx6index++] = c;
		}
	}
}
#endif
// ---------------------- application ----------------------

void report_interrupts(void)
{
#ifdef USE_CDC_PRINT
unsigned int i, p;
p = __NVIC_GetPriorityGrouping();
while (CDC_print("priority grouping %d\n", p )) {};
// special systick
i = -1;
if	(  SysTick->CTRL & SysTick_CTRL_TICKINT_Msk )
	{
	p = __NVIC_GetPriority(i);
	while (CDC_print("int #%2d, pri %d\n", i, p )) {};
	}
// tous les autres
for	( i = 0; i <=  97; ++i )
	{
	if	( __NVIC_GetEnableIRQ(i) )
		{
		p = __NVIC_GetPriority(i);
		while (CDC_print("int #%2d, pri %d\n", i, p )) {};
		}
	}
#else
#ifdef USE_LOGFIFO
unsigned int i, p;
p = __NVIC_GetPriorityGrouping();
LOGprint("priority grouping %d", p );
// special systick
i = -1;
if	(  SysTick->CTRL & SysTick_CTRL_TICKINT_Msk )
	{
	p = __NVIC_GetPriority(i);
	LOGprint("int #%2d, pri %d", i, p );
	}
// tous les autres
for	( i = 0; i <=  97; ++i )
	{
	if	( __NVIC_GetEnableIRQ(i) )
		{
		p = __NVIC_GetPriority(i);
		LOGprint("int #%2d, pri %d", i, p );
		}
	}
#endif
#endif
}


// trace reticule
void draw_reticle( int x, int y )
{
GC.line_color = ARGB_RED;
jlcd_hline( 0, y, LCD_DX );
jlcd_vline( x, 0, LCD_DY );
}

// mise a jour de idrag.yobjmin et position par defaut
void unscroll(void)
{
idrag.yobj = 0;	// top de la page est visible (presque tous les cas)
if	( show_flags & MENU_FLAG )
	idrag.yobjmin = - menu.ty;
else
#ifdef USE_TRANSCRIPT
if	( show_flags & TRANS_FLAG )
	{
	idrag.yobjmin = LCD_DY - trans.dy;
	idrag.yobj = idrag.yobjmin;	// bas de la page, exception !
	}
else	
#endif
#ifdef USE_PARAM
if	( show_flags & PARAM_FLAG )
	idrag.yobjmin = LCD_DY - para.dy;
else		
#endif
#ifdef USE_DEMO
if	( show_flags & DEMO_FLAG )
	idrag.yobjmin = LCD_DY - DEMO_DY;
#endif
;}

// initialise les zones en fonction des options valides
// et d'un flag concernant la zone de scroll
void init_scroll_zones( int flag )
{
switch	( flag )
	{
	case MENU_FLAG :
		show_flags |= MENU_FLAG;
		break;
	case TRANS_FLAG :
		show_flags = TRANS_FLAG | BURG_FLAG;
		break;
	case PARAM_FLAG :
		show_flags = PARAM_FLAG | BURG_FLAG;
		break;
	case DEMO_FLAG :
		show_flags = DEMO_FLAG | BURG_FLAG;
		break;
	default:
		show_flags = BURG_FLAG;
	}
unscroll();
}

// initialise par defaut en fonction des options
// le dernier element est affiche par defaut
void init_zones_default(void)
{
init_scroll_zones( 0 );
#ifdef USE_DEMO
init_scroll_zones( DEMO_FLAG );
#endif
#ifdef USE_PARAM
init_scroll_zones( PARAM_FLAG );
#endif
#ifdef USE_TRANSCRIPT
init_scroll_zones( TRANS_FLAG );
#endif
}

void create_menu( void )
{
menu_init( &JFont20, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
#ifdef USE_TRANSCRIPT
menu_add( TRANS_FLAG, "TRANSCRIPT" );
#endif
#ifdef USE_DEMO
menu_add( DEMO_FLAG, "FONTS DEMO" );
#endif
#ifdef USE_PARAM
menu_add( PARAM_FLAG, "PARAMETRES" );
#endif
menu_add( LOCPIX_FLAG, "LOCPIX" );
}

// toutes les operations de trace
void repaint( TS_StateTypeDef * touch )
{
int x, y;
char tbuf[32];
__HAL_RCC_DMA2D_CLK_ENABLE();
// zone FIX no-scroll ( burger, logo, heure et date ) ===========================
if	( show_flags & ( BURG_FLAG | LOCPIX_FLAG ) )
	{	// preparer affichage
	GC.fill_color = ARGB_WHITE;
	jlcd_rect_fill( FIX_ZONE_X0 + 1, 0, FIX_ZONE_DX - 1, LCD_DY );
	}
if	( show_flags & BURG_FLAG )
	{
	GC.vfont = &JVFont36n;
	//snprintf( tbuf, sizeof(tbuf), "<" );		// logo, centre
	//int xc = FIX_ZONE_X0 + ( FIX_ZONE_DX / 2 );	// le centre
	//int w = jlcd_vtext_dx( tbuf );
	//x = xc - ( w / 2 );
	//y = YLOGO;
	//jlcd_vtext( x, y, tbuf );
	snprintf( tbuf, sizeof(tbuf), "=" );		// burger, cale au bord
	#ifdef LEFT_BURG
	x = FIX_ZONE_X0 + MBURG;
	#else
	int w = jlcd_vtext_dx( tbuf );
	x = FIX_ZONE_X0 + FIX_ZONE_DX - w - MBURG;
	#endif
	y = MBURG;
	jlcd_vtext( x, y, tbuf );
	}
// zone SCROLL ==================================================================
if	( show_flags & MENU_FLAG )	// le menu scrollatif (prempte les autres)
	kmenu = menu_draw( idrag.yobj );
else
#ifdef USE_DEMO
if	( show_flags & DEMO_FLAG )	// page de demo scrollable
	demo_draw( idrag.yobj, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
else
#endif
#ifdef USE_TRANSCRIPT
if	( show_flags & TRANS_FLAG )	// page de transcript scrollable
	transdraw( idrag.yobj );
else
#endif
#ifdef USE_PARAM
if	( show_flags & PARAM_FLAG )	// page de parametres scrollable
	param_draw( idrag.yobj );
else
#endif
	{				// remplissage par defaut
	GC.fill_color = ARGB_LIGHTGRAY;
	jlcd_rect_fill( SCROLL_ZONE_X0, 0, SCROLL_ZONE_DX, LCD_DY );
	snprintf( tbuf, sizeof(tbuf), "V%s", VERSION );
	x = SCROLL_ZONE_X0 + 20;
	y = 20;
	GC.font = &JFont24; GC.text_color = ARGB_BLACK;
	jlcd_text( x, y, tbuf );
	}
// Auxiliaires (zone FIX) =======================================================
tbuf[0] = 0;
if	( show_flags & LOCPIX_FLAG )	// assistant localisation pixels
	{
	snprintf( tbuf, sizeof(tbuf), "%3d %3d", touch->touchX[0], touch->touchY[0] );
	draw_reticle( touch->touchX[0], touch->touchY[0] );
	}
else if	( show_flags & MENU_FLAG )
	{
	snprintf( tbuf, sizeof(tbuf), "%d", kmenu );
	}
else	{
	#ifdef COMPILE_THE_FONTS
	int dummy;
	dummy = (int)JFont24_Table + (int)JFont20_Table +
		(int)JFont16_Table + (int)JFont12_Table + (int)JFont8_Table; 
	snprintf( tbuf, sizeof(tbuf), "%u", dummy );
	#ifdef FLASH_THE_FONTS
	snprintf( tbuf, sizeof(tbuf), "%d %d", flash_bytes, flash_errs );
	#endif
	#else
	// snprintf( tbuf, sizeof(tbuf), "%d", idrag.yobj );
	#endif
	}
if	( tbuf[0] )			// blue text sous le hamburger
	{
	x = FIX_ZONE_X0 + 12;
	y = WBURG - 20;
	GC.font = &JFont20; GC.text_color = ARGB_BLUE;
	jlcd_text( x, y, tbuf );
	}

__HAL_RCC_DMA2D_CLK_DISABLE();
swap_layer();
jlcd_reload_shadows();	// mais il faudra attentre bloquante le prochain vblank !!!
}

// interpreter un simple clic dans la zone FIX
void clic_fix_event_call( int x, int y )
{
if	(
	#ifdef LEFT_BURG
	( x < ( FIX_ZONE_X0 + MBURG + WBURG ) )
	#else
	( x > ( FIX_ZONE_X0 + FIX_ZONE_DX - MBURG - WBURG ) )
	#endif
	&& ( y < ( MBURG + WBURG ) )
	)
	{
	if	(!( show_flags & MENU_FLAG ))
		{			// entrer dans le menu
		init_scroll_zones( MENU_FLAG );	// NB les autres flags ne sont pas effaces
		}
	else	{			// interpreter choix menu et sortie
		show_flags &= ~MENU_FLAG;
		switch	( kmenu )
			{
			case DEMO_FLAG:
			case TRANS_FLAG:
			case PARAM_FLAG:
				init_scroll_zones( kmenu ); break;
			case LOCPIX_FLAG:
				show_flags ^= LOCPIX_FLAG; break;
			}
		unscroll();
		}			// entrer
	}
else	{
	#ifdef FLASH_THE_FONTS
	flash_errs = check_the_fonts();
	#endif
	}
}

// interpreter un long clic dans la zone FIX
void long_clic_fix_event_call( int x, int y )
{
// if	( ( y > ... ) && ( y < ... ) ) )
	{
	#ifdef FLASH_THE_FONTS
	flash_bytes = flash_the_fonts();
	#else
	jlcd_panel_off();
	#endif
	}
}

#ifdef USE_PARAM
// interpreteur d'edition de parametre
void param_handler( int item, int val )
{
switch	( item )
	{
	case 0 :
		unscroll_timout = val;
		LOGprint("unscroll_timout %d", unscroll_timout ); break;
	}
}
#endif

// interpreteur de commandes sur 1 char
static void cmd_handler( int c )
{
switch	( c )
	{
	case '0' :
		LOGprint("0123456789\n012345\n67890123456789012345678901");
		break;
	default :
		LOGprint("cmd '%c'", c );
	}
}

int main(void)
{
TS_StateTypeDef TS_State;
int paint_flag, old_second=0, last_touch_second=0;
unsigned int old_ltdc_irq_cnt = 0;

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();

// juste pour eteindre le panel
jlcd_gpio1();

jrtc_init();
if	( jrtc_is_cold_poweron() )	// N.B. ce test ne marche qu'une fois
	{
	daytime.md = 27;
	daytime.mm = 5;
	daytime.wd = 1;
	daytime.hh = 9;
	daytime.mn = 33;
	daytime.ss = 44;
	jrtc_set_day_time( &daytime );
	}

// sequence d'init rapide :
SystemClock200_Config();
#ifndef FLASH_THE_FONTS
USB_PhyEnterLowPowerMode();
ETH_PhyEnterPowerDownMode();
__HAL_RCC_USB_OTG_HS_CLK_DISABLE();
__HAL_RCC_USB_OTG_HS_ULPI_CLK_DISABLE();
__HAL_RCC_ETH_CLK_DISABLE();
#endif
jlcd_sdram_init();
jlcd_gpio2();
jlcd_init();
// __HAL_RCC_DMA2D_CLK_ENABLE();
GC_init();
#ifdef PROFILER_PI2
GPIO_config_profiler_PI1_PI2();
#endif
BSP_TS_Init( LCD_DX, LCD_DY );

// BSP_LED_Init(LED1);
BSP_PB_Init( BUTTON_KEY, BUTTON_MODE_GPIO );   

#ifdef USE_UART1
CDC_init();
#endif
#ifdef USE_UART6
UART6_init(38400);
GPIO_config_uart6();
#endif

idrag_init();
idrag.yobjmax = 0;
idrag.yobjmin = -LCD_DY;

create_menu();

#ifdef USE_TRANSCRIPT
transcript_init( &JFont16n, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
report_interrupts();
#else
#ifdef USE_LOGFIFO
logfifo_init();
#endif
#endif

#ifdef USE_PARAM
param_init( &JFont20, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
#endif

init_zones_default();	// doit etre APRES create_menu

jlcd_interrupt_on();
jlcd_panel_on();

while	(1)
	{
	paint_flag = 0;
	// traiter le touch
	BSP_TS_GetState( &TS_State );
	if	( TS_State.touchDetected == 1 )
		{ 					// single touch
		if	( !jlcd_is_panel_on() )
			{				// reveil du panel
			if	( touch_occur_cnt == 0 )
				{
				jlcd_panel_on();
				unscroll();
				}
			}
		else	{
			++touch_occur_cnt;
			#ifdef USE_PARAM
			if	( ( show_flags & PARAM_FLAG ) && ( para.editing ) )
				idrag_event_call( 0, 0, 0, GC.ltdc_irq_cnt );	// laisser courir
			else
			#endif
			if	(
				( TS_State.touchX[0] > FIX_ZONE_X0 ) &&
				( TS_State.touchX[0] < ( FIX_ZONE_X0 + FIX_ZONE_DX ) )
				)
				{					// zone FIX =============
				idrag_event_call( 0, 0, 0, GC.ltdc_irq_cnt );	// laisser courir
				if	( touch_occur_cnt == SHORT_TOUCH_DELAY )
					clic_fix_event_call( TS_State.touchX[0], TS_State.touchY[0] );
				else if	( touch_occur_cnt == LONG_TOUCH_DELAY )
					long_clic_fix_event_call( TS_State.touchX[0], TS_State.touchY[0] );
				}
			else	{					// zone SCROLL ==========
				idrag_event_call( TS_State.touchDetected,	// scroll normal
						TS_State.touchX[0], TS_State.touchY[0],
						GC.ltdc_irq_cnt );
				#ifdef USE_PARAM
				if	( show_flags & PARAM_FLAG )
					{
					if	( touch_occur_cnt == SHORT_TOUCH_DELAY )
						param_select( TS_State.touchY[0] );
					if	( ( touch_occur_cnt == MED_TOUCH_DELAY ) && ( para.editing == 0 ) )
						{
						idrag.yobj = param_start();	// demarrer l'edition sur le param qui a ete deja selectionne
						idrag.yobjmin = - adj.ty;	// a faire apres adju_start()
						idrag.touching = 0;		// forcer un nouveau landing
						}				// N.B. param_start() a mis para.editing a 1
					}
				#endif
				}
			}
		paint_flag = 1; last_touch_second = daytime.day_seconds;
		old_touch_cnt = 1;
		}
	else if	( TS_State.touchDetected == 2 )
		{	// double touch : 2 fingers
		#ifdef USE_PARAM
		if	( ( old_touch_cnt == 1 ) && ( show_flags & PARAM_FLAG ) && ( para.editing == 0 ) )
			{
			idrag.yobj = param_start();	// demarrer l'edition sur le param qui a ete deja selectionne
			idrag.yobjmin = - adj.ty;	// a faire apres adju_start()
			idrag.touching = 0;		// forcer un nouveau landing
			}				// N.B. param_start() a mis para.editing a 1
		#endif
		int x2 = TS_State.touchX[1];
		int y2 = TS_State.touchY[1];
		idrag_event_call( TS_State.touchDetected, x2, y2, GC.ltdc_irq_cnt );
		paint_flag = 1; last_touch_second = daytime.day_seconds;
		old_touch_cnt = 2;
		}
	else	{						// zero touch
		idrag_event_call( 0, 0, 0, GC.ltdc_irq_cnt );
		if	( old_touch_cnt )
			{
			#ifdef USE_PARAM
			if	( show_flags & PARAM_FLAG )
				{
				if	( para.editing )
					{
					param_save();	// enregistrer la valeur et quitter le mode adj
					unscroll();
					}
				paint_flag = 1;
				}
			#endif
			}
		touch_occur_cnt = 0;
		old_touch_cnt = 0;
		} // if	TS_State.touchDetected 1, 2 ou 0
	jrtc_get_day_time( &daytime );
	if	( old_second != daytime.day_seconds )
		{	// traitement cadence a la seconde
		#ifdef USE_UART6	// emission de message sur UART6 Tx pour tester UART6 Rx avec un bouclage
		snprintf( tx6buf, sizeof(tx6buf), "-> %02d:%02d:%02d\n", daytime.hh, daytime.mn, daytime.ss );
		tx6index = 0;
		UART6_TX_INT_enable();
		#else
		#ifdef USE_LOGFIFO
		LOGprint("-> %02d:%02d:%02d", daytime.hh, daytime.mn, daytime.ss );
		#endif
		#endif
		paint_flag = 1;
		old_second = daytime.day_seconds;
		if	( ( unscroll_timout ) && ( daytime.day_seconds > ( last_touch_second + unscroll_timout ) ) )
			unscroll();
		}
	
	if	( idrag.drifting )
		paint_flag = 1;
	if	( !jlcd_is_panel_on() )
		paint_flag = 0;

	if	( paint_flag )
		{
		repaint( &TS_State );
		LTDC->SRCR |= LTDC_SRCR_VBR;
		}

	#ifdef USE_PARAM
	{
	int item = param_scan();
	if	( item >= 0 )
		param_handler( item, param_get_val( item ) );
	}
	#endif

	#ifdef USE_UART1
	{
	int c = CDC_getcmd();
	if	( c > 0 )
		cmd_handler( c );
	}
	#endif

	#ifdef PROFILER_PI2
	profile_D8(0);	// PI2 aka D8 profiler pin
	#endif

	#ifdef GREEN_CPU
	while	( GC.ltdc_irq_cnt == old_ltdc_irq_cnt )
		{
		#ifdef PROFILER_PI2
		// profile_D13(0);	// PI1
		#endif
		HAL_PWR_EnterSLEEPMode( PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI );
		#ifdef PROFILER_PI2
		// profile_D13(1);	// PI1
		#endif
		}
	old_ltdc_irq_cnt = GC.ltdc_irq_cnt;
	#else
	while	( GC.ltdc_irq_cnt == old_ltdc_irq_cnt )
		{}
	old_ltdc_irq_cnt = GC.ltdc_irq_cnt;
	#endif

	#ifdef PROFILER_PI2
	profile_D8(1);
	#endif
	}
}

