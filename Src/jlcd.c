/* Include LCD component Driver */
/* LCD RK043FN48H-CT672B 4,3" 480x272 pixels */
#include "../Components/rk043fn48h/rk043fn48h.h"

/* Include SDRAM Driver */
#include "stm32746g_discovery_sdram.h"
 
#include "stm32746g_discovery.h"

#include "options.h"
#include "jlcd.h"

#ifdef COMPILE_THE_FONTS
#include "../Fonts/jfont24.c"
#include "../Fonts/jfont20.c"
#include "../Fonts/jfont16.c"
#include "../Fonts/jfont12.c"
#include "../Fonts/jfont8.c"
#ifdef FLASH_THE_FONTS
#include "../Fonts/flashy.c"
#endif
#else
const JFONT JFont24 = {
  (const unsigned short *)FLASH_FONTS_BASE,
  16, // w
  21, // h
  17, // dx
  24  // dy
};
const JFONT JFont20 = {
  ((const unsigned short *)FLASH_FONTS_BASE) + 21*QCHAR,
  13, // w
  17, // h
  14, // dx
  20  // dy
};
const JFONT JFont16 = {
  ((const unsigned short *)FLASH_FONTS_BASE) + (21+17)*QCHAR,
  11, // w
  13, // h
  11, // dx
  16  // dy
};
const JFONT JFont16n = {
  ((const unsigned short *)FLASH_FONTS_BASE) + (21+17)*QCHAR,
  11, // w
  13, // h
  10, // dx
  16  // dy
};
const JFONT JFont12 = {
  ((const unsigned short *)FLASH_FONTS_BASE) + (21+17+13)*QCHAR,
  7, // w
  10, // h
  7, // dx
  12  // dy
};
const JFONT JFont8 = {
  ((const unsigned short *)FLASH_FONTS_BASE) + (21+17+13+10)*QCHAR,
  5, // w
  8, // h
  5, // dx
  8  // dy
};
#endif

// fonts variables toujours compilees pour le moment
// necessitent une BMP chargee en flash separement
#include "vfonts.c"

// contexte graphic global
GCtype GC;

// constructeur
void GC_init()
{
GC.ilayer = 0;
GC.fb_base = LAYER_BASE(GC.ilayer);
GC.font = &JFont20;
GC.vfont = &JVFont36n;
GC.bg_color = ARGB_WHITE;
GC.text_color = ARGB_BLACK;
GC.fill_color = ARGB_GREEN;
GC.line_color = ARGB_RED;
GC.ytop = 0;
GC.ybot = LCD_DY;
GC.ltdc_irq_cnt = 0;
}

// ---------------------- vertical blank interrupt -------------------


// interrupt routine
void LTDC_IRQHandler(void)
{
// LTDC->ICR = LTDC_ICR_CRRIF;		// register reload (no good)
LTDC->ICR = LTDC_ICR_CLIF;		// vertical blank
++GC.ltdc_irq_cnt;
HAL_IncTick();		// cette interruption se substitue au HAL tick (systick)
}

// ---------------------- acces direct LTC --------------------------


void jlcd_reload_shadows()
{
LTDC->SRCR |= LTDC_SRCR_VBR;
}

void jlcd_reload_shadows_and_wait()
{
LTDC->SRCR |= LTDC_SRCR_VBR;
while	( LTDC->SRCR & LTDC_SRCR_VBR )
	{}
}

void jlcd_layer_enable( int layer )
{
if	( layer == 0 )
	LTDC_Layer1->CR = 1;
else	LTDC_Layer2->CR = 1;
}

void jlcd_layer_disable( int layer )
{
if	( layer == 0 )
	LTDC_Layer1->CR = 0;
else	LTDC_Layer2->CR = 0;
}

void jlcd_layer_alpha( int layer, int alpha )
{
if	( layer == 0 )
	LTDC_Layer1->CACR = alpha;
else	LTDC_Layer2->CACR = alpha;
}

// ---------------------- acces direct DMA2D --------------------------

