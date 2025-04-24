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
//		min	max	val	chng
{"vol out",	0,	64,	41,	0 },
{"vol in L",	0,	52,	11,	0 },
{"vol in R",	0,	52,	11,	0 },
{"session #",	0,	30,	0,	0 },
{"rec on/off",	0,	9,	0,	0 }
};

// constructeur
void param_init( const JFONT * lafont, int x0, int dx )
{
para.qitem = sizeof(lesparams) / sizeof(PARAMitem);
para.font = lafont;
para.x0 = x0;
para.dx = dx;
para.items = lesparams;
int pitchoun = LCD_DY / ( 2 + para.qitem );		// occuper tout l'espace
if	( pitchoun < ( ( lafont->dy * 7 ) / 4 ) )	// si trop serre, on devra scroller
	pitchoun = ( lafont->dy * 7 ) / 4;		// i.e. espace minimum entre les lignes (arbitraire)
para.pitch = pitchoun;
para.dy = ( 2 + para.qitem ) * para.pitch; // marges = 1 pitch
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
para.selitem = ( ( ys + 5 - para.last_ypos ) / para.pitch ) - 1;
if	( para.selitem < 0 )
	para.selitem = 0;
if	( para.selitem >= para.qitem )
	para.selitem = para.qitem - 1; 
}

// enregistrer la valeur du param en fin d'edition, et quitter le mode adj
void param_save(void)
{
para.items[para.selitem].val = adj.val;
para.items[para.selitem].changed = 1;
para.editing = 0;
// para.selitem = -1;
}

// demarrage edition (2eme touch), appeler seulement quand para.editing == 0
// rend une valeur de ypos pour idrag
int param_start()
{
int is = para.selitem, ypos = 0;
if	( ( para.editing == 0 ) && ( is >= 0 ) && ( is < para.qitem ) )
	{
	ypos = adju_start( para.font, para.x0 + para.xv,
			para.last_ypos + ( para.pitch * ( 1 + is ) ) + ( para.font->h / 2 ), 44, 60,
			para.items[is].min, para.items[is].max, para.items[para.selitem].val );
	para.editing = 1;
	}
return ypos;
}

// rend l'indice du premier param qui a change, -1 si aucun ou si adj en cours
int param_scan(void)
{
if	( para.editing )
	return -1;
for	( int i = 0; i < para.qitem; ++i )
	{
	if	( para.items[i].changed )
		return i;
	}
return -1;
}

// rend la valeur du param, et le marque lu
int param_get_val( unsigned int i )
{
if	( i < para.qitem )
	{
	para.items[i].changed = 0;
	return para.items[i].val;
	}
return 0;
}

// fixe la valeur du param, et le marque lu
void param_set_val( unsigned int i, int val )
{
if	( ( i < para.qitem ) && ( para.editing == 0 ) )
	{
	para.items[i].changed = 0;
	if	( val < para.items[i].min )
		val = para.items[i].min;
	if	( val >= para.items[i].max )
		val = para.items[i].max - 1;
	para.items[i].val = val;
	}
}
#endif
