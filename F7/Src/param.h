// une page de parametres ajustables


typedef struct {
const char * label;
int min;	// min inclus
int max;	// max exclu
int val;	// >= 0 car adju ne supporte pas les valeurs negatives (FIX ME)
int changed;
} PARAMitem;

typedef struct {
int x0;			// bord gauche en px
int dx;			// largeur en px
int pitch;		// pas vertical
int dy;			// hauteur totale
int last_ypos;		// a utiliser en cas de gel du scroll
const JFONT * font;
PARAMitem * items;
int qitem;
int xv;			// position de la valeur
int curitem;
int selitem;		// item selectionne
int editing;
} PARAMtype;

// contexte global : la page decrite en ROM
extern PARAMtype para;

// constructeur
void param_init( const JFONT * lafont, int x0, int dx );

// affichage
void param_draw( int ypos );

// interpretation d'un touch
void param_select( int ys );

// demarrage edition (2eme touch) rend une valeur de ypos pour idrag
int param_start(void);

// enregistrer la valeur du param en fin d'edition, et quitter le mode adj
void param_save(void);

// rend l'indice du premier param qui a change, -1 si aucun
int param_scan(void);

// rend la valeur du param, et le marque lu
int param_get_val( unsigned int i );

// fixe la valeur du param, et le marque lu
void param_set_val( unsigned int i, int val );

