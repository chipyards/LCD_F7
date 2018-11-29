/* LCD on STM32F746 discovery */

#define LCD_DX 480	// format 30/17
#define LCD_DY 272
#define LCD_BASE_0 ((volatile unsigned int*)0xC0000000)
#define LCD_BASE_1 (LCD_BASE_0+(LCD_DX*LCD_DY))		// C007F800
#define LAYER_BASE(i) ((i)?(LCD_BASE_1):(LCD_BASE_0))

// homebrew variable font
typedef struct {
  short x;	// position dans l'image mere
  short y;
  short w;	// dimensions sprite
  short h;
} JVCHAR;

typedef struct {
  unsigned int base_adr;	// adresse data (ici bmp entiere)
  const JVCHAR * chardefs;	// tableau des dimensions des caracteres
  int ascii0;	// code ascii du premier char
  int qchar;
  int sx;	// char separation (hint)
  int dy;	// line spacing
} JVFONT;
   
// homebrew monospace font
typedef struct {    
  const unsigned short *rows;
  int w;
  int h;
  int dx;	// char pitch (hint)
  int dy;	// line spacing
} JFONT;

// utiliser cette option si les fontes ne sont pas deja flashees
// les fonts seront logees dans le secteur 0 avec l'appli
// #define COMPILE_THE_FONTS
// N.B. pour avoir assez d'espace dans le secteur 0, enlever USE_TRANSCRIPT dans le projet

// utiliser cette option (qui implique la precedente) pour flasher les fonts
// les fonts seront logees dans le secteur FLASH_FONTS_SECTOR @ FLASH_FONTS_BASE
// #define FLASH_THE_FONTS

// sans ces options les fonts sont supposees presentes @ FLASH_FONTS_BASE

#ifdef FLASH_THE_FONTS
#define COMPILE_THE_FONTS
#endif

#define FLASH_FONTS_BASE	0x08018000	// sector 3
#define FLASH_FONTS_SECTOR	3		// pour l'effacement
#define QCHAR	95	// nombre de caracteres dans chaque font


// le contexte graphique
typedef struct {    
int ilayer;
volatile unsigned int * fb_base;	// en mode ARGB8888
const JFONT * font;
const JVFONT * vfont;
int bg_color;
int text_color;
int fill_color;
int line_color;
int ytop;	// ymin pour clip (inclusif)
int ybot;	// ymax pour clip (exclusif)
volatile unsigned int ltdc_irq_cnt;	// frame counter
} GCtype;

// constructeur
void GC_init(void);

// contexte graphic global
extern GCtype GC;
#ifdef COMPILE_THE_FONTS
extern JFONT JFont24;
extern JFONT JFont20;
extern JFONT JFont16;
extern JFONT JFont16n;
extern JFONT JFont12;
extern JFONT JFont8;
#else
const extern JFONT JFont24;
const extern JFONT JFont20;
const extern JFONT JFont16;
const extern JFONT JFont16n;
const extern JFONT JFont12;
const extern JFONT JFont8;
#endif

const extern JVFONT JVFont36n;
const extern JVFONT JVFont26s;
const extern JVFONT JVFont19n;

// init GPIO for LCD
void jlcd_gpio1( void );
void jlcd_gpio2( void );
void jlcd_sdram_init( void );
// init LCD, panel
void jlcd_init(void);
// init LCD interrupt
void jlcd_interrupt_on( void );
// switch panel on
void jlcd_panel_on( void );
// switch panel on
void jlcd_panel_off( void );
// read panel state
int jlcd_is_panel_on( void );

// ---------------------- acces direct LTC --------------------------

void jlcd_reload_shadows(void);
void jlcd_reload_shadows_and_wait(void);
void jlcd_layer_enable( int layer );
void jlcd_layer_disable( int layer );
void jlcd_layer_alpha( int layer, int alpha );

// ---------------------- acces frame buffer selon GC --------------------------
// pour fonctionnement en double-buffer. GC pointe sur le layer en cours d'ecriture

// permuter les buffers dans le contexte double-buffering
// prendra effet avec jlcd_reload_shadows() ou jlcd_reload_shadows_and_wait()
void swap_layer( void );

// fill background en ARGB8888 (bloquant)
void jlcd_bg_fill( void );
// fill d'un rectangle en ARGB8888 (bloquant)
void jlcd_rect_fill( unsigned int x, unsigned int y, unsigned int w, unsigned int h );
// ligne horizontale en ARGB8888 (bloquant)
void jlcd_hline( unsigned int x, unsigned int y, unsigned int w );
// ligne verticale en ARGB8888 (bloquant)
void jlcd_vline( unsigned int x, unsigned int y, unsigned int h );