// copie d'un sprite en ARGB8888 (bloquant)
// les translations src et dest doivent etre incluses dans les adresses src_addr
// et dest_addr a calculer au prealable
// (les calculs ne sont pas inclus car aussi bien src que dst peuvent etre dans le
// frame-buffer ou ailleurs)
void jdma_sprite_blit(	unsigned int src_addr, unsigned int src_stride,
			unsigned int dest_addr, unsigned int dest_stride,
			unsigned int width, unsigned int height )
{
// config source : c'est ce qu'on appelle "foreground"
DMA2D->FGMAR = src_addr;
DMA2D->FGOR = src_stride - width;
DMA2D->FGPFCCR = CM_ARGB8888;		// en laissant le reste a zero on copie aussi alpha
// config dest
DMA2D->OMAR = dest_addr;
DMA2D->OOR = dest_stride - width;
DMA2D->OPFCCR = CM_ARGB8888;
// params communs
DMA2D->NLR = width << 16 | height;
DMA2D->CR = DMA2D_M2M;
// go
DMA2D->CR |= DMA2D_CR_START;
while	( DMA2D->CR & DMA2D_CR_START )
	{}
DMA2D->IFCR = 0x3F;	// clear des interrups flags pour les fonctions HAL qui les testent
}

// fill d'un rectangle en ARGB8888 (bloquant)
// la translation dest doivent etre incluses dans dest_addr a calculer au prealable
// (les calculs ne sont pas inclus car dst peut etre dans le frame-buffer ou ailleurs)
void jdma_rect_fill(  	unsigned int dest_addr, unsigned int dest_stride,
			unsigned int width, unsigned int height, unsigned int argb )
{
// config src
DMA2D->OCOLR = argb;
// config dest
DMA2D->OMAR = dest_addr;
DMA2D->OOR = dest_stride - width;
DMA2D->OPFCCR = CM_ARGB8888;
// params communs
DMA2D->NLR = width << 16 | height;
DMA2D->CR = DMA2D_R2M;
// go
DMA2D->CR |= DMA2D_CR_START;
while	( DMA2D->CR & DMA2D_CR_START )
	{}
DMA2D->IFCR = 0x3F;	// clear des interrups flags pour les fonctions HAL qui les testent
}

// conversion d'une ligne d'image de RGB888 en ARGB8888 (bloquant)
// les translations src et dest doivent etre incluses dans les adresses src_addr
// et dest_addr a calculer au prealable
// 1 seule ligne a la fois because fichier BMP necessite changer l'ordre des lignes
void jdma_RGB_line(	unsigned int src_addr,
			unsigned int dest_addr, unsigned int dest_stride,
			unsigned int width )
{
// config source : c'est ce qu'on appelle "foreground"
DMA2D->FGMAR = src_addr;
DMA2D->FGOR = 0;		// sans objet : 1 seule ligne
DMA2D->FGPFCCR = CM_RGB888;	// en laissant le reste a zero on force alpha = FF
// config dest
DMA2D->OMAR = dest_addr;
DMA2D->OOR = dest_stride - width;
DMA2D->OPFCCR = CM_ARGB8888;
// params communs
DMA2D->NLR = width << 16 | 1;
DMA2D->CR = DMA2D_M2M_PFC;
// go
DMA2D->CR |= DMA2D_CR_START;
while	( DMA2D->CR & DMA2D_CR_START )
	{}
DMA2D->IFCR = 0x3F;	// clear des interrups flags pour les fonctions HAL qui les testent
}

// ---------------------- acces frame buffer selon GC --------------------------

// permuter les buffers dans le contexte double-buffering
// prendra effet avec jlcd_reload_shadows() ou jlcd_reload_shadows_and_wait()
void swap_layer( void )
{
// le layer pointe par ilayer sera affiche au prochain vblank
jlcd_layer_enable( GC.ilayer );
jlcd_layer_alpha( GC.ilayer, 255 );
// on change de layer
GC.ilayer ^= 1;
// a present ilayer designe le layer sur lequel on dessine, c'est a dire PAS celui qui est visible !
jlcd_layer_disable( GC.ilayer );
GC.fb_base = LAYER_BASE(GC.ilayer);
}

