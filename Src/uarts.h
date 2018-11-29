#ifdef __cplusplus
extern "C" {
#endif

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
