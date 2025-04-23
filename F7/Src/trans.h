// N.B. ce module necessite le module logfifo

// marges, servent a donner une info visuelle qu'on est en haut ou en bas de la page
#define MTOP	10	// marge top en pixels
#define MBOT	32	// marge bottom en pixels

/* Le rendu visuel depend de 2 choses :
	* position de la page sur l'ecran = pilotage du scroll vertical (notamment par idrag)
	  Le transcript est une page largement plus haute que l'ecran,
	  transdraw( ypos ) en affiche une partie :
		* ypos = 0 : le top de la page est visible (N.B. il peut etre vide)
		* ypos = LCD_DY - trans.dy = yobjmin : le bas de la page est visible
	   ypos est normalement negatif (RAPPEL : LCD : y origine en haut, y croit vers le bas)
	* position du texte dans la page (virtuelle, seule la partie visible de la page existe)
 	  le texte reside dans le buffer circulaire logfifo
 	  ce que fait transdraw( ypos ) :
 		* calcul des index i0 et i1 determinant les lignes affichees relativement a la page (virtuelle)
 		  en fonction de ypos (NB si logfifo n'est pas rempli, on peut scroller sur une grande zone vide)
 		* calcul de l'index j dans le buffer circulaire pour chacune ligne entre i0 et i1
 		  en fonction de logfifo.wri.
 		  cas particulier : quand i1 = LFIFOQL (bas de la page), j = ( i1 + wri ) % LFIFOQL = wri
 		  la derniere ligne visible est i1-1 donc @ wri-1, la derniere entree dans le fifo
   Resume : si ypos = yobjmin, on voit les lignes les plus recentes de LOGFIFO, la page se remplit "par le bas"
   ce qui explique la magie: quand une nouvelle ligne apparait, ce n'est pas la page qui est scrollee
   (ypos ne change pas), mais le contenu de la page qui bouge par rotation de wri sur le buffer circulaire
 */
// le contexte transcript
typedef struct {
int x0;			// bord gauche en px
int dx;			// largeur en px
int dy;			// hauteur totale, depend de LFIFOQL defini par logfifo
int qcharvis;		// caracteres par ligne sur LCD
int qlinvis;		// nombre de lignes visibles
int last_ypos;		// pour gérer le gel du scroll
const JFONT * font;
} TRANStype;

// contexte global (singleton)
extern TRANStype trans;

// constructeur
int transcript_init( const JFONT * lafont, int x0, int dx );

// afficher le transcript
void transdraw( int ypos );
