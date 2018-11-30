#define QTX1 128

// le contexte pour UART1
// TX : un buffer de message avec lock
// RX : un byte de commande
typedef struct {
volatile char TXbuf[QTX1];
volatile int TXindex;
volatile int RXbyte;
} UART1type;

extern UART1type CDC;	// pour communiquer avec PC via ST-Link 

#ifdef __cplusplus
extern "C" {
#endif

// haut niveau : objet CDC

// constructeur
void CDC_init(void);

// envoyer une ligne de texte formattee
// retourne 1 si renoncement pour cause de transmission en cours
int CDC_print( const char *fmt, ... );

// lire une commande
int CDC_getcmd(void);

// bas niveau : UARTs

// UART1 pour CDC
void UART1_init( unsigned int bauds );
void UART1_TX_INT_enable(void);
void UART1_TX_INT_disable(void);

// UART6 pour Arduino
void UART6_init( unsigned int bauds );
void UART6_TX_INT_enable(void);
void UART6_TX_INT_disable(void);

#ifdef __cplusplus
}
#endif
