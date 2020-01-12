/* lib pour gestion compacte de la FAT sur une carte SD dediee appli embarquee
   compatible WIN32 (verifie) et STM32 (pas encore, structures a verifier)
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "myfat.h"

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
#include "diskio.h"

/** declarations -------------------------------------------------------- */

/* FAT 32 boot sector */
#pragma pack (1)
typedef struct{
  unsigned char Jmp[3];
  char OEM_Name[8];
  unsigned short bytesPerSector;
  unsigned char SectorsPerCluster;
  unsigned short ReservedSectors;
  unsigned char NumFATs;
  unsigned short NumRoot;
  unsigned short TotalSectors_16;
  unsigned char MediaDB;
  unsigned short SectorsPerFAT_16;
  unsigned short SectorsPerTrack;
  unsigned short NumHeads;
  unsigned int NumHidSect;
  unsigned int TotalSectors_32;
  /* start of specific FAT32 layout */
  unsigned int SectorsPerFAT_32;
  unsigned short Flags;
  unsigned short Version;
  unsigned int RootStartCluster;
  unsigned short FSInfoSec;
  unsigned short BkUpBootSec;
  unsigned char Reserved[12];
  unsigned char DrvNum;
  unsigned char Reserved1;
  unsigned char BootSig;
  unsigned char VolID[4];
  char VolLab[11];
  char FileSysType[8];
} FAT32boot; /* size of struct should be 90 */

/* The RootStartCluster is new to the boot record.
   This field contains a 32-bit starting cluster number
   of the root directory of the FAT32 drive. This is because
   the root directory is just another sub-directory in FAT32.
   There is also a backup boot sector in FAT32, which is
   denoted in BkUpBootSec.
 */

/* FAT 12/16 boot sector */
typedef struct{
  unsigned char Jmp[3];
  char OEM_Name[8];
  unsigned short bytesPerSector;
  unsigned char SectorsPerCluster;
  unsigned short ReservedSectors;
  unsigned char NumFATs;
  unsigned short NumRoot;
  unsigned short TotalSectors_16;
  unsigned char MediaDB;
  unsigned short SectorsPerFAT_16;
  unsigned short SectorsPerTrack;
  unsigned short NumHeads;
  unsigned int NumHidSect;
  unsigned int TotalSectors_32;
  /* start of specific FAT12/16 layout */
  unsigned char DrvNum;
  unsigned char Reserved1;
  unsigned char BootSig;
  unsigned char VolID[4];
  char VolLab[11];
  char FileSysType[8];
} FAT16boot; /* size of struct should be 62 */

/* The partition table contains up to 4 such records,
   it begins at offset 512-64-2 : */

typedef struct{
  unsigned char BootInd;
  unsigned char Head;
  unsigned char Sector;
  unsigned char Cylinder;
  unsigned char SysInd;
  unsigned char LastHead;
  unsigned char LastSector;
  unsigned char LastCylinder;
  unsigned int RelativeSector;
  unsigned int NumberSectors;
} PARTITION; /* size of struct should be 16 */


#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME 0x0F

typedef struct {
char name[8];
char ext[3];
unsigned char attr;
char reserved;
unsigned char ctime_frac;  /* creation */
unsigned short ctime;
unsigned short cdate;
unsigned short adate;      /* access */
unsigned short cluster_hi; /* only for FAT32 */
unsigned short mtime;      /* modification */
unsigned short mdate;
unsigned short cluster_lo;
unsigned int   size;
} DIRENT; /* size of struct should be 32 */

#pragma pack ()

/** variables globales ----------------------------------------------------- */

MYFAT zemyfat;

/** fonctions travaillant en RAM uniquement -------------------------------- */

/* cette fonction 
   - met a jour FATpartBase si partition FAT rencontree
   - met a jour EXTpartBaseF si partition etendue F rencontree
   - rend non zero si partition etendue rencontree
   - lit EXTpartBase
 */
