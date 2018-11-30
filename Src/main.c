
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
#include <stdlib.h>

void USB_PhyEnterLowPowerMode(void);
void ETH_PhyEnterPowerDownMode(void);

// #define GREEN_CPU	// deporte dans options.h
// #define PROFILER_PI2	// pin PI2 aka D8 deporte dans options.h

// ---------------------- contexte global ------------------

// flags d'affichage
#define DEMO_FLAG	1	// zone de scroll
#define TRANS_FLAG	2
#define MENU_FLAG	4

#define LOGO_FLAG	0x10	// zone fixe
#define DATE_FLAG	0x20
#define HOUR_FLAG	0x40

#define MN_ADJ_FLAG	0x100	// ajustements
#define HH_ADJ_FLAG	0x200
#define WD_ADJ_FLAG	0x1000
#define MD_ADJ_FLAG	0x2000
#define MM_ADJ_FLAG	0x4000
#define TIME_ADJ_FLAGS ( HH_ADJ_FLAG | MN_ADJ_FLAG | WD_ADJ_FLAG | MD_ADJ_FLAG | MM_ADJ_FLAG )

#define LOCPIX_FLAG	0x10000


int show_flags;			// flags d'affichage
int shown_day = 0;		// jour de semaine  0 à 4
int touch_occur_cnt = 0;	// anti-rebond pour single touch
int old_touch_cnt = 0;		// detection retour de double touch

DAY_TIME daytime;	// le temps sous diverses formes

#define UNSCROLL_TIMOUT 	10	// en s
#define SHORT_TOUCH_DELAY	3	// en frames
#define LONG_TOUCH_DELAY	120	// en frames

int kmenu = 0;

// quelques parametres geometriques de GUI (ecran 480*272)
// Axe X
#ifdef LEFT_FIX
#define LEFT_BURG	// justification du burger dans la zone fix
#define FIX_ZONE_X0 	0
#define FIX_ZONE_DX	180
#define SCROLL_ZONE_X0	FIX_ZONE_DX
#define SCROLL_ZONE_DX	(LCD_DX-FIX_ZONE_DX)
#else
#define SCROLL_ZONE_X0	0
#define SCROLL_ZONE_DX	300
#define FIX_ZONE_X0 	SCROLL_ZONE_DX
#define FIX_ZONE_DX	(LCD_DX-SCROLL_ZONE_DX)
#endif
// Axe Y
#define YLOGO 100	// sommet

#define YDATE 180	// sommet des lettres
#define YHOUR 220

#define MBURG 12	// marge burger
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

// ---------------------- application ----------------------

// trace reticule
void draw_reticle( int x, int y )
{
GC.line_color = ARGB_RED;
jlcd_hline( 0, y, LCD_DX );
jlcd_vline( x, 0, LCD_DY );
}

void unscroll(void)
{
idrag.yobj = 0;
#ifdef USE_TRANSCRIPT
if	(
	( show_flags & TRANS_FLAG ) &&
	( ( show_flags & ( MENU_FLAG | TIME_ADJ_FLAGS ) ) == 0 )
	)
	{
	idrag.yobjmin = LCD_DY - trans.dy;
	idrag.yobj = idrag.yobjmin;
	}
#endif
#ifdef USE_DEMO
if	( show_flags & DEMO_FLAG )
	idrag.yobj = 0;
#endif
}

