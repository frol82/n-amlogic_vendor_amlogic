#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdarg.h>
#include <serial.h>

#ifdef IN_FBC_MAIN_CONFIG
	#ifndef MAX_CONSOLE_BUF_SIZE
		#define MAX_CONSOLE_BUF_SIZE	1024
	#endif
#else
	#ifndef MAX_CONSOLE_BUF_SIZE
		#define MAX_CONSOLE_BUF_SIZE	256
	#endif
#endif

#ifndef CONFIG_SYS_MAXARGS
	#define CONFIG_SYS_MAXARGS		10
#endif

#define TAG_VPP "VPP"
#define TAG_HDMIRX "HDMIRX"
#define TAG_VBY "VBYONE"

extern void delay_us ( int  us );
int readline ( struct serial_device *dev, const char *const prompt );
int readline_into_buffer ( struct serial_device *dev, const char *const prompt, char *buffer, int *len );
int parse_line ( char *line, char *argv[] );
int debug_console ( struct serial_device *dev );

#ifndef EXTEND_BUF_ADDR
	#define EXTEND_BUF_ADDR			0x80020000
#endif

#ifndef EXTEND_BUF_SIZE
	#define EXTEND_BUF_SIZE			0x8000
#endif

#ifdef IN_FBC_MAIN_CONFIG
	int put_pq_data ( unsigned port, char *buf, unsigned len );
	int console_enable ( void );
	int console_disable ( void );
#else

	#ifndef CONFIG_BOOT_DELAY

		#ifdef _IN_PRE_BOOT_
			#define CONFIG_BOOT_DELAY 0x3
		#else
			#define CONFIG_BOOT_DELAY 0x2
		#endif

	#endif

#endif

#ifdef IN_FBC_MAIN_CONFIG
extern void dbg_task_init ( void );
extern void ref_protocol_init ( void );
extern int test_running_spi_code ( int x );
extern int pq_cmd_flag;
#else
extern struct serial_device *serial_dev;

extern "C"
{
	int printfx ( const char *__fmt, ... );
}

extern int show_update_msg ( int msg_type, const char *msg );
#endif

extern int ctrlc_flag;

extern "C" {
	int puts ( const char *__s );
	int printf2 ( const char *__fmt, ... );
	int printf_pq ( const char *__fmt, ... );
	int sprintf ( char *buffer, const char *__fmt, ... );
	int printf ( const char *__fmt, ... );
	/*common/log.c*/
	int vsprintf ( char *buf, const char *fmt, va_list args );
}

struct serial_device *get_serial_device ( int index );

extern unsigned char console_state;
extern unsigned char cmd_state;
extern int default_uart;
extern void serial_putc ( const char c );

//extern audio_control_t audio_state;


struct customParams_t
{
	unsigned port;
	unsigned baudRate;
};

extern void enable_custom_baudRate(unsigned enable);
void save_custom_uart_params(unsigned port, unsigned baudRate );
struct customParams_t get_custom_uart_params( void );


#endif