int DumpPartTable( unsigned char * buf )
{
PARTITION * part[4]; int i, retval;
retval = 0;

buf += ( 512 - 64 - 2 );
part[0] = (PARTITION *)buf;
part[1] = part[0] + 1;
part[2] = part[1] + 1;
part[3] = part[2] + 1;
for	( i = 0; i < 4; i++ )
	{
	printf("partition %d : bootable : %s, type %02x, debut %x, taille %x (%d Mb)\n",
            i, (part[i]->BootInd)?("oui"):("non"), part[i]->SysInd, part[i]->RelativeSector,
            part[i]->NumberSectors, part[i]->NumberSectors / 2048 );

	/* l'offset de la partition FAT est relatif a la partition etendue courante */
	if	( part[i]->SysInd == 0xE )
		myfat->FATpartBase = part[i]->RelativeSector + myfat->EXTpartBase;

	/* l'offset de la partition etendue 5 est relatif a la partition etendue englobante */
	if	( ( part[i]->SysInd == 0x05 ) || ( part[i]->SysInd == 0x85 ) )
		retval = part[i]->RelativeSector + myfat->EXTpartBaseF;

	/* partition etendue F englobe les autres */
	if	( part[i]->SysInd == 0xF )
		{
		retval = part[i]->RelativeSector + myfat->EXTpartBase;
		myfat->EXTpartBaseF = retval;
		}
	}
return retval;
}

