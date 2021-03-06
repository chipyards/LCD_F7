#include "options.h"
#ifdef USE_SDCARD
#include "jrtc.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"

// static data

static FATFS SDFatFs;  /* File system object for SD card logical drive */
static char SDPath[4]; /* SD card logical drive path */


/** local utility functions ---------------------------------- */

#define CRC_POLY 0xEDB88320	// polynome de zlib, zip et ethernet
static unsigned int crc_table[256];
// The table is simply the CRC of all possible eight bit values.
static void make_crc_table()
{
unsigned int c, n, k;
for ( n = 0; n < 256; n++ )
    {
    c = n;
    for ( k = 0; k < 8; k++ )
        c = c & 1 ? CRC_POLY ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
    }
}

// cumuler le calcul du CRC sur des bytes
// initialiser avec :	crc = 0xffffffff;
// finir avec :		crc ^= 0xffffffff;
static void icrc32( const unsigned char *buf, int len, unsigned int * crc )
{
while	( len-- )
	{
	*crc = crc_table[((*crc) ^ (*buf++)) & 0xff] ^ ((*crc) >> 8);
	};
}

// mount
int SDCard_mount(void)
{
// monter le FS
if	( f_mount( &SDFatFs, (TCHAR const*)SDPath, 0) )
	return -2;
return 0;
}

/** test function ------------------------------------------ */

// simple read test
int SDCard_read_test( const char *fnam, char * tbuf, unsigned int size )
{
FIL MyFile;
if	( f_open( &MyFile, "DEMO.TXT", FA_READ ) )
	return -1;
unsigned int bytesread = 0;
if	( f_read( &MyFile, tbuf, size, &bytesread ) )
	return -2;
if	( bytesread < size )
	tbuf[bytesread] = 0;
else	tbuf[sizeof(tbuf)-1] = 0;
// fermer
f_close(&MyFile);
return bytesread;
}

// simple write test
int SDCard_write_test( const char *fnam, char * tbuf, unsigned int size )
{
FIL MyFile;
if	( f_open( &MyFile, fnam, FA_CREATE_ALWAYS | FA_WRITE ) )
	return -1;
unsigned int byteswritten = 0;
// N.B. ne pas ecrire le NULL dans le fichier
if	( f_write( &MyFile, tbuf, size, &byteswritten ) )
	return -2;
f_close(&MyFile);
if	( byteswritten != size )
	return -3;
return byteswritten;
}

// simple append test
int SDCard_append_test( const char *fnam, char * tbuf, unsigned int size )
{
FIL MyFile;
if	( f_open( &MyFile, fnam, FA_OPEN_APPEND | FA_WRITE ) )
	return -1;
unsigned int byteswritten = 0;
// N.B. ne pas ecrire le NULL dans le fichier
if	( f_write( &MyFile, tbuf, size, &byteswritten ) )
	return -2;
f_close(&MyFile);
if	( byteswritten != size )
	return -3;
return byteswritten;
}

// create random file with CRC - size is in bytes
// rend la duree en s, ou <0 si erreur
#define QBUF 32768
int SDCard_random_write_test( unsigned int size, const char * path, unsigned int * crc )
{
FIL MyFile;
unsigned char wbuf[QBUF];
unsigned int tstart, tstop;
unsigned int cnt, wcnt;
make_crc_table();
*crc = 0xffffffff;
jrtc_get_day_time( &daytime ); tstart = daytime.ss + 60 * daytime.mn;
if	( f_open( &MyFile, path, FA_CREATE_ALWAYS | FA_WRITE ) )
	return -1;
while	( size )
	{
	cnt = 0;
	while	( ( size ) && ( cnt < QBUF ) )
		{
		wbuf[cnt] = rand();
		--size; ++cnt;
		}
	icrc32( wbuf, cnt, crc );
	if	( f_write( &MyFile, wbuf, cnt, &wcnt ) )
		return -2;
	if	( wcnt != cnt )
		return -3;
	}
*crc ^= 0xffffffff;
// fermer
f_close(&MyFile);
jrtc_get_day_time( &daytime ); tstop = daytime.ss + 60 * daytime.mn;
tstop -= tstart;
if	( tstop < 0 )
	tstop += 3600;
return tstop;
}

// read random file with CRC - cnt is in bytes
// rend la duree en s, ou <0 si erreur
#define QBUF 32768
int SDCard_random_read_test( const char * path, unsigned int * crc, unsigned int * cnt )
{
FIL MyFile;
unsigned char rbuf[QBUF];
unsigned int tstart, tstop;
unsigned int rcnt;
make_crc_table();
*crc = 0xffffffff; *cnt = 0;
jrtc_get_day_time( &daytime ); tstart = daytime.ss + 60 * daytime.mn;
if	( f_open( &MyFile, path, FA_READ ) )
	return -1;
while	(1)
	{
	if	( f_read( &MyFile, rbuf, QBUF, &rcnt ) )
		break;
	*cnt += rcnt;
	icrc32( rbuf, rcnt, crc );
	if	( rcnt != QBUF )
		break;
	}
*crc ^= 0xffffffff;
// fermer
f_close(&MyFile);
jrtc_get_day_time( &daytime ); tstop = daytime.ss + 60 * daytime.mn;
tstop -= tstart;
if	( tstop < 0 )
	tstop += 3600;
return tstop;
}

// create random raw block with CRC
// rend la duree en s, ou <0 si erreur
#define QSPB 64
int SDCard_random_write_raw( unsigned int startsec, unsigned int qsec, unsigned int * crc )
{
unsigned char wbuf[QSPB*512];
unsigned int tstart, tstop, isec, endsec, i;
int retval;

make_crc_table();
*crc = 0xffffffff;
jrtc_get_day_time( &daytime ); tstart = daytime.ss + 60 * daytime.mn;
endsec = startsec + qsec;

for	( isec = startsec; isec < endsec; isec += QSPB )
	{
	for	( i = 0; i < QSPB*512; ++i )
		wbuf[i] = rand();
		// wbuf[i] = (isec >> 6);
	icrc32( wbuf, QSPB*512, crc );
	retval = disk_write( 0, wbuf, isec, QSPB );
	if	( retval )
		return -retval;
	}
*crc ^= 0xffffffff;

jrtc_get_day_time( &daytime ); tstop = daytime.ss + 60 * daytime.mn;
tstop -= tstart;
if	( tstop < 0 )
	tstop += 3600;
return tstop;
}

// verify random raw block with CRC
// rend la duree en s, ou <0 si erreur
int SDCard_random_read_raw( unsigned int startsec, unsigned int qsec, unsigned int * crc )
{
unsigned char rbuf[QSPB*512];
unsigned int tstart, tstop, isec, endsec;
int retval;

make_crc_table();
*crc = 0xffffffff;
jrtc_get_day_time( &daytime ); tstart = daytime.ss + 60 * daytime.mn;
endsec = startsec + qsec;

for	( isec = startsec; isec < endsec; isec += QSPB )
	{
	retval = disk_read( 0, rbuf, isec, QSPB );
	if	( retval )
		return -retval;
	icrc32( rbuf, QSPB*512, crc );
	}
*crc ^= 0xffffffff;

jrtc_get_day_time( &daytime ); tstop = daytime.ss + 60 * daytime.mn;
tstop -= tstart;
if	( tstop < 0 )
	tstop += 3600;
return tstop;
}

#endif
