
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
#include "param.h"
#include <stdio.h>
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
#define PARAM_FLAG	8

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

#ifdef USE_SDCARD
#include "ff_gen_drv.h"
#include "sd_diskio.h"
FATFS SDFatFs;  /* File system object for SD card logical drive */
FIL MyFile;     /* File object */
char SDPath[4]; /* SD card logical drive path */
#endif

// ----------------------  Interrupts ----------------------

void SysTick_Handler(void)
{
  HAL_IncTick();
}

// ---------------------- application ----------------------

#ifdef USE_SDCARD
#define CRC_POLY 0xEDB88320	// polynome de zlib, zip et ethernet
static unsigned int crc_table[256];
// The table is simply the CRC of all possible eight bit values.
static void make_crc_table()
{
unsigned int c, n, k;
for ( n = 0; n < 256; n++ )
    {
    c = n;
    for ( k = 0; k < 8; k++ )
        c = c & 1 ? CRC_POLY ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
    }
}
// cumuler le calcul du CRC
// initialiser avec :	crc = 0xffffffff;
// finir avec :		crc ^= 0xffffffff;
static void icrc32( const unsigned char *buf, int len, unsigned int * crc )
{
do	{
	*crc = crc_table[((*crc) ^ (*buf++)) & 0xff] ^ ((*crc) >> 8);
	} while (--len);
}
// random file with CRC - size is in bytes
// rend la duree en s, ou <0 si erreur
#define QBUF 32768
static int write_test_file( unsigned int size, const char * path, unsigned int * crc )
{
unsigned char wbuf[QBUF];
unsigned int tstart, tstop;
unsigned int cnt, wcnt;
make_crc_table();
*crc = 0xffffffff;
jrtc_get_day_time( &daytime ); tstart = daytime.ss + 60 * daytime.mn;
if	( f_open( &MyFile, path, FA_CREATE_ALWAYS | FA_WRITE ) )
	return -1;
while	( size )
	{
	cnt = 0;
	while	( ( size ) && ( cnt < QBUF ) )
		{
		wbuf[cnt] = rand();
		--size; ++cnt;
		}
	icrc32( wbuf, cnt, crc );
	if	( f_write( &MyFile, wbuf, cnt, &wcnt ) )
		return -2;
	if	( wcnt != cnt )
		return -3;
	}
*crc ^= 0xffffffff;
// fermer
f_close(&MyFile);
jrtc_get_day_time( &daytime ); tstop = daytime.ss + 60 * daytime.mn;
tstop -= tstart;
if	( tstop < 0 )
	tstop += 3600;
return tstop;
}
#endif

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
idrag.yobj = 0;	//pour presque tous les cas
if	( show_flags & MENU_FLAG )
	idrag.yobjmin = - menu.ty;
else
#ifdef USE_TRANSCRIPT
if	( show_flags & TRANS_FLAG )
	{ idrag.yobjmin = LCD_DY - trans.dy; idrag.yobj = idrag.yobjmin; }
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
		show_flags = TRANS_FLAG | LOGO_FLAG;
		break;
	case PARAM_FLAG :
		show_flags = PARAM_FLAG | LOGO_FLAG;
		break;
	case DEMO_FLAG :
		show_flags = DEMO_FLAG | LOGO_FLAG;
		break;
	default:
		show_flags = LOGO_FLAG;
	}
#ifdef USE_TIME_DATE
show_flags |= ( DATE_FLAG | HOUR_FLAG );
#endif
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
	{
	if	( show_flags & TIME_ADJ_FLAGS )
		kmenu = menu_draw( menu.last_ypos );
	else	kmenu = menu_draw( idrag.yobj );
	}