// analyse du boot sector
// retourne 1 si manque 55 AA, 2 si manque jmp, 3 si taille secteur illegale, 4 si no FAT
// 5 si taille FAT incoherente, 0 si parametre FAT lus Ok
int DumpBootSector( unsigned char * buf )
{
FAT32boot * b; FAT16boot * d;
int SectorsPerFAT2;

if ( ( buf[510] != 0x55 ) || ( buf[511] != 0xAA ) )
   { printf("manque signature 55 AA en fin de secteur...\n"); return(1); }
if ( ( buf[0] != 0xEB ) && ( buf[0] != 0xE9 ) )
   { printf("manque jump EB ou E9 en debut de secteur...\n"); return(2); }
b = (FAT32boot *) buf;
myfat->bps = b->bytesPerSector;
printf("taille secteur %d\n", myfat->bps );
if ( ( myfat->bps != 512 ) && ( myfat->bps != 1024 ) && ( myfat->bps != 2048 ) && ( myfat->bps != 4096 ) )
   { printf("taille secteur incompatible avec FAT...\n"); return(3); }
printf("nom OS : %-.8s\n", b->OEM_Name );
myfat->SectorsPerCluster = b->SectorsPerCluster;
printf("secteurs par cluster %d (cluster de %d bytes)\n",
        myfat->SectorsPerCluster, b->bytesPerSector * myfat->SectorsPerCluster );
printf("secteurs reserves %d\n", b->ReservedSectors );
printf("nombre de FATs %d\n", b->NumFATs );
printf("nombre d'entrees dans le root dir %d\n", b->NumRoot );

myfat->TotalSectors = b->TotalSectors_16;
if ( myfat->TotalSectors == 0 )
   myfat->TotalSectors = b->TotalSectors_32;
printf("nombre total secteurs %d\n", myfat->TotalSectors );
printf("Media(F8 ou F0) 0x%02X\n", b->MediaDB );

myfat->SectorsPerFAT = b->SectorsPerFAT_16;
if ( myfat->SectorsPerFAT == 0 )
   myfat->SectorsPerFAT = b->SectorsPerFAT_32;
printf("nombre de secteurs par FAT %d\n", myfat->SectorsPerFAT );
// printf("nombre de secteurs par piste %d, de tetes %d\n", b->SectorsPerTrack, b->NumHeads );
printf("nombre de secteurs caches %d\n", b->NumHidSect );
if ( b->NumFATs == 0 )
   { printf("Ce n'est probablement pas un systeme FAT...\n"); return(4); }
/* determinons quel type de FAT on a : (methode OFFICIELLE !) */
myfat->RootDirSectors = ( (b->NumRoot * 32) + (myfat->bps - 1) ) / myfat->bps; /* arrondi par exces */

myfat->FirstFATSector = b->ReservedSectors;
myfat->FirstDirSector = b->ReservedSectors + ( b->NumFATs * myfat->SectorsPerFAT );
myfat->FirstDataSector = myfat->FirstDirSector + myfat->RootDirSectors;
myfat->DataSectors = myfat->TotalSectors - myfat->FirstDataSector;

/* translatons ce qui est relatif a la base de la partition FAT */
myfat->FirstFATSector += myfat->FATpartBase;
myfat->FirstDirSector += myfat->FATpartBase;
myfat->FirstDataSector += myfat->FATpartBase;

myfat->DataClusters = myfat->DataSectors / b->SectorsPerCluster; 
if   ( myfat->DataClusters < 4085 )
     myfat->FAT = 12;  /* Volume is FAT12 */
else if   ( myfat->DataClusters < 65525 )
	  myfat->FAT = 16; /* Volume is FAT16 */
     else myfat->FAT = 32; /* Volume is FAT32 */

printf("d'apres le nombre de clusters %d, le volume est FAT %d\n", myfat->DataClusters, myfat->FAT );
/* note : le premier Data Cluster est le numero 2, les clusters
   0 et 1 sont virtuels mais ont leurs entrees dans la table
 */
myfat->BytesPerFAT = ( ( ( myfat->DataClusters + 2 ) * myfat->FAT ) + 7 ) / 8; /* arrondi par exces */
SectorsPerFAT2 = ( myfat->BytesPerFAT + (myfat->bps - 1) ) / myfat->bps; /* arrondi par exces */
printf("nombre de secteurs par FAT recalcule %d (vs %d)\n", SectorsPerFAT2, myfat->SectorsPerFAT );
if ( SectorsPerFAT2 > myfat->SectorsPerFAT )
   { printf("erreur sur taille FAT...\n"); return(5); }

if   ( myfat->FAT == 32 )
     {
     printf("Flags %02X, FAT32 version %04X\n", b->Flags, b->Version );
     myfat->RootStartCluster = b->RootStartCluster;
     printf("premier cluster du root dir %d\n", b->RootStartCluster );
     printf("premier secteur de la zone FSinfo %d\n", b->FSInfoSec );
     printf("premier secteur du backup du boot sector %d\n", b->BkUpBootSec );
     printf("Drive Number %02X\n", b->DrvNum );
     printf("Signature (doit etre 29) 0x%02X\n", b->BootSig );
     printf("Numero de serie %02x%02x%02x%02x ou %08x\n",
             b->VolID[0], b->VolID[1], b->VolID[2], b->VolID[3], *((int*)b->VolID) );
     printf("Nom du volume %-.11s\n", b->VolLab );
     printf("Nom du systeme de fichiers %-.8s\n", b->FileSysType );
     }
else {
     d = (FAT16boot *) buf;
     printf("Drive Number %02X\n", d->DrvNum );
     printf("Signature (doit etre 29) 0x%02X\n", d->BootSig );
     printf("Numero de serie %02x%02x%02x%02x ou %08x\n",
             d->VolID[0], d->VolID[1], d->VolID[2], d->VolID[3], *((int*)d->VolID) );
     printf("Nom du volume %-.11s\n", d->VolLab );
     printf("Nom du systeme de fichiers %-.8s\n", d->FileSysType );
     }
return(0);
}


