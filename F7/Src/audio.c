/** N.B. dans ce prog  :
 * 	1 sample = 1 short (left ou right)
 * 	1 frame  = 1 int (left + right)
 */
#include "options.h"
#ifdef USE_AUDIO
// Audio experiments :
    // simple echo (software loopback)
    #define ECHO
    // generateur
    // #define GENE

#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "bsp_audio.h"	// inclut chip_wm8994.h
#include "audio.h"
#ifdef USE_SIDEBAND
#include "s_gpio.h"
#endif
#include "stdlib.h"	// juste pour abs() !

AUDIObuffers_type audio_buf;

// reglages
int mic_volume = 200;		// 0.375 dB/step, 0 = mute, FS = 239
int line_in_volume = 11;	// 1.5 dB/step, 0dB = 11, full scale 51
int out_volume = 41;		// 1dB/step, 0dB = 57, full-scale 63

// variable pour observation/debug
int halfi_cnt = 0, fulli_cnt = 0;
int halfo_cnt = 0, fullo_cnt = 0;	// cnt DMA interrupts


/** public functions */

#ifdef ECHO
int audio_demo_init( int mic_input )
{
int retval;
unsigned int delai;	// le delai de l'echo exprime en frames
			// le minimum serait AQBUF/2, en effet chaque transfert DMA fait AQBUF/2 samples
			// le max serait FQBUF - AQBUF/2
delai = AQBUF/2;	// delai = 8 ==> mesure analog 930 us de delai total @ 44.1 kHz
audio_buf.fifoR = FQBUF - delai; // fin du PONG
audio_buf.fifoW = FQHEAD;	 // debut du PING
audio_buf.iside = 0;
audio_buf.left_peak = 0;
audio_buf.right_peak = 0;

if	( mic_input )
	retval = BSP_AUDIO_IN_OUT_Init( INPUT_DEVICE_DIGITAL_MICROPHONE_2,
					OUTPUT_DEVICE_HEADPHONE, out_volume, FSAMP );
else	retval = BSP_AUDIO_IN_OUT_Init( INPUT_DEVICE_INPUT_LINE_1,
					OUTPUT_DEVICE_HEADPHONE, out_volume, FSAMP );
// res.	retval = BSP_AUDIO_OUT_Init( OUTPUT_DEVICE_HEADPHONE, out_volume, FSAMP );
// res. retval = BSP_AUDIO_IN_Init( INPUT_DEVICE_INPUT_LINE_1, volume, FSAMP );
BSP_AUDIO_OUT_SetAudioFrameSlot( CODEC_AUDIOFRAME_SLOT_02 );

return retval;
}

void audio_start(void)
{
BSP_AUDIO_OUT_Play( (uint16_t*)(audio_buf.txbuf), AQBUF*4 );	// ridicule, sera divise par 2
BSP_AUDIO_IN_Record( (uint16_t*)(audio_buf.rxbuf), AQBUF*2 );
wm8994_Set_DMIC_Volume( mic_volume );
wm8994_Set_line_in_Volume( line_in_volume );
wm8994_Set_out_Volume( out_volume );
}
#endif

#ifdef GENE
int audio_demo_init( int mic_input )
{
int retval;
retval = BSP_AUDIO_OUT_Init( OUTPUT_DEVICE_HEADPHONE, out_volume, FSAMP );
BSP_AUDIO_OUT_SetAudioFrameSlot( CODEC_AUDIOFRAME_SLOT_02 );
return retval;
}

void audio_start(void)
{
BSP_AUDIO_OUT_Play( (uint16_t*)(audio_buf.txbuf), AQBUF*2 );	// ridicule *2
wm8994_Set_out_Volume( out_volume );
}
#endif


/** private DMA functions --------------------------------------------------- */

#ifdef USE_SIDEBAND
// cette fonction doit etre appelle tous les (AQBUF/2) samples audio par le DMA
// le buffer sidebuf est vide tous les clusters i.e. 8000 samples audio
// avec AQBUF = 16, on a 1000 samples de 4 bits dans 125 ints par cluster
void sideband_sample_input(void)
{
int * sidebuf;		// portion du header de cluster
static int nibbuf = 0;
if	( audio_buf.fifoW < (FQBUF/2) )
	sidebuf = audio_buf.Sfifo + 4;			// PING sauter 4 ints de header du header
else	sidebuf = audio_buf.Sfifo + 4 + (FQBUF/2);	// PONG
unsigned int inbits = GPIO_sideband_in();	// acquisition 4 bits utiles
nibbuf |= inbits;
if	( (audio_buf.iside % 8) == 7 )		// 8 nibbles par int32
	{
	sidebuf[audio_buf.iside/8] = nibbuf;
	nibbuf = 0;
	}
else	nibbuf <<= 4;				// big-endian!!
audio_buf.iside++;
if	( audio_buf.iside >= (8000/(AQBUF/2)) )
	audio_buf.iside = 0;
}
#endif

#ifdef ECHO
// AUDIO OUT sound processing (demo-specific) : sound source for tx DMA
int get_next_stereo_sample(void)
{
int val = audio_buf.Sfifo[audio_buf.fifoR];
audio_buf.fifoR++;
if	( audio_buf.fifoR == FQBUF )
	audio_buf.fifoR = FQHEAD;
else if	( audio_buf.fifoR == (FQBUF/2) )
	audio_buf.fifoR = (FQBUF/2)+FQHEAD;
return val;
}

