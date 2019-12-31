
// buffer circulaire contenant des lignes de longueur fixe
#define TRANSLL		(1<<6)		// capacite d'une ligne en bytes
#define TRANSQL		(1<<8)		// nombre de lignes
#define TRANSQB		(TRANSLL*TRANSQL)	// capacite en bytes

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
int jwri;		// prochaine ligne a ecrire
int last_ypos;		// pour gérer le gel du scroll
const JFONT * font;
char circ[TRANSQB];	// buffer circulaire
} TRANStype;

// contexte global
extern TRANStype trans;

// constructeur
int transcript_init( const JFONT * lafont, int x0, int dx );

// ajouter une ligne de texte au transcript - sera tronquee si elle est trop longue
void transline( const char *txt );

// ajouter une ligne de texte formattee
void transprint( const char *fmt, ... );

// afficher le transcript
void transdraw( int ypos );
