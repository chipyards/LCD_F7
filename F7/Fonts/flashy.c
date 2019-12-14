#include "../Src/options.h"
#ifdef FLASH_THE_FONTS
/* acces memoire FLASH -------------------------------------------- */

// copie une fonte - ne fait pas l'effacement
// rend la taille en bytes
unsigned int flash_1_font( const JFONT * lafont, unsigned int adr )
{
unsigned int size2, i, retval, val;
size2 = lafont->h * QCHAR;
retval = 0;
for	( i = 0; i < size2; ++i )
	{
	val = lafont->rows[i];
	retval |= HAL_FLASH_Program( FLASH_TYPEPROGRAM_HALFWORD, adr + ( i * 2 ), (uint64_t)val );
	}
return( size2 * 2 );
}

// verifie une fonte 
// rend le nombre d'erreurs
unsigned int check_1_font( const JFONT * lafont, unsigned int adr )
{
int i, retval = 0;
for	( i = 0; i < ( lafont->h * QCHAR ); ++i )
	{
	if	( lafont->rows[i] != ((unsigned short *)adr)[i] )
		++retval;
	}
return retval;
}


// copie et/ou verif des fonts dans la flash, @ FLASH_FONTS_BASE & FLASH_FONTS_SECTOR 
// rend l'encombrement total
unsigned int flash_the_fonts()
{
unsigned int adr = FLASH_FONTS_BASE;
int retval;

retval = HAL_FLASH_Unlock();
FLASH_Erase_Sector( FLASH_FONTS_SECTOR, FLASH_VOLTAGE_RANGE_3 );
retval = FLASH_WaitForLastOperation((uint32_t)50000);
retval = flash_1_font( &JFont24, adr );
adr += retval;
retval = flash_1_font( &JFont20, adr );
adr += retval;
retval = flash_1_font( &JFont16, adr );
adr += retval;
retval = flash_1_font( &JFont12, adr );
adr += retval;
retval = flash_1_font( &JFont8, adr );
adr += retval;
HAL_FLASH_Lock();
return( adr - FLASH_FONTS_BASE );
}

// copie et/ou verif des fonts dans la flash, @ FLASH_FONTS_BASE & FLASH_FONTS_SECTOR 
// rend le nombre d'erreurs
unsigned int check_the_fonts()
{
unsigned int adr = FLASH_FONTS_BASE;
int retval;

retval = check_1_font( &JFont24, adr );
adr += ( JFont24.h * QCHAR * 2 );
retval += check_1_font( &JFont20, adr );
adr += ( JFont20.h * QCHAR * 2 );
retval += check_1_font( &JFont16, adr );
adr += ( JFont16.h * QCHAR * 2 );
retval += check_1_font( &JFont12, adr );
adr += ( JFont12.h * QCHAR * 2 );
retval += check_1_font( &JFont8, adr );
adr += ( JFont8.h * QCHAR * 2 );
return retval;
}
#endif
