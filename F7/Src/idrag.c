#include <stdlib.h>
#include "idrag.h"
#include "logfifo.h"	// for debug

// contexte global
IDRAGtype idrag;

// l'API n'expose qu'une methode (a part idrag_init() bien sur);
// la methode int idrag_event_call( int touch, int x, int y, int t );
// cette methode doit etre appelee periodiquement par la boucle principale du prog
// qui lui fournit couramment un timestamp en LCD frames

// ------------- les traitements d'evenements par type -----------
// methodes statiques, toutes appelees par idrag_event_call

// "land" c'est quand le doigt se pose sur la dalle, cela marque le debut d'un drag
// s'il y a un drift en cours cela l'interromp
static void event_land_call( int x, int y, int t )
{
idrag.vy = 0; idrag.drifting = 0;	// stopper mouvement inertiel
idrag.anchy = y - idrag.yobj;	// nouvel ancrage
idrag.oldy = y; idrag.oldt = t;	// references
idrag.land_t = t;
}

// drag en cours, on se contente de mesurer la vitesse
// ce qui cree un dilemme pour la détermination de ddt :
// - si la duree de mesure est courte, le dy quantifie au pixel sera bruite ou simplement nul
// - si la duree est longue, le drag peut etre trop bref pour produire une mesure
// actuellement la duree est quantifiee en frames, basee sur un compteur incrementee env. 60fps
// par l'interruption blanking (LTDC_IRQHandler() dans jlcd.c)
// - reduire la periode d'echantillonnage du touch (appels a BSP_TS_GetState() ) necessiterait de
//   connaitre la duree d'execution de cette fonction (mesurable) et aussi de savoir si le touch chip
//   FT5336 scanne plus vite que 60fps, ce qui n'est pas evident
// - ameliorer la resolution Y (->subpixel) est a investiguer - le FT5336 est capable d'envoyer 12 bits par coord.
//   comment le FT5336 a-t-il le bon facteur d'echelle pour fournir des XY en pixels reste un MYSTERE !
static void event_drag_call( int x, int y, int t )
{
int ddt, ddy;
idrag.yobj = y - idrag.anchy;	// poursuite
ddt = t - idrag.oldt;
if	( ddt > MINDT )	// estimation vitesse en vue release
	{
	ddy = y - idrag.oldy;
	// systeme "IDRAG_EXPERIMENTAL", distingue l'acceleration de la deceleration
	// le but premier etait d'eviter un freinage abrupt au lacher
	// un autre but etait de mesurer les ordres de grandeur des accelerations
	// le premier but n'est pas atteint car les freinages abrupts apparaissent quand
	// la duree de contact a ete trop breve pour permettre une mesure de vitesse
	// accelerations mesurees : de 16k (1<<16) a 2M (1<<21)
	#ifdef IDRAG_EXPERIMENTAL
	int newvy = ( ddy << LOG_KVEL ) / ddt;
	int dvy = newvy - idrag.vy;		// acceleration
	if	(			// acceleration dans le sens du mouvement ?
		( ( dvy >= 0 ) && ( idrag.vy >= 0 ) ) ||
		( ( dvy < 0 ) && ( idrag.vy < 0 ) )
		)			// on retient la nouvelle vitesse
		{
		idrag.vy = newvy;
		if	( idrag.max_acc < abs(dvy ) )
			idrag.max_acc = abs(dvy);	// noter acceleration record
		}
	else	{			// sinon c'est un freinage
		if	( idrag.max_dec < abs(dvy ) )
			idrag.max_dec = abs(dvy);	// noter deceleration record
		if	( dvy > VYSLEW )	// on limite la deceleration
			dvy = VYSLEW;
		if	( dvy < -VYSLEW )
			dvy = -VYSLEW;		// pour eviter un choc au lacher ?
		idrag.vy += dvy;
		}
	#else
	idrag.vy = ( ddy << LOG_KVEL ) / ddt;
	#endif
	if	( abs(idrag.peak_vy) < abs(idrag.vy ) )
		idrag.peak_vy = idrag.vy;	// vitesse de pointe signee
	idrag.oldy = y; idrag.oldt = t;
	}
}

// lacher : on initie le drifting, c'est tout
static void event_release_call( int t )
{
idrag.drifting = 1;
idrag.oldt = t;
#ifdef IDRAG_EXPERIMENTAL
LOGprint("vmax=%d amax=%d dmax=%d", idrag.peak_vy, idrag.max_acc, idrag.max_dec );
idrag.max_acc = idrag.max_dec = 0;
#endif
int tot_t = t - idrag.land_t;
// drag lent : pas de drift
if	( ( idrag.squelch ) && ( abs(idrag.peak_vy) < idrag.squelch_vy ) )
	idrag.vy = 0;
// arret sur drag : pas de drift
else if	( tot_t > TNODRIFT )
	idrag.vy = 0;
// drfit, possiblement a la vitese de pointe
else if	( ( idrag.rush ) && ( abs(idrag.peak_vy) > idrag.rush_vy ) )
	idrag.vy = idrag.peak_vy;	// retain peak vy instead of last vy
LOGprint("tot_t=%d, peak=%d", tot_t, idrag.peak_vy );
idrag.peak_vy = 0;
}

// drifting : on applique une deceleration exponentielle
static void event_drift_call( int t )
{
if	( idrag.drifting )
	{
	int ddt;
	ddt = t - idrag.oldt;
	idrag.oldt = t;
	idrag.yobj += ( ddt * idrag.vy ) >> LOG_KVEL;
	idrag.vy -= ( idrag.vy >> LOG_TAU );
	if	( abs(idrag.vy) < MINV )	// exponentielle ne finit jamais...
		{				// alors en dessous de MINV on arrete
		idrag.drifting = 0;
		}
	}
}

// ================= l'API ==========================

// constructeur
void idrag_init( void )
{
idrag.min_dt = MINDT;
idrag.drifting = 0;
idrag.touching = 0;
idrag.vy = 0;
idrag.yobj = 0;
// experiences
idrag.land_t = 0;
idrag.peak_vy = 0;
idrag.rush = 1;
idrag.rush_vy = 700000;
idrag.squelch = 1;
idrag.squelch_vy = 400000;
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
