
#ifndef _UART_API_H_
#define _UART_API_H_

int uart_ports_open ( unsigned uart_port_num );
int uart_ports_cloe ( unsigned uart_port_num );
unsigned  uart_ports_read ( unsigned uart_port_num, unsigned char *read_buf, unsigned length ) ;
unsigned uart_ports_read_till_zero ( unsigned uart_port_num, unsigned char *read_buf, unsigned int buf_size );
void uart_ports_write ( unsigned uart_port_num, unsigned char *write_buf, unsigned length ) ;
//0 disable;1 enable
void UartDebugEnable ( unsigned char ucVal );
unsigned char GetUartDebugStatus ( void );
//0 disable;1 enable
void Set_0a_to_0d0a_enable ( unsigned char ucVal );
//0 disable;1 enable
void uart_print_enable ( unsigned char ucValue );
//0 disable;1 enable
void uart_console_hd_enable ( unsigned char ucValue );

#define DEVICE_UART_PORT_0       0
#define DEVICE_UART_PORT_1       1
#define DEVICE_UART_PORT_2       2


#endif
