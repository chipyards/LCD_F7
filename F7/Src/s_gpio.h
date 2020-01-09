// carte disco F7 LCD
#ifdef __cplusplus
extern "C" {
#endif

// configuration des GPIO
void GPIO_config_bouton(void);
void GPIO_config_uart1(void);
void GPIO_config_uart6(void);
void GPIO_config_profiler_PI1_PI2( void );
void GPIO_config_SDCard( void );
// divers I/O
int GPIO_bouton_bleu(void);
int GPIO_SDCARD_present(void);
// action bits profiler
void profile_D13( int val );
void profile_D8( int val );

#ifdef __cplusplus
}
#endif