else
#ifdef USE_DEMO
if	( show_flags & DEMO_FLAG )	// page de demo scrollable
	demo_draw( idrag.yobj, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
else
#endif
#ifdef USE_TRANSCRIPT
if	( show_flags & TRANS_FLAG )	// page de transcript scrollable
	{
	if	( show_flags & TIME_ADJ_FLAGS )
		transdraw( trans.last_ypos );
	else	transdraw( idrag.yobj );
	}
else
#endif
#ifdef USE_PARAM
if	( show_flags & PARAM_FLAG )	// page de parametres scrollable
	{
	if	( show_flags & TIME_ADJ_FLAGS )
		param_draw( para.last_ypos );
	else	param_draw( idrag.yobj );
	}
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

idrag_init();
idrag.yobjmax = 0;
idrag.yobjmin = -LCD_DY;

create_menu();
#ifdef USE_TRANSCRIPT
transcript_init( &JFont16n, SCROLL_ZONE_X0, SCROLL_ZONE_DX );
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
			if	(
				( show_flags & TIME_ADJ_FLAGS )
				#ifdef USE_PARAM
				|| ( ( show_flags & PARAM_FLAG ) && ( para.editing ) )
				#endif
				)
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
				#ifdef USE_PARAM
				if	( ( touch_occur_cnt == SHORT_TOUCH_DELAY ) && ( show_flags & PARAM_FLAG ) )
					param_select( TS_State.touchY[0] );
				#endif
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
			( ( show_flags & PARAM_FLAG ) == 0 ) &&
			( x2 > FIX_ZONE_X0 ) &&
			( x2 < ( FIX_ZONE_X0 + FIX_ZONE_DX ) )
			)
			{
			if	( y2 > ( YHOUR - 10 ) )
				{
				if	( x2 < ( FIX_ZONE_X0 + 70 ) )
					{
					show_flags |= ( HH_ADJ_FLAG | HOUR_FLAG );
					idrag.yobj = adju_start( &JFont20, FIX_ZONE_X0+20, YHOUR+18, 44, 60, 0, 24, daytime.hh );
					}
				else if	( x2 < ( FIX_ZONE_X0 + 130 ) )
					{
					show_flags |= ( MN_ADJ_FLAG | HOUR_FLAG );
					idrag.yobj = adju_start( &JFont20, FIX_ZONE_X0+79, YHOUR+18, 44, 60, 0, 60, daytime.mn );
					}
				// else	rien
				}
			else if	( y2 > ( YDATE - 10 ) )
				{
				if	( x2 < ( FIX_ZONE_X0 + 90 ) )
					{
					show_flags |= WD_ADJ_FLAG | DATE_FLAG;
					idrag.yobj = adju_start( &JFont20, FIX_ZONE_X0+8, YDATE+14, 85, 60, 0, 5, daytime.wd );
					}
				else if	( x2 < ( FIX_ZONE_X0 + 130 ) )
					{
					show_flags |= MD_ADJ_FLAG | DATE_FLAG;
					idrag.yobj = adju_start( &JFont20, FIX_ZONE_X0+90, YDATE+14, 38, 60, 1, 32, daytime.md );
					}
				else	{
					show_flags |= MM_ADJ_FLAG | DATE_FLAG;
					idrag.yobj = adju_start( &JFont20, FIX_ZONE_X0+135, YDATE+14, 38, 60, 1, 13, daytime.mm );
					}
				}
			if	( show_flags & TIME_ADJ_FLAGS )		// elements communs a tous les time adjs
				{
				idrag.yobjmin = - adj.ty;	// a faire apres adju_start()
				idrag.touching = 0;		// forcer un nouveau landing
				}
			}
		else
		#endif
		#ifdef USE_PARAM
		if	( ( old_touch_cnt == 1 ) && ( show_flags & PARAM_FLAG ) && ( para.editing == 0 ) )
			{
			idrag.yobj = param_start();	// demarrer l'edition sur le param qui a ete deja selectionne
			idrag.yobjmin = - adj.ty;	// a faire apres adju_start()
			idrag.touching = 0;		// forcer un nouveau landing
			}
		else
		#endif
		{}	// pour le dernier else
		idrag_event_call( TS_State.touchDetected,
				  TS_State.touchX[1], TS_State.touchY[1],
				  GC.ltdc_irq_cnt );
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
			#ifdef USE_PARAM
			if	( show_flags & PARAM_FLAG )
				{
				if	( para.editing )
					{
					param_save();
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
		{					// traitement cadence a la seconde
		#ifdef USE_TRANSCRIPT
		transprint("-> %02d:%02d:%02d", daytime.hh, daytime.mn, daytime.ss );
		#endif
		#ifdef USE_UART1
		// CDC_print("-> %02d show %08x\r\n", daytime.ss, show_flags );
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
	{			// attention : un seul CDC_print() par commande, car il n'y a pas de queue
	int c = CDC_getcmd();
	if	( c > 0 )
		{
		#ifdef USE_SDCARD
		switch	( c )
			{
			case 'm' :	// linker le driver (connection soft, n'aborde pas le HW)
				if	( FATFS_LinkDriver(&SD_Driver, SDPath) )
					CDC_print("\nfailed : FATFS_LinkDriver\n");
				else	{
					// monter le FS
					if	( f_mount( &SDFatFs, (TCHAR const*)SDPath, 0) )
						CDC_print("\nFailed : f_mount\n");
					else	CDC_print("\nOk : f_mount {%s}\n", SDPath );
					}
			break;
			case 'r' :	// ouvrir fichier en lecture
				if	( f_open( &MyFile, "DEMO.TXT", FA_READ ) )
					CDC_print("Failed : f_open r DEMO.TXT\n");
				else	{
					// lire 1 buffer
					unsigned int bytesread = 0; char tbuf[64];
					if	( f_read( &MyFile, tbuf, sizeof(tbuf), &bytesread ) )
						CDC_print("Ok : f_open r DEMO.TXT\nFailed : f_read\n");
					else	{
						if	( bytesread < sizeof(tbuf) )
							tbuf[bytesread] = 0;
						else	tbuf[sizeof(tbuf)-1] = 0;
						CDC_print("Ok : f_open r DEMO.TXT\nOk : f_read %d bytes {%s}\n", bytesread, tbuf );
						}
					// fermer
					f_close(&MyFile);
					}
			break;
			case 'w' :	// ouvrir fichier en ecriture
				if	( f_open( &MyFile, "WEMO.TXT", FA_CREATE_ALWAYS | FA_WRITE ) )
					CDC_print("Failed : f_open w WEMO.TXT\n");
				else	{
					// ecrire un peu
					unsigned int byteswritten = 0; const char wbuf[] = "One Love, One Heart";
					// N.B. ne pas ecrire le NULL dans le fichier ==> sizeof(wbuf)-1
					if	( f_write( &MyFile, wbuf, sizeof(wbuf)-1, &byteswritten ) )
						CDC_print("Ok : f_open w WEMO.TXT\nFailed : f_write\n");
					else	CDC_print("Ok : f_open w WEMO.TXT\nOk : f_write %d bytes\n", byteswritten );
					// fermer
					f_close(&MyFile);
					}
			break;
			case 'a' :	// ouvrir fichier en append
				if	( f_open( &MyFile, "WEMO.TXT", FA_OPEN_APPEND | FA_WRITE ) )
					CDC_print("Failed : f_open a WEMO.TXT\n");
				else	{
					// ecrire un peu
					unsigned int byteswritten = 0; const char wbuf[] = " !";
					if	( f_write( &MyFile, wbuf, sizeof(wbuf)-1, &byteswritten ) )
						CDC_print("Ok : f_open a WEMO.TXT\nFailed : f_write\n");
					else	CDC_print("Ok : f_open a WEMO.TXT\nOk : f_write %d bytes\n", byteswritten );
					// fermer
					f_close(&MyFile);
					}
			break;
			case '1' :	// creer un fichier de test
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' :
				{
				unsigned int retval, crc, size;
				char fnam[32];
				size = c - '0';
				snprintf( fnam, sizeof(fnam), "test%d00M.bin", size );
				size *= 100000000;
				retval = write_test_file( size, fnam, &crc );
				if	( retval < 0 )
					CDC_print("Failed : write %s : code %d\n", fnam, retval );
				else	CDC_print("Ok : write %s in %d s, crc %08X\n", fnam, retval, crc );
				}
			break;
			default :
				CDC_print("cmd '%c'\r\n", c );
			} // switch c
		#else
		CDC_print("cmd '%c'\r\n", c );
		#endif
		}
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

