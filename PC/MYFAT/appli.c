/* ce programme analyse le master boot sector d'un volume
   ou d'un drive
   - tente de lire une table de partitions
   - si FAT, lit le root dir et eventuellement recupere
     un fichier (nom court 11 char, par exemple "EZHOST  GIF").
 */
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "diskio.h"
#include "myfat.h"


void ecrit( char *fnam, char *dbuf, int aecrire )
{
int desthand;
desthand = open( fnam, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666 );
if ( desthand <= 0 ) { fprintf( stderr, "%s non ouvert \n", fnam); exit(1); }
if ( write( desthand, dbuf, aecrire ) != aecrire )
   { fprintf( stderr, "erreur ecriture disque\n"); close(desthand); exit(1); }
close(desthand);
}


/*

nclu = ScanChain( clu );
printf("chain size %d clusters = %d bytes\n", nclu, 
       nclu * bps * SectorsPerCluster );
dbuf = CopyChain( clu, nclu );
ecrit( "saved", dbuf, ( (DIRENT *)(dirbuf + 32 * i) ) -> size );
return(0);
}
*/


int main( int argc, char *argv[] )
{
int retval;
char cdrive;
char devicename[32];

if ( argc < 2 )
   { printf("usage : appli drive_letter_or_phys_index_or_name <file>\n"); return(1); }

if   ( argv[1][1] == 0 )
     {
     cdrive = argv[1][0];
     if ( cdrive > 'Z' )
        cdrive -= ( 'a' - 'A' );
     if   ( cdrive >= 'A' )
          snprintf( devicename, sizeof(devicename), "\\\\.\\%c:", cdrive );
     else snprintf( devicename, sizeof(devicename), "\\\\.\\PhysicalDrive%c", cdrive );
     }
else snprintf( devicename, sizeof(devicename), "\\\\.\\%s", argv[1] );
printf("opening %s\n", devicename );

retval =  myfat_start_session( devicename );
if	( retval )
	{ printf("pas vu de FAT pour le moment, err %d\n", retval ); return(1); }

// alloc et lecture FAT entiere en RAM, retour 0 si Ok
retval = myfat_read_fat();	// appeler apres succes DumpBootSector()
if	( retval )
	{ printf("echec lecture FAT entiere, err %d\n", retval ); return(1); }

// alloc et lecture du root dir en RAM, retour 0 si Ok
retval = myfat_read_root_dir();
if	( retval )
	{ printf("echec lecture root dir, err %d\n", retval ); return(1); }

if	( argc >= 3 )
	{		// tenter de copier un fichier present dans le root dir
	unsigned int startcluster, size;
	int i = FindFile( argv[2], myfat->rootbuf, myfat->RootDirSectors * (myfat->bps/32), &startcluster, &size );
	if	( i < 0 )
		{ printf("\"%s\" non trouve dans root dir\n", argv[2] ); return(1); }
	printf("\"%s\" : start %u, %u bytes\n", argv[2], startcluster, size );
	retval = myfat_save_chain( startcluster, size, "saved" );
	if	( retval )
		{ printf("echec copie \"%s\" sur disque local, err %d\n", argv[2], retval ); return(1); }
	}

win32_disk_close();
return(0);
}
