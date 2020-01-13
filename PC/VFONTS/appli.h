// un caractere dans une fonte a largeur variable
typedef struct {
int x;
int y;
int w;
int h;
} vcell;


// scanne le rectangle selectionne global
void horizontal_scan();


// initialisations diverses
void aInit();

// affichage objet et eventuellement rectangles de selection
void aDisplay();

// traitement clavier
void aKey( short int c );

// traitement mouse
void aMouse( int x, int y, int curbou );
