/* scrollable transcript */
#include "options.h"
#ifdef USE_TRANSCRIPT

#include <stdio.h>
#include <stdarg.h>
#include "jlcd.h"
#include "trans.h"

// contexte global
TRANStype trans;

// constructeur - rend la hauteur totale de la page scrollable en px
int transcript_init( const JFONT * lafont, int x0, int dx )
{
int a;
trans.x0 = x0;
trans.dx = dx;
trans.linelen = dx / lafont->dx;	// caracteres imprimables par ligne
trans.qlin = TRANSQ / trans.linelen;	// nombre de lignes
trans.qlinvis = ( LCD_DY / lafont->dy ) + 1;	// nombre de lignes visibles
trans.jwri = 0;
trans.font = lafont;
trans.dy = MTOP + MBOT + trans.qlin * lafont->dy;
for	( a = 0; a < TRANSQ; a += trans.linelen )
	{
	trans.circ[a] = 0;	// lignes toutes vides
	}

transprint( "Transcript V%s", VERSION );
transprint( " %d bytes, %d wasted", TRANSQ, TRANSQ - ( trans.linelen * trans.qlin ) );
transprint( " height %d px", trans.dy );
transprint( " %d lines, pitch %d px", trans.qlin, trans.font->dy );
return ( trans.dy );
}

// contexte global

// ajouter une ligne de texte au transcript - sera tronquee si elle est trop longue
void transline( const char *txt )
{
int ali, aca;
char c;
ali = trans.jwri * trans.linelen;
for	( aca = 0; aca < trans.linelen; ++aca )
	{
	c = txt[aca];
	trans.circ[ali+aca] = c;
	if	( c == 0 )
		break;
	}
trans.jwri = ( trans.jwri + 1 ) % trans.qlin;
}

void transprint( const char *fmt, ... )
{
static char lbuf[FMTLEN];
va_list  argptr;
va_start( argptr, fmt );
vsnprintf( lbuf, sizeof(lbuf), fmt, argptr );
va_end( argptr );
transline( lbuf );
}

// afficher le transcript
void transdraw( int ypos )
{
int x, ys;	// XY screen relative
int i, i0, i1;	// indexes lignes dans la page scrollable
int j;		// index de ligne dans le buffer circulaire
int ali, aca;	// index dans trans.circ[a]
char c;

i0 = ( - ypos - MTOP ) / trans.font->dy;
if	( i0 < 0 )
	i0 = 0;
i1 = i0 + trans.qlinvis;
if	( i1 > trans.qlin )
	i1 = trans.qlin;

GC.ytop = 0;
GC.ybot = LCD_DY;
// element fixe
GC.fill_color = ARGB_BLACK;
jlcd_rect_fill( trans.x0, 0, trans.dx, LCD_DY );

// elements scrollables : fonctions _yclip obligatoires
ys = ypos + MTOP + ( i0 * trans.font->dy );
GC.text_color = ARGB_GREEN;
GC.font = trans.font;
for	( i = i0; i < i1; ++i )
	{
	j = ( i + trans.jwri ) % trans.qlin;
	ali = j * trans.linelen;
	// jlcd_yclip_text( 0, ys, trans.circ + a ); <-- on deroule ça
	if	(
		( ys < GC.ybot ) &&
		( ys >= ( GC.ytop - GC.font->h ) )
		)
		{
		x = trans.x0;
		for	( aca = 0; aca < trans.linelen; ++aca )
			{
			c = trans.circ[ali+aca];
			if	( c == 0 )
				break;
			jlcd_yclip_char( x, ys, c );
			x += GC.font->dx;
			}
		}
	ys += GC.font->dy;
	ali += trans.linelen;
	}
}
#endif
