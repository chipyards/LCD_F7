#include "options.h"
#if defined (USE_UART1) || defined (USE_UART6)

#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_utils.h"
// #include "stm32f7xx_ll_pwr.h"
#include "stm32f7xx_ll_usart.h"
#include <stdio.h>
#include <stdarg.h>
#include "s_gpio.h"
#include "uarts.h"

#ifdef USE_UART1
// objet CDC ------------------------------------------------------------------------------

UART1type CDC;	// pour communiquer avec PC via ST-Link 

// traitement interrupts
void USART1_IRQHandler( void )		// UART CDC vers USB
{
if	(
	( LL_USART_IsActiveFlag_TXE( USART1 ) ) &&
	( LL_USART_IsEnabledIT_TXE( USART1 ) )
	)
	{
	int c;
	c = CDC.TXbuf[CDC.TXindex++];
	if	( ( c ) && ( CDC.TXindex < QTX1 ) )
		{
		LL_USART_TransmitData8( USART1, c );
		}
	else	{
		UART1_TX_INT_disable();
		CDC.TXindex = -1;		// unlock
		}
	}
if	(
	( LL_USART_IsActiveFlag_RXNE( USART1 ) ) &&
	( LL_USART_IsEnabledIT_RXNE( USART1 ) )
	)
	{
	CDC.RXbyte = LL_USART_ReceiveData8( USART1 );
	}
}

// constructeur
void CDC_init()
{
CDC.TXindex = -1;		// unlock
CDC.RXbyte = -1;		// empty
GPIO_config_uart1();
UART1_init(9600);
}

// envoyer une ligne de texte formattee
// retourne 1 si renoncement pour cause de transmission en cours
int CDC_print( const char *fmt, ... )
{
va_list  argptr;
if	( CDC.TXindex >= 0 )
	return 1;		// transmetteur occupe..
va_start( argptr, fmt );
vsnprintf( (char *)CDC.TXbuf, sizeof(CDC.TXbuf), fmt, argptr );
va_end( argptr );
CDC.TXindex = 0;
UART1_TX_INT_enable();
return 0;
}

// lire une commande
int CDC_getcmd()
{
int c = CDC.RXbyte;
CDC.RXbyte = -1;	// consommer le byte
return c;
}

#endif
// LOW LAYER ------------------------------------------------------------------------------

// code d'initialisation communs aux UARTs
static void UART_8_N( USART_TypeDef * U, unsigned int bauds, int stops )
{
LL_USART_InitTypeDef usart_initstruct;
usart_initstruct.BaudRate            = bauds;
usart_initstruct.DataWidth           = LL_USART_DATAWIDTH_8B;
usart_initstruct.StopBits            = ((stops==1)?(LL_USART_STOPBITS_1):(LL_USART_STOPBITS_2));
usart_initstruct.Parity              = LL_USART_PARITY_NONE;
usart_initstruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
usart_initstruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
usart_initstruct.OverSampling        = LL_USART_OVERSAMPLING_16;
LL_USART_Init( U, &usart_initstruct);
LL_USART_Enable( U );
// wait
while	(!(LL_USART_IsActiveFlag_TEACK( U )))
	{}
}

// config d'un canal d'interruption
static void NVIC_init( IRQn_Type IRQn, int prio )
{
NVIC_SetPriority( IRQn, prio );  
NVIC_EnableIRQ( IRQn );
}

#ifdef USE_UART1
// UART1 en mode interruption ==============================================================
// la fonction USART1_IRQHandler() doit etre définie en quelque part

void UART1_init( unsigned int bauds )
{
LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
LL_RCC_SetUSARTClockSource( LL_RCC_USART1_CLKSOURCE_PCLK2 );
UART_8_N( USART1, bauds, 1 );

NVIC_init( USART1_IRQn, 9 );
NVIC_ClearPendingIRQ( USART1_IRQn );
/* Enable USART1 Receive interrupts --> USART1_IRQHandler */
LL_USART_EnableIT_RXNE( USART1 );
// N.B. interruption TX sera validee a la demande
}

// mise en route de l'emission par interruption
void UART1_TX_INT_enable()
{
LL_USART_EnableIT_TXE( USART1 );
}

// arret de l'emission par interruption
void UART1_TX_INT_disable()
{
LL_USART_DisableIT_TXE( USART1 );
}
#endif

#ifdef USE_UART6
// UART6 en mode interruption ==============================================================
// la fonction USART6_IRQHandler() doit etre définie en quelque part

void UART6_init( unsigned int bauds )
{
LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART6 );
LL_RCC_SetUSARTClockSource( LL_RCC_USART6_CLKSOURCE_PCLK2 );
UART_8_N( USART6, bauds, 2 );

NVIC_init( USART6_IRQn, 10 );
NVIC_ClearPendingIRQ( USART6_IRQn );
/* Enable USART6 Receive interrupts --> USART6_IRQHandler */
LL_USART_EnableIT_RXNE( USART6 );
// N.B. interruption TX sera validee a la demande
}

// mise en route de l'emission par interruption
void UART6_TX_INT_enable()
{
LL_USART_EnableIT_TXE( USART6 );
}

// arret de l'emission par interruption
void UART6_TX_INT_disable()
{
LL_USART_DisableIT_TXE( USART6 );
}
#endif
#endif
