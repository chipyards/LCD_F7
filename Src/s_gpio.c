#include "options.h"

#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_gpio.h"
#include "s_gpio.h"

#ifdef USE_UART1
void GPIO_config_uart1(void)
{
LL_GPIO_InitTypeDef gpio_initstruct;

// port A
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

// PA9 alternate push pull, 2MHz		// UART 1 TX
gpio_initstruct.Mode       = LL_GPIO_MODE_ALTERNATE;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_MEDIUM;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_NO;
gpio_initstruct.Alternate  = LL_GPIO_AF_7;
gpio_initstruct.Pin        = LL_GPIO_PIN_9;
LL_GPIO_Init(GPIOA, &gpio_initstruct);

// port B
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
// RB7 alternate input				// UART 1 RX
gpio_initstruct.Mode       = LL_GPIO_MODE_ALTERNATE;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_MEDIUM;
gpio_initstruct.Pull       = LL_GPIO_PULL_NO;
gpio_initstruct.Alternate  = LL_GPIO_AF_7;
gpio_initstruct.Pin        = LL_GPIO_PIN_7;
LL_GPIO_Init(GPIOB, &gpio_initstruct);
}
#endif
#ifdef USE_UART6
void GPIO_config_uart6(void)
{
LL_GPIO_InitTypeDef gpio_initstruct;

// port C
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
// CN4.D1	PC6 = UART6 TX	AF8
// CN4.D0	PC7 = UART6 RX	AF8
gpio_initstruct.Mode       = LL_GPIO_MODE_ALTERNATE;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_MEDIUM;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_NO;
gpio_initstruct.Alternate  = LL_GPIO_AF_8;
gpio_initstruct.Pin        = LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
LL_GPIO_Init(GPIOC, &gpio_initstruct);
}
#endif

#ifdef PROFILER_PI2
// GPIO output pins PI1 et PI2 for profiling
void GPIO_config_profiler_PI1_PI2( void )
{
LL_GPIO_InitTypeDef gpio_initstruct;

// port I (aussi utilise pour LED verte et bouton bleu)
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOI);
// CN7.D13	PI1	(LED verte)
// CN7.D8	PI2
gpio_initstruct.Mode       = LL_GPIO_MODE_OUTPUT;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_NO;
gpio_initstruct.Pin        = LL_GPIO_PIN_1 | LL_GPIO_PIN_2;
LL_GPIO_Init(GPIOI, &gpio_initstruct);
}

void profile_D13( int val )
{
if	( val )
	LL_GPIO_SetOutputPin( GPIOI, LL_GPIO_PIN_1 );
else	LL_GPIO_ResetOutputPin( GPIOI, LL_GPIO_PIN_1 );
}

void profile_D8( int val )
{
if	( val )
	LL_GPIO_SetOutputPin( GPIOI, LL_GPIO_PIN_2 );
else	LL_GPIO_ResetOutputPin( GPIOI, LL_GPIO_PIN_2 );
}
#endif
