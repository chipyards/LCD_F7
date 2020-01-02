// N.B. ce module necessite le module logfifo

// mise en page
#define MTOP	10	// marge top en pixels
#define MBOT	50	// marge bottom en pixels

// le contexte transcript
typedef struct {
int x0;			// bord gauche en px
int dx;			// largeur en px
int dy;			// hauteur totale
int qcharvis;		// caracteres par ligne sur LCD
int qlinvis;		// nombre de lignes visibles, meme partiellement
int last_ypos;		// pour gérer le gel du scroll
const JFONT * font;
} TRANStype;

// contexte global (singleton)
extern TRANStype trans;

// constructeur
int transcript_init( const JFONT * lafont, int x0, int dx );

// afficher le transcript
void transdraw( int ypos );
