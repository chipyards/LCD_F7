
#define MENUQ	10		// nombre max d'items
#define MENUMX	26		// marge gauche
#define MENUMY	(LCD_DY/2)	// marge haut et bas

// un item
typedef struct {
int key;	// valeur que retournera le menu
const char * label;
} MENUITEMtype;

// un menu
typedef struct {
int x0;			// bord gauche en px
int dx;			// largeur en px
int ty;			// amplitude translation verticale
const JFONT * font;
int qitem;		// nombre d'items
MENUITEMtype item[MENUQ];
} MENUtype;

// contexte global
extern MENUtype menu;	// un seul pour le moment

// constructeur
void menu_init( const JFONT * lafont, int x0, int dx );

// ajouter un item
void menu_add( int key, const char * label );

// afficher le menu, rend la clef de l'element selectionne
int menu_draw( int ypos );
