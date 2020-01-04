
// sample frequ
#define FSAMP 44100

// dma buffers
#define AQBUF 256	// en short (stereo buffer)
#define DMA_PER_SEC	(FSAMP/(AQBUF/2)) // buffers entiers par s.

// delay fifo pour la demo
#define FQBUF (1<<15)	// (1<<15) <==> 0.74s à 44.1kHz
#define FMASK (FQBUF-1)

// variable pour observation/debug
extern int halfi_cnt, fulli_cnt;
extern int halfo_cnt, fullo_cnt;	// cnt DMA interrupts
extern int peak_in_sl16;		// peak input value

/** public functions */

int audio_demo_init( int mic_input );

void audio_start(void);

// 1dB/step, 0dB = 57, full-scale 63
void set_out_volume( int volume );

// 1.5 dB/step, 0dB = 11, full scale 51
void set_line_in_volume( int volume );

// 0.375 dB/step, 0 = mute, FS = 239
void set_mic_volume( int volume );

int get_out_volume(void);
int get_line_in_volume(void);
int get_mic_volume(void);