// fill background en ARGB8888 (bloquant)
void jlcd_bg_fill( void )
{
jdma_rect_fill( (unsigned int)GC.fb_base, LCD_DX, LCD_DX, LCD_DY, GC.bg_color );
}

// fill d'un rectangle en ARGB8888 (bloquant)
void jlcd_rect_fill( unsigned int x, unsigned int y, unsigned int w, unsigned int h )
{
unsigned int adr_base;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, w, h, GC.fill_color );
}

// ligne horizontale en ARGB8888 (bloquant)
void jlcd_hline( unsigned int x, unsigned int y, unsigned int w )
{
unsigned int adr_base;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, w, 1, GC.line_color );
}

// ligne verticale en ARGB8888 (bloquant)
void jlcd_vline( unsigned int x, unsigned int y, unsigned int h )
{
unsigned int adr_base;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, 1, h, GC.line_color );
}

// fill d'un rectangle en ARGB8888 (bloquant)
// avec clip y (supporte y < 0 )
void jlcd_yclip_rect_fill( int x, int y, int w, int h )
{
unsigned int adr_base; int y1;
y1 = y + h;
if	( y < GC.ytop )	 y  = GC.ytop;
if	( y1 > GC.ybot ) y1 = GC.ybot;
h = y1 - y;
if	( h <= 0 )
	return;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, w, h, GC.fill_color );
}

// ligne horizontale en ARGB8888 (bloquant)
// avec clip y (supporte y < 0 )
void jlcd_yclip_hline( int x, int y, int w )
{
unsigned int adr_base;
if	( ( y < GC.ytop ) || ( y >= GC.ybot ) )
	return;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, w, 1, GC.line_color );
}

// ligne verticale en ARGB8888 (bloquant)
// avec clip y (supporte y < 0 )
void jlcd_yclip_vline( int x, int y, int h )
{
unsigned int adr_base; int y1;
y1 = y + h;
if	( y < GC.ytop )	 y  = GC.ytop;
if	( y1 > GC.ybot ) y1 = GC.ybot;
h = y1 - y;
if	( h <= 0 )
	return;
adr_base = (unsigned int)GC.fb_base + ( ( ( y * LCD_DX ) + x ) << 2 );
jdma_rect_fill( adr_base, LCD_DX, 1, h, GC.line_color );
}
// trace de char a fond transparent base sur une font JLN
void jlcd_char( unsigned int x, unsigned int y, int ascii )
{
unsigned int i, j;
unsigned int height, width, line;
__IO uint32_t * adr_base;	// en mode ARGB8888
const uint16_t *prow;
  
ascii -= ' ';			// localiser le char dans la table
if	( ascii < 0 )
	ascii = 0;
height = GC.font->h;
prow = GC.font->rows + ( ascii * height );
adr_base = GC.fb_base + ( y * LCD_DX ) + x;
width  = GC.font->w;

for	( i = 0; i < height; i++ )
	{				// boucle Y
	line =  prow[i];      
	for	( j = 0; j < width; j++ )
		{			// boucle X
		if	( line & ( 0x8000 >> j ) )
			adr_base[j] = GC.text_color;
		}
	adr_base += LCD_DX;
	}
}

// trace de char a fond transparent base sur une font JLN
// avec clip en y
void jlcd_yclip_char( int x, int y, int ascii )
{
int i, j;
int height, width, line;
__IO uint32_t * adr_base;	// en mode ARGB8888
const uint16_t *prow;
  
height = GC.font->h;
width  = GC.font->w;
ascii -= ' ';			// localiser le char dans la table
if	( ascii < 0 )
	ascii = 0;
prow = GC.font->rows + ( ascii * height );
adr_base = GC.fb_base + ( y * LCD_DX ) + x;

for	( i = y; i < (y+height); i++ )
	{				// boucle Y
	if	( i >= GC.ybot )
		break;
	if	( i >= GC.ytop )
		{
		line =  prow[i-y];      
		for	( j = 0; j < width; j++ )
			{			// boucle X
			if	( line & ( 0x8000 >> j ) )
				adr_base[j] = GC.text_color;
			}
		}
	adr_base += LCD_DX;
	}
}

