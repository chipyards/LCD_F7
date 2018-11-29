// un adjustement
typedef struct {
const JFONT * font;
int x0;		// bord gauche en px
int y0;		// bord sup en px
int dx;		// largeur en px
int ty;		// amplitude translation verticale
int dys;	// hauteur visible
int val;
int min;	// valeur min incluse	
int max;	// valeur max exclue
int my;		// marge y
int mx;		// marge x
} ADJUtype;

// contexte global
extern ADJUtype adj;	// un seul, configurable a chaud

void adju_start( const JFONT * lafont, int x0, int y0, int w, int h, int min, int max );

// afficher l'ajustement, rend la valeur selectionnee
// ou -1 si aucune valeur selectionnee
int adju_draw( int ypos );

