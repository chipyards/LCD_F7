/* une page de parametres ajustables */
#include "options.h"
#ifdef USE_PARAM

#include <stdio.h>
#include "jlcd.h"
#include "adju.h"
#include "param.h"

// contexte global : la page decrite en ROM
PARAMtype para;

PARAMitem lesparams[] = {
{"byte",	0,	256,	0 },
{"dizaine",	0,	10,	0 },
{"douzaine",	1,	13,	1 },
{"centaine",	0,	100,	0 },
{"bool",	0,	2,	0 },
{"byte 2",	0,	256,	0 },
{"byte 3",	0,	256,	0 },
{"byte 4",	0,	256,	0 },
{"byte 5",	0,	256,	0 },
};

// constructeur
void param_init( const JFONT * lafont, int x0, int dx )
{
para.x0 = x0;
para.dx = dx;
para.pitch = ( lafont->dy * 7 ) / 4;
para.items = lesparams;
para.qitem = sizeof(lesparams) / sizeof(PARAMitem);
para.dy = ( 2 + para.qitem ) * para.pitch; // marges = 1 pitch
para.font = lafont;
para.xv = para.dx - 5 * lafont->dx;
para.last_ypos = 0;
para.curitem = 0;
para.selitem = -1;
para.editing = 0;
}

// affichage
void param_draw( int ypos )
{
int xs, ys, i; char tbuf[8];
if	( para.editing == 0 )
	para.last_ypos = ypos;

// obligatoire pour toute page utilisant des fonctions _yclip
GC.ytop = 0;
GC.ybot = LCD_DY;

// element fixe
GC.fill_color = ARGB_WHITE;
jlcd_rect_fill( para.x0, 0, para.dx, LCD_DY );

// a partir d'ici, les elements scrollables
// fonctions _yclip obligatoires
GC.text_color = ARGB_BLACK;
GC.font = para.font;
xs = para.x0 + 2;
if	( para.editing )
	ys = para.last_ypos;
else	ys = ypos;
ys += para.pitch;
 
for	( i = 0; i < para.qitem; ++i )
	{
	if	( i == para.selitem )
		GC.text_color = ARGB_RED;
	else	GC.text_color = ARGB_BLACK;
	jlcd_yclip_text( xs, ys, para.items[i].label );
	snprintf( tbuf, sizeof(tbuf), "%d", para.items[i].val );
	jlcd_yclip_text( xs + para.xv, ys, tbuf );
	ys += para.pitch;
	}
// overlay
if	( para.editing )
	adju_draw( ypos );
}

// interpretation d'un touch
// note de calculs
//	ys = ypos + ( para.pitch * ( 1 + i ) )
// ==>	i = ( ( ys - ypos ) / para.pitch ) - 1
// ys est en haut des caracteres, l'arrondi par defaut de la division va accepter
// des y > ys donc en dessous de ys ==> ok
// on ajoute un petit offset a ys pour decaler la zone vers le haut
void param_select( int ys )
{
if	( ys < 0 )
	{
	if	( para.editing )
		{			// conclure edition
		para.editing = 0;
		para.items[para.selitem].val = adj.val;
		}
	para.selitem = -1;
	return;
	}
para.selitem = ( ( ys + 5 - para.last_ypos ) / para.pitch ) - 1;
}

// demarrage edition (2eme touch)
void param_start()
{
int is = para.selitem;
if	( ( is >= 0 ) && ( is < para.qitem ) )
	{
	adju_start( para.font, para.x0 + para.xv,
		    para.last_ypos + ( para.pitch * ( 1 + is ) ),
		    44, 50, para.items[is].min, para.items[is].max );
	para.editing = 1;
	}
}

#endif
