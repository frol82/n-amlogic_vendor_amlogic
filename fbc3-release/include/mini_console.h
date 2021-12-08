#ifndef __FBC_MINI_CONSOLE_H__
#define __FBC_MINI_CONSOLE_H__

void do_wait_cmd();
int abortboot ( int bootdelay );

extern unsigned int bootdelay;
extern struct serial_device *current_serial_device;
extern void do_wait_cmd ( void );

extern char console_buffer[MAX_CONSOLE_BUF_SIZE];
extern char lastcommand[MAX_CONSOLE_BUF_SIZE];

#endif	//__FBC_MINI_CONSOLE_H__