// trace de char opaque base sur une font JLN variable, rend sa largeur effective
int jlcd_vchar( unsigned int x, unsigned int y, int ascii )
{
int pos; 
const JVCHAR * def;
pos = ascii - GC.vfont->ascii0;
if	( ( pos < 0 ) || ( pos >= GC.vfont->qchar ) )
	return 0;	// petite precaution sinon erreur indiagnosticable...
def = GC.vfont->chardefs + pos;
jlcd_blit_bmp(  x, y, def->x, def->y, def->w, def->h, (uint8_t *)GC.vfont->base_adr );
return( def->w );
}

// trace de texte a fond transparent base sur une font JLN
void jlcd_text( int x, int y, const char * tbuf )
{
while	( *tbuf )
	{
	jlcd_char( x, y, *(tbuf++) );
	x += GC.font->dx;
	}
}

// trace de texte a fond transparent base sur une font JLN
// avec clip en y
void jlcd_yclip_text( int x, int y, const char * tbuf )
{
if	( y >= GC.ybot )
	return;
if	( y < ( GC.ytop - GC.font->h ) )
	return;
while	( *tbuf )
	{
	jlcd_yclip_char( x, y, *(tbuf++) );
	x += GC.font->dx;
	}
}

// trace de texte opaque base sur une font JLN variable, rend x final
int jlcd_vtext( int x, int y, const char * tbuf )
{
while	( *tbuf )
	{
	x += jlcd_vchar( x, y, *(tbuf++) );
	x += GC.vfont->sx;
	}
return x;
}

// mesure de texte opaque base sur une font JLN variable, rend largeur
int jlcd_vtext_dx( const char * tbuf )
{
int x, pos; 
x = 0;
while	( *tbuf )
	{
	pos = *(tbuf++) - GC.vfont->ascii0;
	if	( ( pos >= 0 ) && ( pos < GC.vfont->qchar ) )
		x += GC.vfont->chardefs[pos].w;
	x += GC.vfont->sx;
	}
if	( x > 0 )
	x -= GC.vfont->sx;
return x;
}

// extraire et afficher l'image d'un fichier BMP RGB present en memoire
// les dimensions sont lues dans le header du fichier
int jlcd_draw_bmp( uint32_t x, uint32_t y, uint8_t *pbmp )
{
uint32_t data_pos, w, h, dest_adr;
  
// lecture BMP header
data_pos  = *( uint16_t *)( pbmp + 10 );
w = *(uint16_t *)( pbmp + 18 );
h = *(uint16_t *)( pbmp + 22 );
  
// bit/pixel ?
if	( (*(uint16_t *)( pbmp + 28 )) != 24 )
	return 1;
  
dest_adr = (unsigned int)GC.fb_base + ( ( ( LCD_DX * y ) + x ) << 2 );
  
// Bypass the bitmap header
pbmp += data_pos;

// commencer en bas de l'image
dest_adr += ( ( LCD_DX * 4 ) * ( h - 1 ) );

// parcourir l'image dans le sens du fichier BMP i.e. en remontant  
for	( y = 0; y < h; ++y )
	{
	jdma_RGB_line( (unsigned int)pbmp, dest_adr, LCD_DX, w );
	dest_adr -= ( LCD_DX * 4 );
	pbmp += ( w * 3 );
	} 
return 0;
}

