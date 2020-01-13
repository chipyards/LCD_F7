#include <stdio.h>

#include <windows.h>			/* must include this before GL/gl.h */
#include <GL/gl.h>			/* OpenGL header file */
#include "model.h"

/* ===================== base de donnees globale ======================== */

flat_tex_quad flat;

/* ============= texture from BMP file ======================= */

#include "bmp_.h"
#include <fcntl.h>
#include <io.h>

void gasp( char *fmt, ... );  /* fatal error handling */


void read_bmp_rgba( flat_tex_quad * f, const char * fnam )
{
int x, y, i; unsigned char *linbuf, *p;
impars s;
s.hand = open( fnam, O_RDONLY | O_BINARY );       /*DOS*/
if ( s.hand <= 0 )  gasp("%s non ouvert\n", fnam );

BMPreadHeader24( &s );
f->tW = s.wi; f->tH = s.he;
f->rgbabuf = malloc( f->tW * f->tH * 4 );
if ( f->rgbabuf == NULL )
   gasp("erreur malloc");
linbuf = malloc( s.ll );
if ( linbuf == NULL )
   gasp("erreur malloc");
/* Bien que les textures GL aient Y min en bas, comme BMP, on choisit de charger
   la BMP dans l'ordre style ecran ou GTK pour compatibilite avec autres traitements
 */
for ( y = ( s.he - 1 ); y >= 0 ; y-- )
    {
    read( s.hand, linbuf, s.ll );
    i = 0; p = f->rgbabuf + y * 4 * s.wi;
    for ( x = 0; x < s.wi; x++ )
        {
        *(p++) = linbuf[i+2];  /* R */
        *(p++) = linbuf[i+1];  /* G */
        *(p++) = linbuf[i];    /* B */
        *(p++) = 255;          /* A */
        i += 3;
        }
    }
close( s.hand ); free( linbuf );
}

void save_crop_bmp_rgba( flat_tex_quad * f, const char * fnam, int x, int y, int w, int h )
{
unsigned char *linbuf, *p;
impars d;
int i, xx, yy;
if	( ( x + w ) > f->tW )
	gasp("crop : depassement horizontal");
if	( ( y + h ) > f->tH )
	gasp("crop : depassement vertical");

d.hand = open( fnam, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666 );
if ( d.hand <= 0 )  gasp("%s non ouvert\n", fnam );

d.wi = w; d.he = h;
BMPwriteHeader24( &d );

linbuf = malloc( d.ll );
if ( linbuf == NULL )
   gasp("erreur malloc");

for	( yy = ( y + h - 1 ); yy >= y ; yy-- )
	{
	i = 0;
	p = f->rgbabuf + ( ( ( yy * f->tW ) + x ) * 4 );
	for	( xx = 0; xx < w; xx++ )
		{
		linbuf[i+2] = *(p++);  /* R */
		linbuf[i+1] = *(p++);  /* G */
		linbuf[i]   = *(p++);  /* B */
		++p;		       /* A */
		i += 3;
		}
	if	( write( d.hand, linbuf, d.ll ) != d.ll )
		gasp("erreur ecriture fichier");
	}
close( d.hand ); free( linbuf );
}

/* ================== creation texture ===================== */

/* un panneau de 1 quad

	a0---a3
	|     |
	a1---a2

   il a les dimensions de la texture,
   ses vertices sont en coordonnees ecran au zoom pres, Y croissant vers le bas
   son origine est dans le coin sup gauche
 */
void init_quad( flat_tex_quad * f )
{
int i=0; int wmax, tstack;
f->xyz3[i++] = 0.0f;           f->xyz3[i++] = 0.0f;           f->xyz3[i++] = 0.0f;
f->xyz3[i++] = 0.0f;           f->xyz3[i++] = (float)(f->tH); f->xyz3[i++] = 0.0f;
f->xyz3[i++] = (float)(f->tW); f->xyz3[i++] = (float)(f->tH); f->xyz3[i++] = 0.0f;
f->xyz3[i++] = (float)(f->tW); f->xyz3[i++] = 0.0f;           f->xyz3[i++] = 0.0f;
i = 0;
f->st2[i++] = 0.0f; f->st2[i++] = 0.0f;
f->st2[i++] = 0.0f; f->st2[i++] = 1.0f;
f->st2[i++] = 1.0f; f->st2[i++] = 1.0f;
f->st2[i++] = 1.0f; f->st2[i++] = 0.0f;
i = 0;
f->vx[i++] = 0;
f->vx[i++] = 1;
f->vx[i++] = 2;
f->vx[i++] = 3;

f->zf =  1;

glGetIntegerv( GL_MAX_TEXTURE_SIZE, &wmax );
glGetIntegerv( GL_MAX_TEXTURE_STACK_DEPTH, &tstack );
printf("texture : wmax=%d, stack depth=%d\n", wmax, tstack );

glGenTextures(1, &f->texHand);
glBindTexture(GL_TEXTURE_2D, f->texHand);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
set_quad_filter( 0 );

glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, f->tW, f->tH,
             0, GL_RGBA, GL_UNSIGNED_BYTE, f->rgbabuf );
}

