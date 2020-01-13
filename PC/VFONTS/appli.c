#include <stdio.h>
#include <math.h>

#include <windows.h>		// pour MessageBox
#include "model.h"
#include "appli.h"



/* inevitables variables globales */

extern flat_tex_quad flat;

static int xobj, yobj;			// coin sup gauche du quad en coord ecran
static int xsel, ysel, wsel, hsel;	// rectangle selection en coordonnees objet

// fonte en cours d'élaboration
// la cellules 0 est speciale : elle determine la zone de travail
// donc englobe les autres
#define QCELL 128
vcell cell[QCELL] =	// cellules rectangulaires
{ // 1 cell :
{ 10, 10, 10, 10 },
};
int qcell = 1;		// nombre de cellules
int icsel;		// indice de la cellule selectionnee pour edition

/* ============================ TRAITEMENT ELABORATION FONT =============== */

// teste la blancheur d'une zone dans un buffer RVBA
#define MINWH	253	// valeur min pour qu'un pixel soit accepte comme blanc
int isblanche( unsigned char * rvba_buf, int x, int y, int w, int h, int stride )
{
int xx, yy, i;
for	( yy = y; yy < ( y + h ); ++yy )
	{
	for	( xx = x; xx < ( x + w ); ++xx )
		{
		i = ( ( stride * yy ) + xx ) * 4;
		if	( ( rvba_buf[i]   < MINWH ) ||	// R
 			  ( rvba_buf[i+1] < MINWH ) ||	// G
 			  ( rvba_buf[i+2] < MINWH )	// B
			) return 0;
		}
	}
return 1;
}

// scanne la texture globale une zone correspondant a la cellule 0
// pour la decouper en cellules contenant chacune un possible caractere
void horizontal_scan()
{
int x; int ic, isb, oldisb;
ic = 1; oldisb = 1;
for	( x = cell[0].x; x < ( cell[0].x + cell[0].w ); ++x )
	{
	isb = isblanche( flat.rgbabuf, x, cell[0].y, 1, cell[0].h, flat.tW );
	if	( ( isb == 0 ) && ( oldisb > 0 ) )
		{	// debut nouveau char
		printf("start %d\n", x );
		if	( ic >= QCELL )
			break;
		cell[ic].x = x;
		cell[ic].y = cell[0].y;
		cell[ic].h = cell[0].h;
		}
	if	( ( isb > 0 ) && ( oldisb == 0 ) )
		{	// fin char
		printf("end %d\n", x-1 );
		cell[ic].w = x - cell[ic].x;
		++ic;
		}
	oldisb = isb;
	}
if	( oldisb == 0 )
	{
	printf("end %d\n", x-1 );
	cell[ic].w = x - cell[ic].x;
	++ic;
	}
qcell = ic;
}

void save_cell_data()
{
int ic; FILE * fil; const char * fnam = "cell.dat";
fil = fopen( fnam, "w" );
if	( fil == NULL )
	return;
printf("saving %d cells in %s\n", qcell-1, fnam );
fprintf( fil, "// %d cells :\n{\n", qcell-1 );
for	( ic = 1; ic < qcell; ++ic )
	{
	fprintf( fil, "{ %d, %d, %d, %d },\n", cell[ic].x, cell[ic].y, cell[ic].w, cell[ic].h );
	}
fprintf( fil, "}\n" );
fclose( fil );
}

/* ============================== USER INTERFACE FUNCTIONS ================ */

// afficher un rectangle semi-transparent specifie en coord. objet, couleur 32 bits rgba
static void aff_sel_rect( int xsel, int ysel, int wsel, int hsel, int color )
{
int drx0, dry0, drx1, dry1;
if	( ( wsel <= 0 ) || ( hsel <= 0 ) )
	return;
drx0 = xobj + ( xsel * flat.zf );
dry0 = yobj + ( ysel * flat.zf );
drx1 = drx0 + ( wsel * flat.zf );
dry1 = dry0 + ( hsel * flat.zf );
aff_transp_rect( &flat, drx0, dry0, drx1, dry1, color );
}
// initialisations diverses
void aInit()
{
xobj = yobj = 1;
icsel = 0;
// preparer modele, y compris charger texture dans la RAM 3D
init_quad( &flat );
}