// extraire et afficher une zone rectangulaire prise dans un fichier BMP RGB present en memoire
// aucune protection contre les debordements
int jlcd_blit_bmp( uint32_t xd, uint32_t yd, uint32_t xs, uint32_t ys,
		   uint32_t w, uint32_t h, uint8_t *pbmp )
{
uint32_t wbmp, hbmp, data_pos, dest_adr;
  
// lecture BMP header
data_pos  = *( uint16_t *)( pbmp + 10 );
wbmp = *(uint16_t *)( pbmp + 18 );
hbmp = *(uint16_t *)( pbmp + 22 );
  
// bit/pixel ?
if	( (*(uint16_t *)( pbmp + 28 )) != 24 )
	return 1;
  
dest_adr = (unsigned int)GC.fb_base + ( ( ( LCD_DX * yd ) + xd ) << 2 );
  
// Bypass the bitmap header
pbmp += data_pos;

// commencer en bas de l'image
dest_adr += ( ( LCD_DX * 4 ) * ( h - 1 ) );
pbmp     += ( ( wbmp * ( hbmp - h - ys ) ) + xs ) * 3;
// parcourir l'image dans le sens du fichier BMP i.e. en remontant  
for	( ys = 0; ys < h; ++ys )
	{
	jdma_RGB_line( (unsigned int)pbmp, dest_adr, LCD_DX, w );
	dest_adr -= ( LCD_DX * 4 );
	pbmp += ( wbmp * 3 );
	} 
return 0;
}

// tracer une ligne oblique (pourrait etre optimise)
void jlcd_line( unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2 )
{
int deltax, deltay, x, y, xinc1, xinc2, yinc1, yinc2;
int den, num, num_add, num_pixels, curpixel;
  
deltax = x2 - x1;
deltay = y2 - y1;
x = x1;
y = y1;
if	( x2 >= x1 )
	{ xinc1 =  1; xinc2 =  1; }
else	{ xinc1 = -1; xinc2 = -1; deltax = -deltax; }
if	( y2 >= y1 )
	{ yinc1 =  1; yinc2 =  1; }
else	{ yinc1 = -1; yinc2 = -1; deltay = -deltay; }
  
if	(deltax >= deltay)	// There is at least one x-value for every y-value
	{
	xinc1 = 0;	// Don't change the x when numerator >= denominator
	yinc2 = 0;	// Don't change the y for every iteration
	den = deltax;
	num = deltax / 2;
	num_add = deltay;
	num_pixels = deltax;	// There are more x-values than y-values
	}
else	{			// There is at least one y-value for every x-value
	xinc2 = 0;	// Don't change the x for every iteration
	yinc1 = 0;	// Don't change the y when numerator >= denominator
	den = deltay;
	num = deltay / 2;
	num_add = deltax;
	num_pixels = deltay;	// There are more y-values than x-values
	}
  
for	( curpixel = 0; curpixel <= num_pixels; curpixel++ )
	{
	// draw 1 pixel
	GC.fb_base[ y*LCD_DX + x] = GC.line_color;
	// update x, y
	num += num_add;	// Increase the numerator by the top of the fraction
	if	( num >= den )
		{
		num -= den;
		x += xinc1;
		y += yinc1;
		}
	x += xinc2;
	y += yinc2;
	}
}

// tracer une fleche vers la droite pour index visuel
// supporte yclip, utilise GC.line_color
// y est l'ordonnee de l'axe de symetrie
// w est la largeur totale, doit etre > h sinon on ne voit rien
// l'"epaisseur" horizontale est w-h
void draw_r_arrow( int x, int y, int w, int h )
{
int i;
// ligne mediane
h >>= 1;		// demi hauteur
w -= ( h << 1 );	// longueur de chaque ligne
jlcd_yclip_hline( x+2*h, y, w );
for	( i = 1; i <= h; ++i )
	{
	jlcd_yclip_hline( x+2*(h-i), y-i, w );
	jlcd_yclip_hline( x+2*(h-i), y+i, w );
	}
}

void draw_centered_text( int x, int y, int len, const char * txt )
{
x -= ( ( len * GC.font->dx ) >> 1 );
while	( len-- )
	{
	jlcd_yclip_char( x, y, *(txt++) );
	x += GC.font->dx;
	}
}

// ======================== initialisation ==================================

/* Display enable pin */
#define LCD_DISP_GPIOI_PIN	GPIO_PIN_12
/* Backlight control pin */
#define LCD_BL_GPIOK_PIN	GPIO_PIN_3

