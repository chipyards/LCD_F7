/* 
gcc -Wall -o spix spix.c -lbcm2835
ou
gcc -Wall -o spix spix.c libbcm2835.a
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage()
{
printf("Ce programme pour Raspberry peut :\n");
printf("  envoyer un message texte ascii sur une liaison SPI dont le raspberry est maitre\n");
printf("  recevoir et afficher le meme nombre de bytes en parallele\n");
printf("options :\n");
printf("  -r <t> : repetition a intervalles de t secondes\n");
exit(1);
}

#include <unistd.h>	// juste pour sleep (que je croyais deprecated...)
#include "bcm2835.h"

/* deprecated produit des glitches sur NSS entre les bytes 
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
*/

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


#define DIAL_SIZE 256

int main( int argc, char ** argv )
{
unsigned char txbuf[DIAL_SIZE];
unsigned char rxbuf[DIAL_SIZE];
int msg_len = 0;

// options
int opt_repeat = -1; txbuf[0] = 0;

int iarg = 1;
while	( iarg < argc )
	{
	if	( argv[iarg][0] != '-' )
		{			// le ';' est vu comme delimiteur par le STM32
		snprintf( (char *)txbuf, sizeof(txbuf), "%s\n;", argv[iarg] );
		txbuf[sizeof(txbuf)-1] = 0;
		}
	else	{
		switch	( argv[iarg][1] )
			{
			case 'r' : if	( argc < (iarg+2) ) usage();
				   opt_repeat = atoi( argv[iarg++] );
				   if	( opt_repeat <= 0 ) usage();
				   break;
			default : usage();
			}
		}
	++iarg;
	}
msg_len = strlen( (char *)txbuf );
if	( msg_len <= 0 )
	usage();

// action eventuellement repetee indefiniment
do	{
	int i, retval;
	retval = spin_dial( txbuf, rxbuf, msg_len );
	if	( retval == 0 )
		{
		char c;
		printf("rx : ");
		for	( i = 0; i < msg_len; ++i )
			{
			c = rxbuf[i];
			if	( c != 0 )
				printf("%c", c );
			}
		printf("\n");
		}
	else	{ printf("spin_dial failed %d\n", retval ); break; }
	if	( opt_repeat >= 0 )
		printf("repetition automatique (periode %d s), Ctrl-C pour stopper\n", opt_repeat );
	if	( opt_repeat > 0 )
		{
		sleep( opt_repeat );
		}
	} while ( opt_repeat >= 0 );
return 0;
}