// AUDIO IN sound processing (demo-specific) : sound sink for rx DMA
void put_next_stereo_sample( int val )
{
audio_buf.Sfifo[audio_buf.fifoW] = val;
// right peak detect
int p = abs( val );
if	( p > audio_buf.right_peak )
	audio_buf.right_peak = p;
// left peak detect
p = abs( val << 16 );
if	( p > audio_buf.left_peak )
	audio_buf.left_peak = p;
audio_buf.fifoW++;
if	( audio_buf.fifoW == FQBUF )
	audio_buf.fifoW = FQHEAD;
else if	( audio_buf.fifoW == (FQBUF/2) )
	audio_buf.fifoW = (FQBUF/2)+FQHEAD;
}

#endif

#ifdef GENE
// sound processing (demo-specific) : sound source for tx
// phase iph varie de 0 a GEN_PERIOD-1
// sawtooth full scale : val = KA * iph - 32768 avec KA = 65536 / GEN_PERIOD
#define GEN_PERIOD 100	// 128 <==> 344,53125 Hz @ 44.1k
#define GEN_AMPL 32760	// peak
#define KA (2*GEN_AMPL/GEN_PERIOD)
static unsigned int iph = 0;
int get_next_left_sample(void)
{
int val = KA * iph - GEN_AMPL;
return val;
}

int get_next_stereo_sample(void)
{
int val = KA * iph - GEN_AMPL;
iph = (iph + 1) % GEN_PERIOD;
val |= (val << 16);
return val;
}
// sound processing (demo-specific) : sound sink for rx
void put_next_stereo_sample( short val )
{}
#endif

// DMA callbacks
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
// AQBUF est la capacite du buffer en frames (1 sample left + 1 sample right)
// on doit transferer la moitie du buffer, soit AQBUF/2 ints
int * sbuf = audio_buf.txbuf;
for	( int i = 0; i < (AQBUF/2); ++i )
	{
	sbuf[i] = get_next_stereo_sample();
	}
#ifdef USE_SIDEBAND
sideband_sample_input();	// N.B. c'est dans DMA OUT pour alleger DMA IN
#endif
// halfo_cnt++;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
int * sbuf = audio_buf.txbuf;
for	( int i = (AQBUF/2); i < AQBUF; ++i )
	{
	sbuf[i] = get_next_stereo_sample();
	}
#ifdef USE_SIDEBAND
sideband_sample_input();
#endif
// fullo_cnt++;
}

void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
int * sbuf = audio_buf.rxbuf;
for	( int i = 0; i < (AQBUF/2); ++i )
	{
	put_next_stereo_sample(sbuf[i]);
	}
halfi_cnt++;
}

void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
int * sbuf = audio_buf.rxbuf;
for	( int i = (AQBUF/2); i < AQBUF; ++i )
	{
	put_next_stereo_sample(sbuf[i]);
	}
fulli_cnt++;
}

// DMA interrupt handlers

/* SAI handle declared in "stm32746g_discovery_audio.c" file */
extern SAI_HandleTypeDef haudio_out_sai;
/* I2S handle declared in "stm32746g_discovery_audio.c" file */
extern SAI_HandleTypeDef haudio_in_sai;

// ces 2 fonctions sont des vrais interrupt handlers definis dans le startup,
// elles appellent une unique fonction de stm32f7xx_hal_dma.c, qui va appeler
// les user callbacks genre BSP_AUDIO_XX_yy_CallBack() definies ci-dessus
// choisies en fonction de la handle qui leur est passee et des flags internes
// du controleur de DMA
void DMA2_Stream4_IRQHandler(void)
{
//profile_D8( 1 );
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
//profile_D8( 0 );
}

void DMA2_Stream7_IRQHandler(void)
{
//profile_D13( 1 );
  HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
//profile_D13( 0 );
}

/** fonctions publiques set-get ------------------------------------ */

// 1dB/step, 0dB = 57, full-scale 63
void set_out_volume( int volume )
{
out_volume = volume;
if	( out_volume > 63 ) out_volume = 63;
if	( out_volume < 0 ) out_volume = 0;
wm8994_Set_out_Volume( out_volume );
}

// 1.5 dB/step, 0dB = 11, full scale 51
void set_line_in_volume( int volume )
{
line_in_volume = volume;
if	( line_in_volume > 51 ) line_in_volume = 51;
if	( line_in_volume < 0 ) line_in_volume = 0;
wm8994_Set_line_in_Volume( line_in_volume );
}

// 0.375 dB/step, 0 = mute, FS = 239
void set_mic_volume( int volume )
{
mic_volume = volume;
if	( mic_volume < 0 ) mic_volume = 0;
if	( mic_volume > 239 ) mic_volume = 239;
wm8994_Set_DMIC_Volume( mic_volume );
}

int get_out_volume(void)
{ return out_volume; }
int get_line_in_volume(void)
{ return line_in_volume; }
int get_mic_volume(void)
{ return mic_volume; }

#endif
