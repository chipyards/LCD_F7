// un adjustement
typedef struct {
const JFONT * font;
int x0;		// bord gauche en px
int y0;		// bord sup en px
int dx;		// largeur en px
int ty;		// amplitude translation verticale
int dys;	// hauteur visible
int val;	// derniere valeur observee
int min;	// valeur min incluse	
int max;	// valeur max exclue
int my;		// marge y
int mx;		// marge x
} ADJUtype;

// contexte global
extern ADJUtype adj;	// un seul, configurable a chaud

int adju_start( const JFONT * lafont, int x0, int y0, int w, int h, int min, int max, int val );

// afficher l'ajustement, rend la valeur selectionnee
// ou -1 si aucune valeur selectionnee
void adju_draw( int ypos );
