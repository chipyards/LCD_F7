
// sample frequ
#define FSAMP 44100

// dma buffers
#define AQBUF 1024	// en short (stereo buffer)
#define DMA_PER_SEC	(FSAMP/(AQBUF/2)) // buffers entiers par s.

// delay fifo pour la demo
#define FQBUF (1<<16)	// >1s à 48kHz
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