// trace de char a fond transparent base sur une font JLN
void jlcd_char( unsigned int x, unsigned int y, int ascii );
// trace de texte a fond transparent base sur une font JLN
void jlcd_text( int x, int y, const char * tbuf );
// trace de char opaque base sur une font JLN variable, rend sa largeur effective
int jlcd_vchar( unsigned int x, unsigned int y, int ascii );
// trace de texte opaque base sur une font JLN variable, rend x final
int jlcd_vtext( int x, int y, const char * tbuf );
// mesure de texte opaque base sur une font JLN variable, rend largeur
int jlcd_vtext_dx( const char * tbuf );

// extraire et afficher l'image d'un fichier BMP RGB present en memoire
int jlcd_draw_bmp( unsigned int x, unsigned int y, unsigned char *pbmp );
// extraire et afficher une zone rectangulaire prise dans un fichier BMP RGB present en memoire
int jlcd_blit_bmp( unsigned int xd, unsigned int yd, unsigned int xs, unsigned int ys,
		   unsigned int w, unsigned int h, unsigned char *pbmp );

// tracer une ligne oblique
void jlcd_line( unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2 );

// fonctions avec clip en y (supportent meme y < 0 )

// trace de char a fond transparent base sur une font JLN avec clip en y
void jlcd_yclip_char( int x, int y, int ascii );
// trace de texte a fond transparent base sur une font JLN avec clip en y
void jlcd_yclip_text( int x, int y, const char * tbuf );
// fill d'un rectangle en ARGB8888 (bloquant) avec clip y
void jlcd_yclip_rect_fill( int x, int y, int w, int h );
// ligne horizontale en ARGB8888 (bloquant) avec clip y
void jlcd_yclip_hline( int x, int y, int w );
// ligne verticale en ARGB8888 (bloquant) avec clip y
void jlcd_yclip_vline( int x, int y, int h );

// tracer une fleche vers la droite pour index, clip y, GC.line_color
void draw_r_arrow( int x, int y, int w, int h );

// texte centre, clip y
void draw_centered_text( int x, int y, int len, const char * txt );

#ifdef FLASH_THE_FONTS
unsigned int flash_the_fonts(void);
unsigned int check_the_fonts(void);
#endif

// ------------------------- quelques couleurs sur 32 bits -----------------------
#define ARGB_BLUE          ((unsigned int)0xFF0000FF)
#define ARGB_GREEN         ((unsigned int)0xFF00FF00)
#define ARGB_RED           ((unsigned int)0xFFFF0000)
#define ARGB_CYAN          ((unsigned int)0xFF00FFFF)
#define ARGB_MAGENTA       ((unsigned int)0xFFFF00FF)
#define ARGB_YELLOW        ((unsigned int)0xFFFFFF00)
#define ARGB_LIGHTBLUE     ((unsigned int)0xFF8080FF)
#define ARGB_LIGHTGREEN    ((unsigned int)0xFF80FF80)
#define ARGB_LIGHTRED      ((unsigned int)0xFFFF8080)
#define ARGB_LIGHTCYAN     ((unsigned int)0xFF80FFFF)
#define ARGB_LIGHTMAGENTA  ((unsigned int)0xFFFF80FF)
#define ARGB_LIGHTYELLOW   ((unsigned int)0xFFFFFF80)
#define ARGB_DARKBLUE      ((unsigned int)0xFF000080)
#define ARGB_DARKGREEN     ((unsigned int)0xFF008000)
#define ARGB_DARKRED       ((unsigned int)0xFF800000)
#define ARGB_DARKCYAN      ((unsigned int)0xFF008080)
#define ARGB_DARKMAGENTA   ((unsigned int)0xFF800080)
#define ARGB_DARKYELLOW    ((unsigned int)0xFF808000)
#define ARGB_WHITE         ((unsigned int)0xFFFFFFFF)
#define ARGB_LIGHTGRAY     ((unsigned int)0xFFD3D3D3)
#define ARGB_GRAY          ((unsigned int)0xFF808080)
#define ARGB_DARKGRAY      ((unsigned int)0xFF404040)
#define ARGB_BLACK         ((unsigned int)0xFF000000)
#define ARGB_BROWN         ((unsigned int)0xFFA52A2A)
#define ARGB_ORANGE        ((unsigned int)0xFFFFA500)
#define ARGB_TRANSPARENT   ((unsigned int)0x00000000)
