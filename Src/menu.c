
#include <stdio.h>
#include "jlcd.h"
#include "menu.h"

// contexte global (menu unique pour le moment)
MENUtype menu;

// constructeur
void menu_init( const JFONT * lafont, int x0, int dx )
{
menu.x0 = x0;			// bord gauche en px
menu.dx = dx;			// largeur en px
menu.ty = 0;			// amplitude translation verticale
menu.font = lafont;
menu.qitem = 0;		// nombre d'items
}

// ajouter un item
void menu_add( int key, const char * label )
{
if	( menu.qitem == MENUQ )
	return;
menu.item[menu.qitem].key = key;
menu.item[menu.qitem].label = label;
++menu.qitem;
menu.ty = ( menu.qitem - 1 ) * menu.font->dy;
}

// afficher le menu, rend la clef de l'element selectionne
// ou -1 si aucun element selectionne
int menu_draw( int ypos )
{
int i, xs, ys, y0, y1, iselect;
menu.last_ypos = ypos;

// obligatoire pour toute page utilisant des fonctions _yclip
GC.ytop = 0;
GC.ybot = LCD_DY;

// element fixe
GC.fill_color = ARGB_DARKGRAY;
jlcd_rect_fill( menu.x0, 0, menu.dx, LCD_DY );
GC.line_color = ARGB_RED;
draw_r_arrow( menu.x0, MENUMY, MENUMX-2, 12 );


// elements scrollables : fonctions _yclip obligatoires
y0 = MENUMY - menu.font->dy + 5;	// top de zone select
y1 = MENUMY + 3;			// bottom de zone select
xs = menu.x0 + MENUMX;		// screen coord
ys = MENUMY + 3 - menu.font->dy/2 + ypos;
GC.font = menu.font;
iselect = -1;
for	( i = 0; i < menu.qitem; ++i )
	{
	if	( ( ys >= y0 ) && ( ys < y1 ) )
		{
		iselect = i;
		GC.text_color = ARGB_YELLOW;
		}
	else	GC.text_color = ARGB_GREEN;
	jlcd_yclip_text( xs, ys, menu.item[i].label );
	ys += GC.font->dy;
	}
if	( ( iselect >= 0 ) && ( iselect < menu.qitem ) )
	return menu.item[iselect].key;
else	return -1;
}
