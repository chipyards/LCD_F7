/* scrollable transcript */
// N.B. ce module necessite le module logfifo

#include "options.h"
#ifdef USE_TRANSCRIPT

#include "jlcd.h"
#include "logfifo.h"
#include "trans.h"
#include "uarts.h"

// contexte global (singleton)
TRANStype trans;

// constructeur - rend la hauteur totale de la page scrollable en px
int transcript_init( const JFONT * lafont, int x0, int dx )
{
trans.x0 = x0;
trans.dx = dx;
trans.qcharvis = dx / lafont->dx;	// caracteres imprimables par ligne
trans.qlinvis = LCD_DY / lafont->dy;	// nombre de lignes visibles
trans.font = lafont;
trans.dy = MTOP + MBOT + LFIFOQL * lafont->dy;
trans.last_ypos = 0;
logfifo_init();
LOGprint( "Transcript V%s", VERSION );
// transprint( " height %dpx, pitch %dpx", trans.dy, trans.font->dy );
LOGprint( " visible width %d chars", trans.qcharvis );
LOGprint( " visible height %d lines", trans.qlinvis );
return ( trans.dy );
}

// afficher le transcript
void transdraw( int ypos )
{
int x, ys;	// XY screen relative
int i, i0, i1;	// indexes lignes dans la page scrollable
int j;		// index de ligne dans le buffer circulaire
int ali, aca;	// index dans trans.circ[a]
char c;

trans.last_ypos = ypos;

i0 = ( - ypos - MTOP ) / trans.font->dy;
if	( i0 < 0 )
	i0 = 0;
i1 = i0 + trans.qlinvis + 1; // + 1 pour inclure les lignes partiellement visibles
if	( i1 > LFIFOQL )
	i1 = LFIFOQL;

// elucidation de la magie...
/// CDC_print("tdw(%d) %d %d %d %d\n", ypos, i0, i1, ( i0 + logfifo.wri ) % LFIFOQL, ( i1 + logfifo.wri ) % LFIFOQL );

// obligatoire pour toute page utilisant des fonctions _yclip
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
	// explication : avec i = 0 (top de la page ) on a j = logfifo.wri, soit la PROCHAINE ligne a ecrire dans le fifo
	// donc aussi la plus ancienne si le fifo est rempli
	// alors avec i = ( LFIFOQL - 1 ), tout en bas de la page, on est a l'autre bout du fifo donc sur la PLUS RECENTE ligne
	j = ( i + logfifo.wri ) % LFIFOQL;
	ali = j * LFIFOLL;
	// jlcd_yclip_text( 0, ys, trans.circ + a ); <-- on deroule ça
	if	(
		( ys < GC.ybot ) &&
		( ys >= ( GC.ytop - GC.font->h ) )
		)
		{
		x = trans.x0;
		for	( aca = 0; aca < trans.qcharvis; ++aca )
			{
			c = logfifo.circ[ali+aca];
			if	( c == 0 )
				break;
			jlcd_yclip_char( x, ys, c );
			x += GC.font->dx;
			}
		}
	ys += GC.font->dy;
	// ali += LFIFOLL;
	}
}
#endif
