#ifndef IMPARS_TYPE
#define IMPARS_TYPE

typedef struct /* structure representant les parametres d'une image ... */
{
int wi, he; /* dimensions utiles en pixels */
int ll;     /* encombrement d'une ligne dans la base de donnees, en bytes */
int tot;   /* encombrement total des pixels */
int bppx;   /* bits par pixel */
int hdlen;  /* header length */
int maplen; /* colormap length (bytes) */
unsigned char *data; /* donnees brutes */
int hand;   /* descripteur de fichier */
char *fnam; /* nom de fichier */
} impars;

#endif

/* cette fonction lit une image BMP entiere en memoire
   elle prend en charge l'allocation si s->data est NULL,
   sinon elle copie les donnees dans s->data qui doit
   pointer sur un espace dont la capacite est s->tot.
*/

void readBMPfile( impars *s );

/* cette fonction ecrit une image BMP entiere
   on doit lui fournir :
     d->fnam
     d->wi
     d->he
     d->data
   elle calcule les autres parametres
*/

void writeBMPfile( impars *d );

void gasp( char *fmt, ... );  /* traitement erreur fatale */
void BMPreadHeader24( impars *s );
void BMPwriteHeader24( impars *d );