// phase 1 : 2 pins as GPIO out, for panel enable & backlight
// tout est laisse off
void jlcd_gpio1( void )
{
GPIO_InitTypeDef gpio_init_structure;
/* Enable GPIOs clock */
__HAL_RCC_GPIOI_CLK_ENABLE();
__HAL_RCC_GPIOK_CLK_ENABLE();
// GPIO output pins
/* LCD_DISP to panel */
HAL_GPIO_WritePin( GPIOI, LCD_DISP_GPIOI_PIN, GPIO_PIN_RESET);
gpio_init_structure.Pin       = LCD_DISP_GPIOI_PIN;
gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
gpio_init_structure.Pull      = GPIO_NOPULL;
gpio_init_structure.Speed     = GPIO_SPEED_LOW;
HAL_GPIO_Init(GPIOI, &gpio_init_structure);

/* LCD_BL to baklight power supply */
HAL_GPIO_WritePin( GPIOK, LCD_BL_GPIOK_PIN, GPIO_PIN_RESET);
gpio_init_structure.Pin       = LCD_BL_GPIOK_PIN;
gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
HAL_GPIO_Init(GPIOK, &gpio_init_structure);
}

// phase 2 : all pins assigned to LTDC as "alternate functions",
void jlcd_gpio2( void )
{
GPIO_InitTypeDef gpio_init_structure;
/* Enable GPIOs clock */
__HAL_RCC_GPIOE_CLK_ENABLE();
__HAL_RCC_GPIOG_CLK_ENABLE();
__HAL_RCC_GPIOI_CLK_ENABLE();
__HAL_RCC_GPIOJ_CLK_ENABLE();
__HAL_RCC_GPIOK_CLK_ENABLE();

/*** LTDC Pins configuration ***/
/* GPIOE configuration */
gpio_init_structure.Pin       = GPIO_PIN_4;
gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
gpio_init_structure.Pull      = GPIO_NOPULL;
gpio_init_structure.Speed     = GPIO_SPEED_FAST;
gpio_init_structure.Alternate = GPIO_AF14_LTDC;  
HAL_GPIO_Init(GPIOE, &gpio_init_structure);

/* GPIOG configuration */
gpio_init_structure.Pin       = GPIO_PIN_12;
gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
gpio_init_structure.Alternate = GPIO_AF9_LTDC;
HAL_GPIO_Init(GPIOG, &gpio_init_structure);

/* GPIOI LTDC alternate configuration */
gpio_init_structure.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | \
                                GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
gpio_init_structure.Alternate = GPIO_AF14_LTDC;
HAL_GPIO_Init(GPIOI, &gpio_init_structure);

/* GPIOJ configuration */  
gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
                                GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
gpio_init_structure.Alternate = GPIO_AF14_LTDC;
HAL_GPIO_Init(GPIOJ, &gpio_init_structure);  

/* GPIOK configuration */  
gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | \
                                GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
gpio_init_structure.Alternate = GPIO_AF14_LTDC;
HAL_GPIO_Init(GPIOK, &gpio_init_structure);
}

void jlcd_sdram_init( void )
{
BSP_SDRAM_Init();
}

