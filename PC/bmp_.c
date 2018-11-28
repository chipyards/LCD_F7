/* Fonctions pour lire ou ecrire d'un coup une image BMP RGB brute
   l'image RGB dans s->data contient :
   - les lignes en partant du bas
   - dans chaque ligne les pixels en partant de gauche,
     plus eventuellement des octets nuls pour arriver a un
     multiple de 4 bytes (s->ll)
   - pour chaque pixel trois octets B,G,R
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

#include "bmp_.h"

/* cette fonction lit une image BMP entiere en memoire
   elle prend en charge l'allocation si s->data est NULL,
   sinon elle copie les donnees dans s->data qui doit
   pointer sur un espace dont la capacite est s->tot.
*/

void readBMPfile( impars *s )
{
int totmem;

s->hand = open( s->fnam, O_RDONLY | O_BINARY );
if ( s->hand <= 0 )
   gasp("pb ouverture fichier %s en lecture", s->fnam );

totmem = s->tot; /* pour le cas s->data != NULL */

BMPreadHeader24( s );

if   ( s->data == NULL )
     {
     s->data = malloc( s->tot );
     if ( s->data == NULL )
        gasp("pb alloc memoire %d bytes", s->tot );
     }
else if ( totmem < s->tot )
        gasp("memoire disponible insuffisante pour %s", s->fnam );

totmem = read( s->hand, s->data, s->tot );
if ( totmem < s->tot )
   gasp("fin de fichier %s prematuree", s->fnam );
close( s->hand );
}

/* cette fonction ecrit une image BMP entiere
   on doit lui fournir :
     d->fnam
     d->wi
     d->he
     d->data
   elle calcule les autres parametres
*/

void writeBMPfile( impars *d )
{
int totmem;

umask(0);
d->hand = open( d->fnam, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666 );
if ( d->hand <= 0 )
   gasp("pb ouverture fichier %s en ecriture", d->fnam );

BMPwriteHeader24( d );
totmem = write( d->hand, d->data, d->tot );
if ( totmem < d->tot )
   gasp("pb ecriture %s (disque plein ?)", d->fnam );
close( d->hand );
}

/* traitement erreur fatale *
void gasp( char *fmt, ... )
{
  char lbuf[2048];
  va_list  argptr;
  va_start( argptr, fmt );
  vsprintf( lbuf, fmt, argptr );
  va_end( argptr );
  printf("STOP : %s\n", lbuf ); exit(1);
}
//*/

/* ces fonctions sont portables sur les architectures
   little-endian et big-endian */

void BMPreadHeader24( impars *s )
{
unsigned char buf[64];
read( s->hand, buf, 54 );
if (  ( buf[0] != 0x42 ) || ( buf[1] != 0x4D )  )
   gasp("not a BMP file");
s->wi = buf[18] + ( buf[19] << 8 );
s->he = buf[22] + ( buf[23] << 8 );
s->bppx = buf[28];
printf("%d bits par pixels, w=%d, l=%d\n", s->bppx, s->wi, s->he );
if   ( s->bppx != 24 ) gasp("only 24 bits/pix is supported");
s->hdlen = buf[14];
if ( s->hdlen != 40 ) gasp("header length != 40");
if ( buf[30] != 0 )  gasp("only uncompressed is supported");
s->tot = buf[34] + ( buf[35] << 8 ) + ( buf[36] << 16 ) + ( buf[37] << 24 );
printf("encombrement des pixels de l'image source = %d\n", s->tot);
s->ll = s->tot / s->he;
printf("encombrement d'une ligne du fichier source = %d\n", s->ll);
if ( s->tot != ( s->he * s->ll ) ) gasp("pb longueur totale");
}

void BMPwriteHeader24( impars *d )
{
unsigned char buf[64]; int i;
for ( i = 0; i < 54; i++ ) buf[i] = 0;

d->bppx = 24;
d->hdlen = 54;
d->ll = d->wi * 3;
i =  ( d->ll & 3 );        /* residu division par 4 */
if (i) d->ll += ( 4 - i ); /* maintenant ll est divisible par 4 */
d->tot = d->ll * d->he;

buf[0] = 0x42; buf[1] = 0x4D;
i = d->tot + d->hdlen;
buf[2] = i;
buf[3] = i >> 8;
buf[4] = i >> 16;
buf[5] = i >> 24;

buf[10] = d->hdlen;

buf[14] = d->hdlen - 14;

buf[18] = d->wi;
buf[19] = d->wi >> 8;
buf[22] = d->he;
buf[23] = d->he >> 8;
buf[26] = 1;
buf[28] = d->bppx;

buf[34] = (unsigned char) ( d->tot );
buf[35] = (unsigned char) ( d->tot >> 8 );
buf[36] = (unsigned char) ( d->tot >> 16 );
buf[37] = (unsigned char) ( d->tot >> 24 );
buf[38] = 0x6D; buf[39] = 0x0B;
buf[42] = 0x6D; buf[43] = 0x0B;
write( d->hand, buf, 54 );
}

