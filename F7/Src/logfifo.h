/* fifo pour log vers uart ou transcript scrollable */
// buffer circulaire contenant des lignes de longueur fixe
// le contenu utile de chaque ligne est delimite par un '\0'
// les caractere de fin de ligne sont superflus et ignores

#define LFIFOLL		(1<<6)		// capacite d'une ligne en bytes
#define LFIFOQL		(1<<8)		// nombre de lignes
#define LFIFOQB		(LFIFOLL*LFIFOQL)	// capacite en bytes

// l'objet logfifo
typedef struct {
unsigned int wra;	// prochaine adresse a ecrire
unsigned int rda;	// prochaine adresse a lire
unsigned int wri;	// prochaine ligne a ecrire
char circ[LFIFOQB];	// buffer circulaire
} LOGtype;

// contexte global (singleton)
extern LOGtype logfifo;

// constructeur
void logfifo_init(void);

// ajouter une ligne de texte au transcript - sera tronquee si elle est trop longue
void LOGline( const char *txt );

// ajouter une ligne de texte formattee
void LOGprint( const char *fmt, ... );
