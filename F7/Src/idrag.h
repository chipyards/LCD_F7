
/* touch-drag params : base de temps 1 ms
#define MINDT		30	// duree min pour estimation vitesse (ms)
#define LOG_KVEL 	16	// log multiplicateur vitesse
#define LOG_TAU		5	// constante de temps (log ms)
#define MINV		(1<<(LOG_KVEL-5))	// 1/32 pix par iteration	
*/

/* touch-drag params : base de temps 16 ms (1 frame video) */
#define MINDT		2	// duree min pour estimation vitesse (en frames)
#define LOG_KVEL 	16	// log du multiplicateur kvel pour ameliorer la resolution du calcul : v = ( dy * kvel ) / dt
#define LOG_TAU		5	// constante de temps (log tau)
#define MINV		(1<<(LOG_KVEL-4))	// vitesse min (en dessous on stop) 1/16 pix par iteration
#define SQUELCHVY	400000	// vitesse en dessous de laquelle le drift est desactive
#define RUSHVY		700000	// vitesse au dessus de laquelle le rush est active (drift a la vitese de pointe du drag)
#define TNODRIFT	30	// duree (en frames) au dela de laquelle le drag a marque un arret (=> no drift)
#ifdef IDRAG_EXPERIMENTAL
#define VYSLEW		(1<<17)	// deceleration max
#endif

/* IDRAG controle le scroll vertical d'un objet ou "page"
   en faisant varier yobj entre les butees yobjmin et yobjmax.
   RAPPEL : LCD : y origine en haut, y croit vers le bas
	* normalement l'origine de la page est son top, alors :
		yobjmax = 0 <==> origine de la page au top de la fenetre
		yobjmin < 0
	* yobj < 0 : la page est scrollee vers le haut
	* yobj = yobjmin : le bas de la page est visible
   Les butees sont fixees par l'appli par ecriture directe dans la struct idrag
   en general yobjmin = LCD_DY - hauteur_de_la_page;
 */

// le contexte idrag
typedef struct {    
int anchy;	// ancrage du curseur sur l'objet en cours de dragage
int yobj;	// position de l'origine de l'objet
int yobjmin;	// yobj pour que le bas de la page soit visible
int yobjmax;	// yobj pour que le top de la page soit visible
int oldy;
int oldt;
int vy;		// vitesse
int peak_vy;	// vitesse de pointe observee
#ifdef IDRAG_EXPERIMENTAL
int max_acc;	// acceleration max observee
int max_dec;	// deceleration max observee
#endif
int min_dt;	// duree min pour mesure vitesse
int touching;	// pour detecter transition (land ou release)
int drifting;	// free running (erre)
// experiences
int land_t;
int rush;
int rush_vy;
int squelch;
int squelch_vy;
} IDRAGtype;

// constructeur
void idrag_init( void );

// contexte global
extern IDRAGtype idrag;

// traitement periodique, avec ou sans toucher
// si touch != 0 : x, y, t sont pris en compte
// si touch  = 0 : t est pris en compte
// rend yobj
int idrag_event_call( int touch, int x, int y, int t );