void DumpDirectory( unsigned char * buf, int entries )
{
int i, j; DIRENT * d; int c;
printf("capacite %d entrees\n", entries );
for ( i = 0; i < entries; i++ )
	{
	d = ((DIRENT *)buf) + i;
	// commence par zero : entree jamais utilisee
	// commence par 0xE5 : fichier efface
	c = d->name[0] & 0xFF;
	if	( ( c != 0 ) && ( c != 0xE5 ) )
		{
		if	( d->attr & ATTR_DIRECTORY )
			printf("<DIR> %-.8s  %-.3s\n", d->name, d->ext );
		else if	( ( d->attr & ATTR_LONG_NAME ) == ATTR_LONG_NAME )
			{
			for	( j = 1; j <= 9; j += 2 )
				{
				c = ((char *)d)[j] & 0xFF;
				if	( ( c >= ' ' ) && ( ( c & 0x80 ) == 0 ) )  
					printf("%c", c );
				}
			for	( j = 14; j <= 24; j += 2 )
				{
				c = ((char *)d)[j] & 0xFF;
				if	( ( c >= ' ' ) && ( ( c & 0x80 ) == 0 ) )  
					printf("%c", c );
				}
			for	( j = 28; j <= 30; j += 2 )
				{
				c = ((char *)d)[j] & 0xFF;
				if	( ( c >= ' ' ) && ( ( c & 0x80 ) == 0 ) )  
					printf("%c", c );
				}
			j = 28;
			printf("\n");
			}
		else printf("----> %-.8s  %-.3s  %d bytes\n", d->name, d->ext, d->size );
		}
	}
// HAdump( buf, entries * sizeof(DIRENT) );
}

// dump hexa+ascii
void HAdump( unsigned char * buf, int q )
{
int i, j; unsigned char c;
i = 0;
while ( i < q )
   {
   j = 0;
   while ( ( j < 16 ) && ( i + j < q ) )
     {
     printf("%02X ", buf[i+j] ); j++;
     }
   j = 0;
   while ( ( j < 16 ) && ( i + j < q ) )
     {
     c = buf[i+j];
     if ( c < ' ' ) c = '.';
     if ( c > 126 ) c = '_'; 
     printf("%c", c ); j++;
     }
   printf("\n");
   i += 16;
   }
}

// cette fonction rend l'indice de l'entree trouvee (sinon -1) et son numero de 1er cluster 
int FindFile( char * shortName, unsigned char * dirbuf, int entries, unsigned int * clu, unsigned int * size )
{
int i, c; DIRENT * d;
// d'abord on cherche l'indice dans le dir
for	( i = 0; i < entries; i++ )
	{
	d = ((DIRENT *)dirbuf) + i;
	c = d->name[0] & 0xFF;
	if	( ( c != 0 ) && ( c != 0xE5 ) )
		{
		if	( ( d->attr & ATTR_LONG_NAME ) != ATTR_LONG_NAME )
			if	( strncmp( d->name, shortName, 11 ) == 0 )
				break;
		}
	}
if	( i >= entries )
	return -1;	// pas trouve
// puis on cherche le numero de cluster, qui est en 2 moities disjointes
*clu = d->cluster_lo;
if	( myfat->FAT == 32 )
	*clu |= ( d->cluster_hi << 16 );
// puis la taille du fichier en bytes
*size = d->size;
return(i);
}


/* cette fontion rend le nombre de clusters de la chaine */
int ScanChain( unsigned int startCluster )
{
unsigned int next, cnt, EOC;
unsigned int * fat32;
unsigned short * fat16;

next = startCluster; cnt = 0;
if   ( myfat->FAT < 32 )
     {
     fat16 = (unsigned short *) myfat->fatbuf;
     EOC = (myfat->FAT==12)?(0xFFF):(0xFFFF);
     while ( next != EOC )
           {
           next = (unsigned int) fat16[next];
           cnt++;
           }
     }
else {
     fat32 = (unsigned int *) myfat->fatbuf;
     EOC = 0x0FFFFFFF;
     while ( next != EOC )
           {
           next = fat32[next] & EOC;
           cnt++;
           }
     }
return(cnt);
}

