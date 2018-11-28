// ---------------------- SCROLLABLE AREA DEMO --------------------------
#include "options.h"
#ifdef USE_DEMO
#include <stdio.h>
#include "jlcd.h"
#include "demo.h"

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

// rend une couleur selon code resistances
unsigned int resistocolor( int i )
{
switch	( i )
	{
	case 0 : return ARGB_BLACK;
	case 1 : return ARGB_DARKYELLOW;
	case 2 : return ARGB_RED;
	case 3 : return ARGB_ORANGE;
	case 4 : return ARGB_YELLOW;
	case 5 : return ARGB_GREEN;
	case 6 : return ARGB_BLUE;
	case 7 : return ARGB_DARKMAGENTA;
	case 8 : return ARGB_GRAY;
	case 9 : return ARGB_WHITE;
	}
return ARGB_CYAN;
}

// tracer une page de demo scrollable verticalement
void demo_draw( int ypos, int xs, int dx )
{
int ys;			// ordonnee ecran
int is;			// indice slot
int dy = 32;			// pour motif repetitif
char tbuf[40];
// tracer 2 ou 3 elements qui n'ont pas besoin de scroller
GC.fill_color = ARGB_WHITE;
jlcd_rect_fill( xs, 0, dx, LCD_DY );
GC.line_color = ARGB_BLACK;
jlcd_vline( xs, 0, LCD_DY );
// tracer l'index
ys = LCD_DY / 2;
GC.line_color = ARGB_RED;
draw_r_arrow( xs, ys, 24, 12 );


// a partir d'ici, les elements scrollables
// fonctions _yclip obligatoires
xs += 2;
GC.text_color = ARGB_BLACK;
ys = ypos + 4;
GC.font = &JFont24;
jlcd_yclip_text( xs, ys, " !\"#$%&'()*+,-./" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "0123456789" );		ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQ" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "RSTUVWXYZ" );		ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopq" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "rstuvwxyz :;" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "<=>?@[\\]^_`{|}~" );	ys+=GC.font->dy;

GC.font = &JFont20;
ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, " !\"#$%&'()*+,-./" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "0123456789" );		ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQ" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "RSTUVWXYZ" );		ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopq" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "rstuvwxyz :;" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "<=>?@[\\]^_`{|}~" );	ys+=GC.font->dy;

GC.font = &JFont16;
ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "!\"#$%&'()*+,-./0123456789" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopqrstuvwxyz" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, " :;^_`{|}<=>?@[\\]~" );	ys+=GC.font->dy;

GC.font = &JFont16n;
ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "!\"#$%&'()*+,-./0123456789" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopqrstuvwxyz" );	ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, " :;^_`{|}<=>?@[\\]~" );	ys+=GC.font->dy;

GC.font = &JFont12;
ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, " !\"#$%&'()*+,-./0123456789:;" );       ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{|}~" );   ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopqrstuvwxyz<=>?@[\\]" ); ys+=GC.font->dy;

GC.font = &JFont8;
ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, " !\"#$%&'()*+,-./0123456789:;" );       ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{|}~" );   ys+=GC.font->dy;
jlcd_yclip_text( xs, ys, "abcdefghijklmnopqrstuvwxyz<=>?@[\\]" ); ys+=GC.font->dy;
ys+=GC.font->dy;

GC.font = &JFont24;
is = 0;
while	( ys < ( LCD_DY + dy ) )
	{
	GC.fill_color = resistocolor( is % 10 );
	GC.text_color = GC.fill_color ^ 0x00FFFFFF;
	jlcd_yclip_rect_fill( xs, ys+1, dx-4, dy-2 );
	snprintf( tbuf, sizeof(tbuf), "%4d %4d", is, ys-ypos );
	jlcd_yclip_text( xs, ys+4, tbuf );
	ys += dy; ++is;
	}
}
#endif
