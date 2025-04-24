/* principe : c'est la valeur de ypos (geree a l'exterieur de ce widget) qui va determiner
   l'index de la valeur choisie, mis dans adj.val
   Calcul ypos --> val
	adju_draw() le fait au cours du trace, en retetnant val t.q  y0 <= ys < y1
	soit y1 - adj.font->dy <= y1 - adj.font->dy/2 + ypos + adj.font->dy * (val - adj.min) < y1 
	soit     -adj.font->dy <=     -adj.font->dy/2 + ypos + adj.font->dy * (val - adj.min) < 0
	soit     -1            <=     -1/2            + (ypos/adj.font->dy) + (val - adj.min) < 0
	soit	 -1/2          <=                     + (ypos/adj.font->dy) + (val - adj.min) < 1/2
	soit val = adj.min - ypos/adj.font->dy avec une tolerance de +- 1/2 (N.B. ypos <= 0)
   Calcul val --> ypos
	ypos = -adj.font->dy * (val - adj.min)
   Bornes pour ypos :
	idrag.yobjmax = 0
	idrag.yobjmin = - adj.ty	
*/
#include <stdio.h>
#include "jlcd.h"
#include "adju.h"

// contexte global
ADJUtype adj;

// rend une valeur de ypos initiale pour idrag
// y0 = position reticule (au mileu du widget) comme cela il reste centre si on change sa hauteur th
// tw et th en caracteres
int adju_start( const JFONT * lafont, int x0, int y0, int tw, int th, int min, int max, int val )
{
adj.font = lafont;
adj.mx  = 5;
adj.dx  = adj.mx + adj.font->dx * tw;
adj.dys = adj.font->dy * th;
adj.my  = adj.dys/2;
adj.x0  = x0;
adj.y0  = y0 - adj.my;	// centrage
adj.min = min;
adj.max = max;
adj.ty = ( adj.max - adj.min - 1 ) * adj.font->dy;
if	( val < adj.min )
	val = adj.min;
if	( val >= adj.max )
	val = adj.max - 1;
adj.val = val;
return( -adj.font->dy * (val - adj.min) );
}

// affiche l'ajustement, retient dans adj.val la valeur selectionnee
// laisse la valeur anterieure si aucune valeur selectionnee
void adju_draw( int ypos )
{
int i, xs, ys, y0, y1, val;
char label[8];
GC.ytop = adj.y0;
if	( GC.ytop < 0 ) GC.ytop = 0;
if	( GC.ytop > LCD_DY ) GC.ytop = LCD_DY;
GC.ybot = adj.y0 + adj.dys;
if	( GC.ybot < 0 ) GC.ybot = 0;
if	( GC.ybot > LCD_DY ) GC.ybot = LCD_DY;

// elements fixes.. dans une page scrollable ! fonctions _yclip obligatoires
GC.fill_color = ARGB_LIGHTCYAN;	// fond
jlcd_yclip_rect_fill( adj.x0, adj.y0, adj.dx, adj.dys );
GC.line_color = ARGB_GRAY;	// index
jlcd_yclip_hline( adj.x0, adj.y0 + adj.my, adj.dx );

// elements scrollables : fonctions _yclip obligatoires
y1 = adj.y0 + adj.my + 3;		// bottom de zone select
y0 = y1 - adj.font->dy;			// top de zone select
ys = y1 - adj.font->dy/2 + ypos;	// screen coord
xs = adj.x0 + adj.mx;
GC.text_color = ARGB_BLACK;
GC.font = adj.font;
val = adj.val;	// valeur anterieure
for	( i = adj.min; i < adj.max; ++i )
	{
	if	( ( ys >= y0 ) && ( ys < y1 ) )
		{
		val = i;
		if	( val < adj.min )
			val = adj.min;
		if	( val >= adj.max )
			val = adj.max - 1;
		adj.val = val;
		GC.text_color = ARGB_RED;
		}
	else	GC.text_color = ARGB_BLACK;
	snprintf( label, sizeof(label), "%d", i );
	jlcd_yclip_text( xs, ys, label );
	ys += GC.font->dy;
	}
}
