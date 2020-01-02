/* fifo pour log vers uart ou transcript scrollable */

#include "options.h"
#ifdef USE_LOGFIFO

#include <stdio.h>
#include <stdarg.h>
#include "logfifo.h"
#ifdef USE_UART1
#include "uarts.h"
#endif

// contexte global (singleton)
LOGtype logfifo;

// constructeur
void logfifo_init(void)
{
int a;
logfifo.wra = 0;
logfifo.rda = 0;
logfifo.wri = 0;
for	( a = 0; a < LFIFOQB; a += LFIFOLL )
	{
	logfifo.circ[a] = 0;	// lignes toutes vides
	}
LOGprint( "LOG FIFO V%s", VERSION );
LOGprint( " %d bytes, %d lines", LFIFOQB, LFIFOQL );
}

// ajouter une ligne de texte au transcript - sera tronquee si elle est trop longue
void LOGline( const char *txt )
{
int ali, aca;
char c;
ali = logfifo.wri * LFIFOLL;			// debut ligne courante
for	( aca = 0; aca < LFIFOLL; ++aca )
	{
	c = txt[aca];
	if	( ( c >= ' ' ) || ( c == 0 ) )
		logfifo.circ[ali++] = c;
	if	( c == 0 )
		break;
	}
logfifo.wri = ( logfifo.wri + 1 ) % LFIFOQL;	// index prochaine ligne
logfifo.wra = logfifo.wri * LFIFOLL;		// debut prochaine ligne
#ifdef USE_UART1
UART1_TX_INT_enable();
#endif
}

void LOGprint( const char *fmt, ... )
{
char lbuf[LFIFOLL];
va_list  argptr;
va_start( argptr, fmt );
vsnprintf( lbuf, sizeof(lbuf), fmt, argptr );
va_end( argptr );
LOGline( lbuf );
}
#endif
