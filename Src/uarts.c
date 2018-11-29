#include "options.h"
#ifdef USE_UART

#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_utils.h"
// #include "stm32f7xx_ll_pwr.h"
#include "stm32f7xx_ll_usart.h"
#include "uarts.h"


// code d'initialisation communs aux UARTs
static void UART_8_N_2( USART_TypeDef * U, unsigned int bauds )
{
LL_USART_InitTypeDef usart_initstruct;
usart_initstruct.BaudRate            = bauds;
usart_initstruct.DataWidth           = LL_USART_DATAWIDTH_8B;
usart_initstruct.StopBits            = LL_USART_STOPBITS_2;
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

// UART1 en mode interruption ==============================================================
// la fonction USART1_IRQHandler() doit etre définie par ailleurs

void UART1_init( unsigned int bauds )
{
LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART1 );
LL_RCC_SetUSARTClockSource( LL_RCC_USART1_CLKSOURCE_PCLK2 );
UART_8_N_2( USART1, bauds );

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

#ifdef USE_UART6
// UART6 en mode interruption ==============================================================
// la fonction USART6_IRQHandler() doit etre définie par ailleurs
// TX ONLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void UART6_init( unsigned int bauds )
{
LL_APB2_GRP1_EnableClock( LL_APB2_GRP1_PERIPH_USART6 );
LL_RCC_SetUSARTClockSource( LL_RCC_USART6_CLKSOURCE_PCLK2 );
UART_8_N_2( USART6, bauds );

NVIC_init( USART6_IRQn, 10 );
NVIC_ClearPendingIRQ( USART6_IRQn );
/* Enable USART6 Receive interrupts --> USART6_IRQHandler */
// LL_USART_EnableIT_RXNE( USART6 );
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
