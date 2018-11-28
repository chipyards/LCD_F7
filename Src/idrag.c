#include <stdlib.h>
#include "idrag.h"

// contexte global
IDRAGtype idrag;

// ------------- les traitements d'evenements par type -----------

static void event_land_call( int x, int y, int t )
{
idrag.vy = 0; idrag.drifting = 0;	// stopper mouvement inertiel
idrag.anchy = y - idrag.yobj;	// nouvel ancrage
idrag.oldy = y; idrag.oldt = t;	// references
}

static void event_drag_call( int x, int y, int t )
{
int ddt, ddy;
idrag.yobj = y - idrag.anchy;	// poursuite
ddt = t - idrag.oldt;
if	( ddt > MINDT )	// estimation vitesse en vue release
	{
	ddy = y - idrag.oldy;
	#ifdef IDRAG_EXPERIMENTAL
	newvy = ( ddy << LOG_KVEL ) / ddt;
	dvy = newvy - idrag.vy;
	if	(
		( ( dvy >= 0 ) && ( idrag.vy >= 0 ) ) ||
		( ( dvy < 0 ) && ( idrag.vy < 0 ) )
		)
		idrag.vy = newvy;
	else	{
		if	( dvy > VYSLEW )
			dvy = VYSLEW;
		if	( dvy < -VYSLEW )
			dvy = -VYSLEW;
		idrag.vy += dvy;
		}
	#else
	idrag.vy = ( ddy << LOG_KVEL ) / ddt;
	#endif
	idrag.oldy = y; idrag.oldt = t;
	}
}

static void event_release_call( int t )
{
idrag.drifting = 1;
idrag.oldt = t;
}

static void event_drift_call( int t )
{
if	( idrag.drifting )
	{
	int ddt;
	ddt = t - idrag.oldt;
	idrag.oldt = t;
	idrag.yobj += ( ddt * idrag.vy ) >> LOG_KVEL;
	idrag.vy -= ( idrag.vy >> LOG_TAU );
	if	( abs(idrag.vy) < MINV )
		idrag.drifting = 0;
	}
}

// ================= l'API ==========================

// constructeur
void idrag_init( void )
{
idrag.drifting = 0;
idrag.touching = 0;
idrag.vy = 0;
idrag.yobj = 0;
}

// traitement periodique, avec ou sans toucher
// si touch != 0 : x, y, t sont pris en compte
// si touch  = 0 : t est pris en compte
int idrag_event_call( int touch, int x, int y, int t )
{
if	( touch )
	{
	if	( idrag.touching == 0 )
		{
		idrag.touching = 1;
		event_land_call( x, y, t );
		}
	else	event_drag_call( x, y, t );
	}
else	{
	if	( idrag.touching )
		{
		idrag.touching = 0;
		event_release_call( t );
		}
	else	event_drift_call( t );
	}
if	( idrag.yobj < idrag.yobjmin )
	{
	idrag.drifting = 0;
	idrag.yobj = idrag.yobjmin;
	}
if	( idrag.yobj > idrag.yobjmax )
	{
	idrag.drifting = 0;
	idrag.yobj = idrag.yobjmax;
	}
return idrag.yobj;
}