// autorise resampling lineaire
void set_quad_filter( int enable )
{
if	( enable )
	enable = GL_LINEAR;
else	enable = GL_NEAREST;
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, enable );
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, enable );
}

// trace du quad, x et y  en coordonnees ecran
void aff_quad( flat_tex_quad * f, int x, int y )
{
// couleur de fond au cas ou la texture ne couvrirait pas tout
glClearColor( 0.1f, 0.1f, 0.4f, 1.0 );
glClear(GL_COLOR_BUFFER_BIT);

// glMatrixMode(GL_MODELVIEW);
// glLoadIdentity();

glPushMatrix();

glTranslatef( -1.0f, 1.0f, 0.0f );
glScalef( ( 2.0f / (float)f->vW ), ( -2.0f / (float)f->vH ), 1.0f );
// on est en coord. ecran, on peut translater
glTranslatef( (float)x, (float)y, 0.0f );
// alors on applique le zoom
glScalef( (float)f->zf, (float)f->zf, 1.0 );

// requis car on enable pas lighting (je suppose)
glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

glEnable(GL_TEXTURE_2D);
glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
glBindTexture(GL_TEXTURE_2D, f->texHand );

glEnableClientState( GL_VERTEX_ARRAY );
glEnableClientState( GL_TEXTURE_COORD_ARRAY );
glVertexPointer( 3, GL_FLOAT, 0, f->xyz3 );
glTexCoordPointer( 2, GL_FLOAT, 0, f->st2 );
glDrawElements( GL_QUADS, 4, GL_UNSIGNED_INT, f->vx );

glPopMatrix();
// IMPORTANT pour permettre le trace d'elements non textures a la
// prochaine frame .. (par exemple le repere )
glDisable(GL_TEXTURE_2D);
}

// trace d'un rectangle de selection en coordonnees ecran
void aff_transp_rect( flat_tex_quad * f, int x0, int y0, int x1, int y1, int color )
{
// inversion axe y
//y0 = f->vH - 1 - y0;
//y1 = f->vH - 1 - y1;
// maintenant on le fait en amenageant les matrices translate et scale
glPushMatrix();
glTranslatef( -1.0f, 1.0f, 0.0f );
glScalef( ( 2.0f / (float)f->vW ), ( -2.0f / (float)f->vH ), 1.0f );
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
unsigned char * rgba = (unsigned char *)&color;
glColor4f( ((float)rgba[0])/255.0, ((float)rgba[1])/255.0,
	   ((float)rgba[2])/255.0, ((float)rgba[3])/255.0 );
glBegin(GL_QUADS);
glVertex2f( (float)x0, (float)y0 );
glVertex2f( (float)x0, (float)y1 );
glVertex2f( (float)x1, (float)y1 );
glVertex2f( (float)x1, (float)y0 );
glEnd();
glDisable (GL_BLEND);
glPopMatrix();
}

// fonction interpretant un resize de la fenetre
void config_viewport( flat_tex_quad * f, int width, int height )
{
f->vW = width; f->vH = height;
/* 2D tout plat */
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glViewport(0, 0, f->vW, f->vH); /* viewport size in pixels */
}

// fin de la zone "pur openGL"

// zone "GL under windows"
// utilise des fonctions Win32 specialement faites pour opengl...

static HWND  hWnd;             // hWnd = window handle
static HDC   hDC;              // hDC = device context
static HGLRC hRC;              // hRC = opengl context

// preparation format pixel
void prepare_win32_pixel( HWND win32_win_handle )
{
    int         pf;
    PIXELFORMATDESCRIPTOR pfd;

    hWnd = win32_win_handle;
    hDC = GetDC(hWnd);   /* hDC = device context declare en global */

    /* on met d'abord tous les bits a zero, car peu seront non nuls */
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cDepthBits   = 32;
    pfd.cColorBits   = 32;

    pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0)
        gasp("ChoosePixelFormat() failed");

    if (SetPixelFormat(hDC, pf, &pfd) == FALSE)
        gasp("SetPixelFormat() failed");

    DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    hRC = wglCreateContext(hDC);  /* fonction wgl hybrides Win32/opengl */
    wglMakeCurrent(hDC, hRC);
}

// concretiser l'affichage de la frame
void swap_win32_buffers()
{
static PAINTSTRUCT ps;
glFlush();
SwapBuffers(hDC); /* hDC = device context declare en global */
BeginPaint(hWnd, &ps);
EndPaint(hWnd, &ps);
}

// eventuellement tout eteindre
void largue_win32_contexts()
{
    wglMakeCurrent(NULL, NULL);   /* fonction wgl hybrides Win32/opengl */
    ReleaseDC(hWnd, hDC);
    wglDeleteContext(hRC);
}


