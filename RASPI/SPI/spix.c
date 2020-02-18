/* 
gcc -Wall -o spix spix.c -lbcm2835
ou
gcc -Wall -o spix spix.c libbcm2835.a
*/

#include <stdio.h>
#include <stdlib.h>

void usage()
{
printf("Ce programme pour Raspberry peut :\n");
printf("  envoyer un message de 5 octets sur une liaison SPI dont le raspberry est maitre\n");
printf("  recevoir 5 octets en parallele\n");
printf("options :\n");
printf("  -v <v1> <v2> <v3> : 3 octets a envoyer au milieu du message\n"); 
printf("  -r <t> : repetition a intervalles de t secondes\n");
exit(1);
}

#include <ctype.h>	// juste pour isdigit()
#include <unistd.h>	// juste pour sleep (que je croyais deprecated...)


#include "bcm2835.h"
int spi_dial( unsigned char * txdata, unsigned char * rxdata, int N )
{
int i;

if	(!bcm2835_init())
        return 1;
if	(!bcm2835_spi_begin())
	return 2;
bcm2835_spi_setBitOrder( BCM2835_SPI_BIT_ORDER_MSBFIRST );
bcm2835_spi_setDataMode( BCM2835_SPI_MODE0 );
bcm2835_spi_setClockDivider( BCM2835_SPI_CLOCK_DIVIDER_2048 );	// freq. source = 250 MHz
bcm2835_spi_chipSelect( BCM2835_SPI_CS0 );			// 2048 -> 122 kHz
bcm2835_spi_setChipSelectPolarity( BCM2835_SPI_CS0, LOW );

for	( i = 0; i < N; ++i )
	{
	rxdata[i] = bcm2835_spi_transfer( txdata[i] );
	}
bcm2835_spi_end();
bcm2835_close();
return 0;
}

int spin_dial( unsigned char * txdata, unsigned char * rxdata, int N )
{
if	(!bcm2835_init())
        return 1;
if	(!bcm2835_spi_begin())
	return 2;
bcm2835_spi_setBitOrder( BCM2835_SPI_BIT_ORDER_MSBFIRST );
bcm2835_spi_setDataMode( BCM2835_SPI_MODE0 );
bcm2835_spi_setClockDivider( BCM2835_SPI_CLOCK_DIVIDER_2048 );	// freq. source = 250 MHz
bcm2835_spi_chipSelect( BCM2835_SPI_CS0 );			// 2048 -> 122 kHz
bcm2835_spi_setChipSelectPolarity( BCM2835_SPI_CS0, LOW );

bcm2835_spi_transfernb(	(char *)txdata, (char *)rxdata, N );		

bcm2835_spi_end();
bcm2835_close();
return 0;
}


#define DIAL_SIZE 5

int main( int argc, char ** argv )
{
int val[3];

// options
int opt_repeat = -1; val[0] = -1;

int iarg = 1;
while	( iarg < argc )
	{
	if	( argv[iarg][0] != '-' )
		usage();
	switch	( argv[iarg][1] )
		{
		case 'r' : if ( argc < (iarg+2) ) usage();
			   if ( !isdigit( argv[iarg+1][0] ) ) usage();
			   opt_repeat = atoi( argv[iarg+1] );
			   ++iarg; break;
		case 'v' : if ( argc < (iarg+4) ) usage();
			   if ( !isdigit( argv[iarg+1][0] ) ) usage();
			   if ( !isdigit( argv[iarg+2][0] ) ) usage();
			   if ( !isdigit( argv[iarg+3][0] ) ) usage();
			   val[0] = atoi( argv[iarg+1] );
			   val[1] = atoi( argv[iarg+2] );
			   val[2] = atoi( argv[iarg+3] );
			   iarg += 3; break;
		default : usage();
		}
	++iarg;
	}

// action eventuellement repetee indefiniment
do	{
	int i, retval;
	unsigned char txdata[DIAL_SIZE] = { 0xCA, 0xFE, 0xBB, 0xD0, 0xD0 };
	unsigned char rxdata[DIAL_SIZE];
	if	( val[0] >= 0 )
		{
		txdata[1] = val[0];
		txdata[2] = val[1];
		txdata[3] = val[2];
		}
	retval = spin_dial( txdata, rxdata, DIAL_SIZE );
	if	( retval == 0 )
		{
		printf("rx : ");
		for	( i = 0; i < DIAL_SIZE; ++i )
			printf("%02x ", rxdata[i] );
		printf("\n");
		}
	else	{ printf("spi_dial failed %d\n", retval ); break; }
	if	( opt_repeat >= 0 )
		printf("repetition automatique (periode %d s), Ctrl-C pour stopper\n", opt_repeat );
	if	( opt_repeat > 0 )
		{
		sleep( opt_repeat );
		}
	} while ( opt_repeat >= 0 );
return 0;
}
