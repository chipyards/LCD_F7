// ---------------------- SCROLLABLE AREA DEMO --------------------------
#include "options.h"
#ifdef USE_DEMO
#include <stdio.h>
#include "jlcd.h"
#include "demo.h"


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
