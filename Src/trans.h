

#define TRANSQ	1024	// capacite en bytes
#define MTOP	10	// marge top en pixels
#define MBOT	50	// marge bottom en pixels
#define FMTLEN	64	// buffer pour transprint

// le contexte transcript
typedef struct {
int x0;			// bord gauche en px
int dx;			// largeur en px
int linelen;		// caracteres par ligne
int qlin;		// nombre de lignes
int qlinvis;		// nombre de lignes visibles, meme partiellement
int jwri;		// prochaine ligne a ecrire
const JFONT * font;
char circ[TRANSQ];	// buffer circulaire
} TRANStype;

// constructeur
int transcript_init( const JFONT * lafont, int x0, int dx );

// ajouter une ligne de texte au transcript - sera tronquee si elle est trop longue
void transline( const char *txt );

// ajouter une ligne de texte formattee
void transprint( const char *fmt, ... );

// afficher le transcript
void transdraw( int ypos );