/* cette fonction convertit la FAT12 en FAT16 */
void ConvertFAT()
{
unsigned short * fat16buf; unsigned char v[3];
int i, j, entries;

entries = ( myfat->bps * myfat->SectorsPerFAT * 2 ) / 3;
fat16buf = ( unsigned short * ) malloc( entries * 2 );
if ( fat16buf == NULL )
   { printf("malloc m'a tuer = %d\n", entries * 2 ); exit(-1); }

i = j = 0;
while( j < entries )
   {
   v[0] = myfat->fatbuf[i++];
   v[1] = myfat->fatbuf[i++];
   v[2] = myfat->fatbuf[i++];
   fat16buf[j++] = v[0] | ( (v[1] & 0xF ) << 8 ); 
   fat16buf[j++] = ( (v[1] >> 4 ) & 0xF ) | ( v[2] << 4 );
   }
free( myfat->fatbuf );
myfat->fatbuf = ( unsigned char * ) fat16buf;
}


/** fonctions utilisant les fonctions d'acces aux secteurs (diskio) --------------------------- */


/* cette fontion charge en memoire les secteurs d'une chaine (i.e. un fichier)
   apres avoir alloue la memoire selon nclu */
int CopyChain( int startCluster, int nclu, unsigned char ** pbuf )
{
unsigned int next, EOC, cnt, bpc;
int retval;
unsigned int * fat32;
unsigned short * fat16;

bpc = myfat->bps * myfat->SectorsPerCluster; 
*pbuf = malloc( nclu * bpc );
if	( *pbuf == NULL )
	return -1;

next = startCluster; cnt = 0;
if	( myfat->FAT < 32 )
	{
	fat16 = (unsigned short *) myfat->fatbuf;
	EOC = (myfat->FAT==12)?(0xFFF):(0xFFFF);
	while	( next != EOC )
		{
		retval = disk_read( 0, *pbuf + ( cnt * bpc ),
				myfat->FirstDataSector + ( next - 2 ) * myfat->SectorsPerCluster,
				myfat->SectorsPerCluster );
		if	( retval )
			return ( - 20 - retval );
		next = (unsigned int) fat16[next];
		cnt++;
		}
	}
else	{
	fat32 = (unsigned int *) myfat->fatbuf;
	EOC = 0x0FFFFFFF;
	while	( next != EOC )
		{
		retval = disk_read( 0, *pbuf + ( cnt * bpc ),
				myfat->FirstDataSector + ( next - 2 ) * myfat->SectorsPerCluster,
				myfat->SectorsPerCluster );
		if	( retval )
			return ( - 20 - retval );
		next = fat32[next] & EOC;
		cnt++;
		}
	}
return 0;
}

// ouvrir le device et decouvrir la FAT
// rend zero si on a trouve un boot sector representatif d'une FAT
int myfat_start_session( const char * devicename )
{
unsigned char buf[512];
int retval, next;

if	( ( sizeof(FAT32boot) != 90 ) ||
	  ( sizeof(FAT16boot) != 62 ) ||
	  ( sizeof(PARTITION) != 16 ) ||
	  ( sizeof(DIRENT)    != 32 )
	)
	return -700;

win32_set_disk_devicename( devicename );

retval = disk_initialize(0);
if	(retval)
	return( - 100 - retval );

// lire le secteur zero
retval = disk_read( 0, buf, 0, 1 );
if	(retval)
	return( - 200 - retval );

// HAdump( buf, 512 );

myfat->FATpartBase = 0;
retval = DumpBootSector( buf );
if	( retval == 0 )		// cas drive non partitionne ou volume simple
	return 0;		// pret pour lecture FAT
else if ( retval == 2 )		// on interprete ce secteur comme premiere table de partition
	{
	myfat->FATpartBase = -1;
	myfat->EXTpartBase = myfat->EXTpartBaseF = 0;
	next = DumpPartTable( buf );
	if	( myfat->FATpartBase >= 0 )	// premiere table
		{
		retval = disk_read( 0, buf, next, 1 );
		if	(retval)
			return( - 210 - retval );
		retval = DumpBootSector( buf );
		if	( retval == 0 )		// pret pour lecture FAT
			return 0;
		}
	while	( next )	// ZONE NON TESTEE
		{			// tables de partition etendues
		retval = disk_read( 0, buf, next, 1 );
		if	(retval)
			return( - 300 - retval );
		printf( "lu 1 partition etendue\n" );
		myfat->EXTpartBase = next;
		next = DumpPartTable( buf );
		if	( myfat->FATpartBase >= 0 )
			{
			retval = disk_read( 0, buf, next, 1 );
			if	(retval)
				return( - 210 - retval );
			retval = DumpBootSector( buf );
			if	( retval == 0 )		// pret pour lecture FAT
				return 0;
			}
		}
	}
return( - 666 );
}