// initialise les zones en fonction des options valides
// et d'un flag concernant la zone de scroll
void init_scroll_zones( int flag )
{
switch	( flag )
	{
	case MENU_FLAG :
		idrag.yobjmin = - menu.ty;
		show_flags |= MENU_FLAG;
		break;
	#ifdef USE_TRANSCRIPT
	case TRANS_FLAG :
		idrag.yobjmin = LCD_DY - transcript_init( &JFont16n, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
		show_flags = TRANS_FLAG | LOGO_FLAG;
		#ifdef USE_TIME_DATE
		show_flags |= ( DATE_FLAG | HOUR_FLAG );
		#endif
		break;
	#endif
	#ifdef USE_DEMO
	case DEMO_FLAG :
		idrag.yobjmin = LCD_DY - DEMO_DY;
		show_flags = DEMO_FLAG | LOGO_FLAG;
		#ifdef USE_TIME_DATE
		show_flags |= ( DATE_FLAG | HOUR_FLAG );
		#endif
		break;
	#endif
	default:
		show_flags = LOGO_FLAG;
		#ifdef USE_TIME_DATE
		show_flags |= ( DATE_FLAG | HOUR_FLAG );
		#endif
	}
unscroll();
}

// initialise par defaut en fonction des options
// le dernier element est prioritaire
void init_zones_default(void)
{
init_scroll_zones( 0 );
#ifdef USE_DEMO
init_scroll_zones( DEMO_FLAG );
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
menu_add( LOCPIX_FLAG, "LOCPIX" );
}

// toutes les operations de trace
void repaint( TS_StateTypeDef * touch )
{
int xc, x, y, w;
char tbuf[32];
__HAL_RCC_DMA2D_CLK_ENABLE();
// zone FIX ( burger, logo, heure et date ) =====================================
if	( show_flags & ( LOGO_FLAG | DATE_FLAG | HOUR_FLAG | LOCPIX_FLAG ) )
	{	// preparer affichage d'elements centres zone FIX (no scroll)
	xc = FIX_ZONE_X0 + ( FIX_ZONE_DX / 2 );	// le centre
	GC.fill_color = ARGB_WHITE;
	jlcd_rect_fill( FIX_ZONE_X0 + 1, 0, FIX_ZONE_DX - 1, LCD_DY );
	}
if	( show_flags & LOGO_FLAG )
	{
	snprintf( tbuf, sizeof(tbuf), "<" );		// logo, centre
	GC.vfont = &JVFont36n;
	w = jlcd_vtext_dx( tbuf );
	x = xc - ( w / 2 );
	y = YLOGO;
	jlcd_vtext( x, y, tbuf );
	snprintf( tbuf, sizeof(tbuf), "=" );		// burger, cale au bord
	w = jlcd_vtext_dx( tbuf );
	#ifdef LEFT_BURG
	x = FIX_ZONE_X0 + MBURG;
	#else
	x = FIX_ZONE_X0 + FIX_ZONE_DX - w - MBURG;
	#endif
	y = MBURG;
	jlcd_vtext( x, y, tbuf );
	}
#ifdef USE_TIME_DATE
if	( show_flags & DATE_FLAG )
	{			// date
	y = YDATE;
	tbuf[0] = ';' + daytime.wd;
	snprintf( tbuf+1, sizeof(tbuf), "  %02d:%02d", daytime.md, daytime.mm );
	GC.vfont = &JVFont26s;
	w = jlcd_vtext_dx( tbuf );
	x = xc - ( w / 2 );
	jlcd_vtext( x, y, tbuf );
	}
if	( show_flags & HOUR_FLAG )
	{			// heure
	y = YHOUR; 
	snprintf( tbuf, sizeof(tbuf), "%02d:%02d", daytime.hh, daytime.mn );
	// calcul largeur pour centrage
	GC.vfont = &JVFont36n;
	w = jlcd_vtext_dx( tbuf ) + GC.vfont->sx;
	GC.vfont = &JVFont19n;
	w += jlcd_vtext_dx( ":00" );
	// affichage hh:mn:ss
	GC.vfont = &JVFont36n;
	x = xc - ( w / 2 );
	x = jlcd_vtext( x, y, tbuf );
	y += (36 - 19);	// compenser difference de hauteur
	snprintf( tbuf, sizeof(tbuf), ":%02d", daytime.ss );
	GC.vfont = &JVFont19n;
	jlcd_vtext( x, y, tbuf );
	}			// overlay d'ajustement
if	( show_flags & TIME_ADJ_FLAGS )
	adju_draw( idrag.yobj );
#endif
// zone SCROLL ==================================================================
if	( show_flags & MENU_FLAG )	// le menu scrollatif (prempte les autres)
	kmenu = menu_draw( idrag.yobj ); 
else
#ifdef USE_DEMO
if	( show_flags & DEMO_FLAG )	// page de demo scrollable a gauche
	demo_draw( idrag.yobj, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
else
#endif
#ifdef USE_TRANSCRIPT
if	( show_flags & TRANS_FLAG )	// page de transcript scrollable a gauche
	transdraw( idrag.yobj );
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
	#ifdef FLASH_THE_FONTS
	snprintf( tbuf, sizeof(tbuf), "%d %d", flash_bytes, flash_errs );
	#else
	// snprintf( tbuf, sizeof(tbuf), "%d", idrag.vy );
	#endif
	}
if	( tbuf[0] )			// status/debug PROVIZOAR pas clean	
	{
	x = FIX_ZONE_X0 + 12;
	y = YLOGO - 40;
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
				init_scroll_zones( DEMO_FLAG ); break;
			case TRANS_FLAG:
				init_scroll_zones( TRANS_FLAG ); break;
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
if	( ( y > YLOGO ) && ( y < ( YDATE - 20 ) ) )	// zone logo
	{
	#ifdef FLASH_THE_FONTS
	flash_bytes = flash_the_fonts();
	#else
	jlcd_panel_off();
	#endif
	}
}

int main(void)
{
TS_StateTypeDef TS_State;
int paint_flag, old_second, last_touch_second;
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

idrag_init();
idrag.yobjmax = 0;
idrag.yobjmin = -LCD_DY;

create_menu();
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
			if	( show_flags & TIME_ADJ_FLAGS )
				idrag_event_call( 0, 0, 0, GC.ltdc_irq_cnt );	// laisser courir
			else if	(
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
				}
			}
		paint_flag = 1; last_touch_second = daytime.day_seconds;
		old_touch_cnt = 1;
		}
	else if	( TS_State.touchDetected == 2 )
		{	// double touch :
		int x2 = TS_State.touchX[1];
		int y2 = TS_State.touchY[1];
		#ifdef USE_TIME_DATE
		if	(				// gerer l'entree dans un ajustement horaire
			( old_touch_cnt == 1 ) &&	// one_shot
			( ( show_flags & TIME_ADJ_FLAGS ) == 0 ) &&
			( x2 > FIX_ZONE_X0 ) &&
			( x2 < ( FIX_ZONE_X0 + FIX_ZONE_DX ) )
			)
			{
			if	( y2 > ( YHOUR - 10 ) )
				{
				if	( x2 < ( FIX_ZONE_X0 + 70 ) )
					{
					show_flags |= ( HH_ADJ_FLAG | HOUR_FLAG );
					adju_start( &JFont20, FIX_ZONE_X0+20, YHOUR-7, 44, 50, 0, 24 );
					idrag.yobjmin = - adj.ty;	// a faire apres adju_start()
					}
				else if	( x2 < ( FIX_ZONE_X0 + 130 ) )
					{
					show_flags |= ( MN_ADJ_FLAG | HOUR_FLAG );
					adju_start( &JFont20, FIX_ZONE_X0+79, YHOUR-7, 44, 50, 0, 60 );
					idrag.yobjmin = - adj.ty;
					}
				// else	rien
				}
			else if	( y2 > ( YDATE - 10 ) )
				{
				if	( x2 < ( FIX_ZONE_X0 + 90 ) )
					{
					show_flags |= WD_ADJ_FLAG | DATE_FLAG;
					adju_start( &JFont20, FIX_ZONE_X0+8, YDATE-9, 85, 50, 0, 5 );
					idrag.yobjmin = - adj.ty;
					}
				else if	( x2 < ( FIX_ZONE_X0 + 130 ) )
					{
					show_flags |= MD_ADJ_FLAG | DATE_FLAG;
					adju_start( &JFont20, FIX_ZONE_X0+90, YDATE-9, 38, 50, 1, 32 );
					idrag.yobjmin = - adj.ty;
					}
				else	{
					show_flags |= MM_ADJ_FLAG | DATE_FLAG;
					adju_start( &JFont20, FIX_ZONE_X0+135, YDATE-9, 38, 50, 1, 13 );
					idrag.yobjmin = - adj.ty;
					}
				}
			}
		else	{
			idrag_event_call( TS_State.touchDetected,
					  TS_State.touchX[1], TS_State.touchY[1],
					  GC.ltdc_irq_cnt );
			}
		#endif
		paint_flag = 1; last_touch_second = daytime.day_seconds;
		old_touch_cnt = 2;
		}
	else	{						// zero touch
		idrag_event_call( 0, 0, 0, GC.ltdc_irq_cnt );
		if	( old_touch_cnt )
			{
			#ifdef USE_TIME_DATE
			if	( show_flags & TIME_ADJ_FLAGS )
				{			// sauver resultat ajustement
				switch	( show_flags & TIME_ADJ_FLAGS )
					{
					case HH_ADJ_FLAG: daytime.hh = adj.val; break;
					case MN_ADJ_FLAG: daytime.mn = adj.val; break;
					case WD_ADJ_FLAG: daytime.wd = adj.val; break;
					case MD_ADJ_FLAG: daytime.md = adj.val; break;
					case MM_ADJ_FLAG: daytime.mm = adj.val; break;
					}
				jrtc_set_day_time( &daytime );

				show_flags &= ~TIME_ADJ_FLAGS;		// quitter mode ajust
				unscroll();
				paint_flag = 1;
				}
			#endif
			}
		touch_occur_cnt = 0;
		old_touch_cnt = 0;
		} // if	TS_State.touchDetected 1, 2 ou 0
	jrtc_get_day_time( &daytime );
	if	( old_second != daytime.day_seconds )
		{					// traitement cadence a la seconde
		#ifdef USE_TRANSCRIPT
		transprint("-> %02d:%02d:%02d", daytime.hh, daytime.mn, daytime.ss );
		#endif
		#ifdef USE_UART1
		CDC_print("-> %02d show %08x\r\n", daytime.ss, show_flags );
		#endif
		paint_flag = 1;
		old_second = daytime.day_seconds;
		if	( daytime.day_seconds > ( last_touch_second + UNSCROLL_TIMOUT ) )
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

	#ifdef USE_UART1
	{
	int c = CDC_getcmd();
	if	( c > 0 )
		CDC_print("cmd '%c'\r\n", c );
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

