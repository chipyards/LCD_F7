
// sample frequ
#define FSAMP 44100

// dma buffers
#define AQBUF 16	// en frames (stereo buffer)
#define DMA_PER_SEC	(FSAMP/AQBUF) // buffers entiers par s. (mais il y a 2 DMA irq par buffer)

// double buffer audio pour SDCard et echo
// pour eviter 1 copie, le driver SD prendra directement le donnees dans Sfifo, header inclus
//#define FQBUF (1<<15)	// en frames (stereo buffer) (1<<15) <==> 0.74s @ 44.1kHz <==> 128 kbytes (sur total 320!)
#define FQBUF  16384	// en frames (stereo buffer) 2 clusters de 32kbytes
#define FQHEAD 192	// header en ints, insere au debut de chaque moitie du buffer
			// nombre d'audio frames = FQBUF - 2*FQHEAD, DOIT etre multiple de AQBUF mais pas puissance de 2
			// e.g. 16000 frames < 2 SD clusters; chaque cluster contient 8000 frames = 181.4ms @ 44.1kHz

// AUDIO buffers
// ne marche que si les buffers DMA sont AVANT les FIFOS
// sinon, corruption du signal par un signal aleatoire quantifie sur 8 samples
typedef struct {
int txbuf[AQBUF];	// DMA, stereo entrelace, AQBUF frames
int rxbuf[AQBUF];	// DMA, stereo entrelace
int Sfifo[FQBUF];	// FIFO stereo entrelace, FQBUF frames
int fifoW;	// index de frame dans abuf.Sfifo pour ecriture par codec
int fifoR;	// index de frame dans abuf.Sfifo pour lecture echo par codec
int fifoRSD;	// index de frame dans abuf.Sfifo pour lecture echo par SDCard recorder
int left_peak;	// vu-metre : peak input value (16 bits left_aligned)
int right_peak;
int iside;	// index sideband buffer (portion du header de cluster), par unites de 4 bits
} AUDIObuffers_type;

extern AUDIObuffers_type audio_buf;

// variable pour observation/debug
extern int halfi_cnt, fulli_cnt;
extern int halfo_cnt, fullo_cnt;	// cnt DMA interrupts

/** public functions */

int audio_demo_init( int mic_input );

void audio_start(void);

// 1dB/step, 0dB = 57, full-scale 63
void set_out_volume( int volume );

// 1.5 dB/step, 0dB = 11, full scale 51
void set_line_in_volumeL( int volume );
void set_line_in_volumeR( int volume );

// 0.375 dB/step, 0 = mute, FS = 239
void set_mic_volume( int volume );

int get_out_volume(void);
int get_line_in_volumeL(void);
int get_line_in_volumeR(void);
int get_mic_volume(void);

