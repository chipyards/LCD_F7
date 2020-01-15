#include "options.h"

#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_gpio.h"
#include "s_gpio.h"

void GPIO_config_bouton(void)	// PI11
{
LL_GPIO_InitTypeDef gpio_initstruct;
// port I (aussi utilise pour LED verte)
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOI);
gpio_initstruct.Mode       = LL_GPIO_MODE_INPUT;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_NO;		// pulldown discret
gpio_initstruct.Pin        = LL_GPIO_PIN_11;
LL_GPIO_Init(GPIOI, &gpio_initstruct);
}

int GPIO_bouton_bleu(void)	// Act Hi
{
return LL_GPIO_IsInputPinSet( GPIOI, LL_GPIO_PIN_11 );
}


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

#ifdef USE_SDCARD
void GPIO_config_SDCard( void )
{
LL_GPIO_InitTypeDef gpio_initstruct;
// ports C et D
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
// params communs
gpio_initstruct.Mode       = LL_GPIO_MODE_ALTERNATE;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_UP;
gpio_initstruct.Alternate  = LL_GPIO_AF_12;
// port C
gpio_initstruct.Pin        = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
LL_GPIO_Init(GPIOC, &gpio_initstruct);
// port D
gpio_initstruct.Pin        = LL_GPIO_PIN_2;
LL_GPIO_Init(GPIOD, &gpio_initstruct);
// card detect
// PC13
gpio_initstruct.Mode       = LL_GPIO_MODE_INPUT;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_UP;	// ! pas de pullup discret!
gpio_initstruct.Pin        = LL_GPIO_PIN_13;
LL_GPIO_Init(GPIOC, &gpio_initstruct);
}

int GPIO_SDCARD_present(void)
{
return (!LL_GPIO_IsInputPinSet( GPIOC, LL_GPIO_PIN_13 ));
}

#ifdef USE_SIDEBAND
void GPIO_config_sideband( void )
{
LL_GPIO_InitTypeDef gpio_initstruct;
// port F
LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
// PF7, PF8, PF9, PF10 aka A5..A2
gpio_initstruct.Mode       = LL_GPIO_MODE_INPUT;
gpio_initstruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
gpio_initstruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
gpio_initstruct.Pull       = LL_GPIO_PULL_UP;	//
gpio_initstruct.Pin        = LL_GPIO_PIN_7 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
LL_GPIO_Init(GPIOF, &gpio_initstruct);
}

// acquisition de 4 bits de data
// PF7, PF8, PF9, PF10 aka A5..A2
unsigned int GPIO_sideband_in(void)
{
unsigned int nibble;
/* generateur de nibbles pour test
static int cnt = 1; // test
nibble = cnt & 0xF;
if	( (++cnt) >= 14 )
	cnt = 1;
*/
nibble = LL_GPIO_ReadInputPort(GPIOF);
nibble >>= 7;
nibble &= 0x0F;
return nibble;
}
#endif
#endif
