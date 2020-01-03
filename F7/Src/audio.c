#include "options.h"
#ifdef USE_AUDIO
// Audio experiments :
    // simple echo (loopback)
    // #define ECHO
    // generateur
    #define GENE

#include "stm32f7xx_hal.h"
#include "stm32746g_discovery.h"
#include "bsp_audio.h"	// inclut chip_wm8994.h
#include "audio.h"
#include "stdlib.h"	// juste pour abs() !

// dma buffers
short rxbuf[AQBUF];	// stereo
short txbuf[AQBUF];	// stereo

#ifdef ECHO
// delay fifo pour la demo
short Lfifo[FQBUF];	// mono
short Rfifo[FQBUF];	// mono
#endif
int fifoW;
int fifoR;

// reglages
int mic_volume = 200;		// 0.375 dB/step, 0 = mute, FS = 239
int line_in_volume = 11;	// 1.5 dB/step, 0dB = 11, full scale 51
int out_volume = 41;		// 1dB/step, 0dB = 57, full-scale 63

// variable pour observation/debug
int halfi_cnt = 0, fulli_cnt = 0;
int halfo_cnt = 0, fullo_cnt = 0;	// cnt DMA interrupts
int peak_in_sl16 = 0;			// peak input value

/** public functions */

#ifdef ECHO
int audio_demo_init( int mic_input )
{
int retval;
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
BSP_AUDIO_OUT_Play( (uint16_t*)txbuf, AQBUF*2 );	// ridicule *2
BSP_AUDIO_IN_Record( (uint16_t*)rxbuf, AQBUF );
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
BSP_AUDIO_OUT_Play( (uint16_t*)txbuf, AQBUF*2 );	// ridicule *2
wm8994_Set_out_Volume( out_volume );
}
#endif

// 1dB/step, 0dB = 57, full-scale 63
void set_out_volume( int volume )
{
out_volume = volume;
wm8994_Set_out_Volume( out_volume );
}

/** private functions */

#ifdef ECHO
// sound processing (demo-specific) : sound source for tx
int get_next_left_sample(void)
{
int val = (int)Lfifo[ fifoR & FMASK ];
return val;
}

int get_next_right_sample(void)
{
int val = (int)Rfifo[ fifoR & FMASK ];
return val;
}

// sound processing (demo-specific) : sound sink for rx
void put_next_left_sample( short val )
{
Lfifo[ fifoW & FMASK ] = val;
if	( abs(val) > peak_in_sl16 )
	peak_in_sl16 = abs(val);
}
void put_next_right_sample( short val )
{
Rfifo[ fifoW & FMASK ] = val;
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

int get_next_right_sample(void)
{
int val = KA * iph - GEN_AMPL;
iph = (iph + 1) % GEN_PERIOD;
return -val;	// inversion de phase, c'est pour rire
}
// sound processing (demo-specific) : sound sink for rx
void put_next_left_sample( short val )
{}
void put_next_right_sample( short val )
{}
#endif

// DMA callbacks
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
for	( int i = 0; i < (AQBUF/2); i+=2 )
	{
	txbuf[i]   = 0; // get_next_left_sample();
			// cette manip bizarre sert a visualiser de "demi buffer"
	txbuf[i+1] = get_next_right_sample();
	++fifoR;
	}
halfo_cnt++;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
for	( int i = (AQBUF/2); i < AQBUF; i+=2 )
	{
	txbuf[i]   = get_next_left_sample();
	txbuf[i+1] = get_next_right_sample();
	++fifoR;
	}
fullo_cnt++;
}

void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
for	( int i = 0; i < (AQBUF/2); i+=2 )
	{
	put_next_left_sample(rxbuf[i]);
	put_next_right_sample(rxbuf[i+1]);
	++fifoW;
	}
halfi_cnt++;
}

void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
for	( int i = (AQBUF/2); i < AQBUF; i+=2 )
	{
	put_next_left_sample(rxbuf[i]);
	put_next_right_sample(rxbuf[i+1]);
	++fifoW;
	}
fulli_cnt++;
}

// DMA interrupt handlers

/* SAI handler declared in "stm32746g_discovery_audio.c" file */
extern SAI_HandleTypeDef haudio_out_sai;
/* I2S handler declared in "stm32746g_discovery_audio.c" file */
extern SAI_HandleTypeDef haudio_in_sai;

/**
  * @brief This function handles DMA2 Stream 4 interrupt request.
  */
void AUDIO_OUT_SAIx_DMAx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

/**
  * @brief This function handles DMA2 Stream 7 interrupt request.
  */
void AUDIO_IN_SAIx_DMAx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
}


#endif
