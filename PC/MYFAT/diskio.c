// interface disk niveau secteur Win32, style FatFs

#include <windows.h>
#include <winioctl.h>
//#include <stdio.h>
//#include <fcntl.h>
//#include <stdlib.h>
//#include <string.h>

//#include <io.h>

#include "diskio.h"

#define SECTOR_512 512

/** global data ------------------------------------------------------- */

const char * devicename;
static HANDLE hDevice = INVALID_HANDLE_VALUE;

/** FatFs shared functions ----------------------------------------------- */

DSTATUS disk_initialize (BYTE pdrv)
{
// Creating a handle to disk drive using CreateFile () function ..
hDevice = CreateFile( devicename, 
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, OPEN_EXISTING, 0, NULL); 
if	( hDevice == INVALID_HANDLE_VALUE )
	return RES_NOTRDY;
return 0;
}

DSTATUS disk_status (BYTE pdrv)
{
if	( hDevice == INVALID_HANDLE_VALUE )
	return RES_NOTRDY;
return 0;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, DWORD count)
{
if	( hDevice == INVALID_HANDLE_VALUE )
	return RES_NOTRDY;
unsigned long long ibyte;	// adresse en bytes
ibyte = (unsigned long long)sector * SECTOR_512;
// Setting the pointer to point to the start of the sector we want to read ..
// cette fonction accepte 64 bits !! en 2 morceaux... dont le second passe par adresse !!!
SetFilePointer( hDevice, ((unsigned int*)&ibyte)[0], (long int *)(&((unsigned int*)&ibyte)[1]), FILE_BEGIN );
unsigned int bytesread = 0;
unsigned int bytestoread = count * SECTOR_512;
if	(!ReadFile( hDevice, buff, bytestoread, (long unsigned int *)&bytesread, NULL ) )
	return RES_ERROR;
if	( bytesread != bytestoread )
	return RES_ERROR;
return 0;
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, DWORD count)
{
return 0;
}

/** Win32 shared functions ----------------------------------------------- */

void win32_disk_close()
{
CloseHandle(hDevice); 
}

void win32_set_disk_devicename( const char * name )
{
devicename = name;
}
