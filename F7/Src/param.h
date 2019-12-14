// une page de parametres ajustables


typedef struct {
const char * label;
int min;
int max;
int val;
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

// enregistrer la valeur du param en fin d'edition
void param_save(void);