// LTDC init
/* prerequis non inclus :
jlcd_gpio1();
jlcd_gpio2();
jlcd_sdram_init();
jlcd_panel_on();
__HAL_RCC_DMA2D_CLK_ENABLE();
*/
void jlcd_init(void)
{
LTDC_HandleTypeDef  hLtdcHandler;
RCC_PeriphCLKInitTypeDef  periph_clk_init_struct;
LTDC_LayerCfgTypeDef  layer_cfg;

// l'adresse de base du LTDC
hLtdcHandler.Instance = LTDC;

/* The RK043FN48H LCD panel 480x272 */

/* Timing Configuration */
hLtdcHandler.Init.HorizontalSync = (RK043FN48H_HSYNC - 1);
hLtdcHandler.Init.VerticalSync = (RK043FN48H_VSYNC - 1);
hLtdcHandler.Init.AccumulatedHBP = (RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
hLtdcHandler.Init.AccumulatedVBP = (RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
hLtdcHandler.Init.AccumulatedActiveH = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP - 1);
hLtdcHandler.Init.AccumulatedActiveW = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP - 1);
hLtdcHandler.Init.TotalHeigh = (RK043FN48H_HEIGHT + RK043FN48H_VSYNC + RK043FN48H_VBP + RK043FN48H_VFP - 1);
hLtdcHandler.Init.TotalWidth = (RK043FN48H_WIDTH + RK043FN48H_HSYNC + RK043FN48H_HBP + RK043FN48H_HFP - 1);
  
/* LTDC clock configuration for RK043FN48H panel */
/* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
/* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
/* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
/* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
periph_clk_init_struct.PLLSAI.PLLSAIN = 192;
periph_clk_init_struct.PLLSAI.PLLSAIR = RK043FN48H_FREQUENCY_DIVIDER;
periph_clk_init_struct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);

/* Initialize the LCD pixel width and pixel height */
hLtdcHandler.LayerCfg->ImageWidth  = RK043FN48H_WIDTH;
hLtdcHandler.LayerCfg->ImageHeight = RK043FN48H_HEIGHT;

/* Background value */
hLtdcHandler.Init.Backcolor.Blue = 0;
hLtdcHandler.Init.Backcolor.Green = 0;
hLtdcHandler.Init.Backcolor.Red = 0;
  
/* Polarity */
hLtdcHandler.Init.HSPolarity = LTDC_HSPOLARITY_AL;
hLtdcHandler.Init.VSPolarity = LTDC_VSPOLARITY_AL; 
hLtdcHandler.Init.DEPolarity = LTDC_DEPOLARITY_AL;  
hLtdcHandler.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

/* Enable the LTDC clock */
__HAL_RCC_LTDC_CLK_ENABLE();

HAL_LTDC_Init(&hLtdcHandler);

/* Layer 0 Init */
layer_cfg.WindowX0 = 0;
layer_cfg.WindowX1 = LCD_DX;
layer_cfg.WindowY0 = 0;
layer_cfg.WindowY1 = LCD_DY; 
layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
layer_cfg.FBStartAdress = (unsigned int)LCD_BASE_0;
layer_cfg.Alpha = 255;
layer_cfg.Alpha0 = 0;
layer_cfg.Backcolor.Blue = 0;
layer_cfg.Backcolor.Green = 0;
layer_cfg.Backcolor.Red = 0;
layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
layer_cfg.ImageWidth = LCD_DX;
layer_cfg.ImageHeight = LCD_DY;
HAL_LTDC_ConfigLayer( &hLtdcHandler, &layer_cfg, 0 ); 

/* Layer 1 Init */
layer_cfg.FBStartAdress = (unsigned int)LCD_BASE_1;
HAL_LTDC_ConfigLayer( &hLtdcHandler, &layer_cfg, 1 ); 


// switch LTDC on
__HAL_LTDC_ENABLE(&hLtdcHandler);
}

void jlcd_interrupt_on( void )
{
HAL_SuspendTick();	// notre interrupt remplace HAL tick

// interrupt : register reload (not good)
// LTDC->IER = LTDC_IER_RRIE;

// interrupt : "line" interrupt (interrupt at a given line number)
// last active line = VSYNC Width + VBP + Active Height - 1 in the LTDC_AWCR register
LTDC->LIPCR = ( LTDC->AWCR & 0x03FF );
LTDC->IER = LTDC_IER_LIE;

HAL_NVIC_SetPriority( LTDC_IRQn, 2, 0 );
HAL_NVIC_EnableIRQ( LTDC_IRQn );
}

void jlcd_panel_on( void )
{
// switch panel on
HAL_GPIO_WritePin( GPIOI, LCD_DISP_GPIOI_PIN, GPIO_PIN_SET);
// switch backlight on
HAL_GPIO_WritePin( GPIOK, LCD_BL_GPIOK_PIN, GPIO_PIN_SET);
}

void jlcd_panel_off( void )
{
// switch panel off
HAL_GPIO_WritePin( GPIOI, LCD_DISP_GPIOI_PIN, GPIO_PIN_RESET);
// switch backlight off
HAL_GPIO_WritePin( GPIOK, LCD_BL_GPIOK_PIN, GPIO_PIN_RESET);
}

int jlcd_is_panel_on( void )
{
return HAL_GPIO_ReadPin( GPIOI, LCD_DISP_GPIOI_PIN );
}

