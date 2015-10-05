void init_uart(void);
uint8_t uart_grab(void);
uint8_t uart_put(uint8_t c);
uint16_t uart_tx_empty(void);
uint16_t uart_rx_vol(void);
uint16_t uart_grab16(void);
void uart_put16(uint16_t w);

#define RXBUFFSIZE 32
#define TXBUFFSIZE 32
#define BUFF_FULL 0
#define BUFF_NOMINAL 1