// affichage objet et rectangles de selection
void aDisplay()
{
aff_quad( &flat, xobj, yobj );
// afficher le rectangles de chaque cell
int ic, color;
for	( ic = 0; ic < qcell; ++ic )
	{
	if	( icsel == ic )
		color = 0x600000FF;
	else if	( ic == 0 )
		color = 0x40FF0000;
	else	color = 0x5000ffdd;
	aff_sel_rect( cell[ic].x, cell[ic].y, cell[ic].w, cell[ic].h, color );
	}
// ici on doit swapper les buffers, mais comme c'est dependant de l'OS
// on le laisse au niveau superieur
}

// dump zone selectionnee vers console
static void dumpzone()
{
printf("zone %2d : %3d %3d %3d %3d\n",
	icsel, cell[icsel].x, cell[icsel].y, cell[icsel].w, cell[icsel].h );
}

// traitement clavier
void aKey( short int c )
{
switch( c )
	{
	case 'n' : set_quad_filter( 0 ); break;
	case 'l' : set_quad_filter( 1 ); break;
	case 'i' :
		{
		char lbuf[64];
		sprintf( lbuf, "%d x %d", flat.vW, flat.vH );
		MessageBox(NULL, lbuf, "taille viewport", MB_OK);
		} break;
	// Zoom
	case 'z' : flat.zf *= 2; break;
	case 'o' : flat.zf /= 2; if ( flat.zf < 1 ) flat.zf = 1; break;
	case '1' : flat.zf = 1; break;
	case 'F' : flat.zf = 1; xobj = yobj = 0; break;
	// deplacement coin sup gauche de la selection : pave esdx
	case 'e' : --cell[icsel].y; ++cell[icsel].h; dumpzone();	break;
	case 's' : --cell[icsel].x; ++cell[icsel].w; dumpzone();	break;
	case 'd' : ++cell[icsel].x; --cell[icsel].w; dumpzone();	break;
	case 'x' : ++cell[icsel].y; --cell[icsel].h; dumpzone();	break;
	// deplacement coin inf droit de la ection : pave tfgv
	case 't' : --cell[icsel].h; dumpzone();		break;
	case 'f' : --cell[icsel].w; dumpzone();		break;
	case 'g' : ++cell[icsel].w; dumpzone();		break;
	case 'v' : ++cell[icsel].h; dumpzone();		break;
	// selection de zone a editer
	case '>' : if ( ++icsel >= qcell ) icsel = qcell - 1;	dumpzone(); break;
	case '<' : if ( --icsel < 0 ) icsel = 0;		dumpzone(); break;
	// action
	case 'H' : horizontal_scan(); break;
	case 'W' : save_cell_data(); break;
	case 'S' :
		save_crop_bmp_rgba( &flat, "crop.bmp", xsel, ysel, wsel, hsel );
		break;
	// default  :
	// printf("akey %04x\n", c & 0xFFFF );
	}
}

// traitement mouse
void aMouse( int x, int y, int curbou )
{
static int oldx, oldy;    /* point precedent */
static int oldbou = 0;

float val;
switch	( curbou )
	{
	case 0 : break;
	case 1 : if	( oldbou == 0 )
			{		// clic gauche : on memorise le point, c'est tout
			// printf("clic gauche\n");
			val = (float)( x - xobj ) / (float)flat.zf;
			cell[icsel].x = (int)round(val);
			val = (float)( y - yobj ) / (float)flat.zf;
			cell[icsel].y = (int)round(val);
			}
		else	{		// drag gauche : on trace le rectangle
			// printf("drag gauche\n");
			val = (float)( x - xobj ) / (float)flat.zf;
			cell[icsel].w = (int)round(val) - cell[icsel].x;
			val = (float)( y - yobj ) / (float)flat.zf;
			cell[icsel].h = (int)round(val) - cell[icsel].y;
			}
		break;
	case 3 : if	( oldbou == 0 )
			{		// clic droit : on memorise la position, c'est tout
			// printf("clic droit\n");
			oldx = x; oldy = y;
			}
		else	{		// drag droit : on deplace l'image en relatif
			int dx, dy;
			dx = x - oldx; dy = y - oldy;
			oldx = x; oldy = y;
			xobj += dx;
			yobj += dy;
			// printf("drag droit %d %d\n", xobj, yobj );
			}
		break;
   }
oldbou = curbou;
}


