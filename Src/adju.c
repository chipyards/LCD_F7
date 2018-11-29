
#include <stdio.h>
#include "jlcd.h"
#include "adju.h"

// contexte global
ADJUtype adj;

void adju_start( const JFONT * lafont, int x0, int y0, int w, int h, int min, int max )
{
adj.font = lafont;
adj.x0  = x0;
adj.y0  = y0;
adj.dx  = w;
adj.dys = h; 
adj.min = min;
adj.max = max;
adj.my  = h/2;
adj.mx  = 5;
adj.ty = ( adj.max - adj.min - 1 ) * adj.font->dy;
}

// afficher l'ajustement, rend la valeur selectionnee
// ou -1 si aucune valeur selectionnee
int adju_draw( int ypos )
{
int i, xs, ys, y0, y1;
char label[4];
// elements fixes
GC.fill_color = ARGB_LIGHTGRAY;	// fond
jlcd_rect_fill( adj.x0, adj.y0, adj.dx, adj.dys );
GC.line_color = ARGB_GRAY;	// index
jlcd_hline( adj.x0, adj.y0 + adj.my, adj.dx );

// elements scrollables : fonctions _yclip obligatoires
y0 = adj.y0 + adj.my - adj.font->dy + 3;	// top de zone select
y1 = adj.y0 + adj.my + 3;			// bottom de zone select
xs = adj.x0 + adj.mx;				// screen coord
ys = adj.y0 + adj.my + 3 - adj.font->dy/2 + ypos;
GC.text_color = ARGB_BLACK;
GC.font = adj.font;
GC.ytop = adj.y0;
GC.ybot = adj.y0 + adj.dys;
adj.val = -1;
for	( i = adj.min; i < adj.max; ++i )
	{
	if	( ( ys >= y0 ) && ( ys < y1 ) )
		{
		adj.val = i;
		GC.text_color = ARGB_RED;
		}
	else	GC.text_color = ARGB_BLACK;
	snprintf( label, sizeof(label), "%d", i );
	jlcd_yclip_text( xs, ys, label );
	ys += GC.font->dy;
	}
if	( adj.val < adj.min )
	adj.val = adj.min;
if	( adj.val >= adj.max )
	adj.val = adj.max - 1;
return adj.val;
}