// alloc et lecture FAT entiere en RAM, retour 0 si Ok
int myfat_read_fat()	// appeler apres succes DumpBootSector()
{
int retval;

printf("lecture FAT...%d sectors\n", myfat->SectorsPerFAT );
myfat->fatbuf = (unsigned char *) malloc( myfat->bps * myfat->SectorsPerFAT );
if	( myfat->fatbuf == NULL )
	return -1;
retval = disk_read( 0, myfat->fatbuf, myfat->FirstFATSector, myfat->SectorsPerFAT );
if	(retval)
	return -2;
if	( myfat->FAT == 12 )
	ConvertFAT();
return 0;
}

// alloc et lecture du root dir en RAM dans myfat->rootbuf, retour 0 si Ok
int myfat_read_root_dir()
{
int retval, nclu;
if	( myfat->FAT < 32 )
	{
	myfat->rootbuf = (unsigned char *) malloc( myfat->bps * myfat->RootDirSectors );
	if	( myfat->rootbuf == NULL )
		return(-1);
	retval = disk_read( 0, myfat->rootbuf, myfat->FirstDirSector, myfat->RootDirSectors );
	if	( retval )
		return ( - 30 - retval );
	}
else	{
	nclu = ScanChain( myfat->RootStartCluster );
	myfat->RootDirSectors = nclu * myfat->SectorsPerCluster;
	printf("FAT32 root dir chain size %d clusters = %d bytes\n", nclu, 
		myfat->bps * myfat->RootDirSectors );
	retval = CopyChain( myfat->RootStartCluster, nclu, &(myfat->rootbuf) );
	if	( retval )
		return ( - 40 - retval );
	}
DumpDirectory( myfat->rootbuf, myfat->RootDirSectors * ( myfat->bps / 32 ) );
return 0;
}

// lecture et copie locale d'une chaine
int myfat_save_chain( unsigned int startCluster, unsigned int size, const char * local_path )
{
int desthand;
desthand = open( local_path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666 );
if	( desthand <= 0 )
	return -1;

unsigned int clu_size = myfat->SectorsPerCluster * myfat->bps;
unsigned char cbuf[clu_size];

// if ( write( desthand, dbuf, aecrire ) != aecrire )
   
unsigned int next, EOC, retval, bytes_to_write;
unsigned int * fat32 = (unsigned int *) myfat->fatbuf;
unsigned short * fat16 = (unsigned short *) myfat->fatbuf;
EOC = (myfat->FAT==32)?(0x0FFFFFFF):((myfat->FAT==12)?(0xFFF):(0xFFFF));
next = startCluster;
while	( next != EOC )
        {
	retval = disk_read( 0, cbuf,			// -2 ? pourquoi ?
			    myfat->FirstDataSector + (next-2) * myfat->SectorsPerCluster,
			    myfat->SectorsPerCluster );
	if	( retval )
		return( - 700 - retval );
	bytes_to_write = (size>=clu_size)?(clu_size):(size);
	if	( write( desthand, cbuf, bytes_to_write ) != bytes_to_write )
		return( - 750 );
	size -= bytes_to_write;
	if	( size == 0 )
		break;
        next = (myfat->FAT==32)?(fat32[next]&EOC):((unsigned int)fat16[next]);
        }
close(desthand);
return(0);
}
