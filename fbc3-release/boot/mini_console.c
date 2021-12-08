#include <serial.h>
#include <command.h>
#include <common.h>
#include <mini_console.h>

#ifndef _IN_PRE_BOOT_
	#define CONFIG_SYS_PROMPT       "fbc-boot#"
#else
	#define CONFIG_SYS_PROMPT       "pre-boot#"
#endif
#define CONFIG_SUPPORT_MULTI_SERAIL

struct serial_device *current_serial_device = 0;

#ifdef CONFIG_SUPPORT_MULTI_SERAIL
int multi_tstc ( void )
{
	struct serial_device *sdev;
	int i, n;

	for ( i = 0, n = 0; i < 2; i++ ) {
		sdev = get_serial_device ( i );
		n = sdev->tstc();

		if ( n ) {
			break;
		}
	}

	return n;
}

char multi_getc ( void )
{
	struct serial_device *sdev;
	int i;
	char c = 0;

	for ( i = 0; i < 2; i++ ) {
		sdev = get_serial_device ( i );

		if ( sdev->tstc() ) {
			default_uart = i;
			current_serial_device = default_serial_console();
			c = sdev->getc();
			break;
		}
	}

	return c;
}

#endif

void do_wait_cmd ( void )
{
	int len;
	int rc = 1;
	int flag;

	for ( ;; ) {
		len = readline ( current_serial_device, CONFIG_SYS_PROMPT );
		flag = 0; /* assume no special flags for now */

		if ( len > 0 ) {
			strcpy ( lastcommand, console_buffer );

		} else if ( len == 0 ) {
			flag |= CMD_FLAG_REPEAT;
		}

		if ( len == -1 ) {
			serial_puts ( "<INTERRUPT>\n" );

		} else {
			rc = run_command ( lastcommand, flag );
		}

		if ( rc <= 0 || len < 0 ) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
}

static void udelay ( int us )
{
	register int n = us * 105;

	while ( n-- )
		;
}

unsigned int bootdelay = CONFIG_BOOT_DELAY;

int abortboot ( int bootdelay )
{
	int abort = 0;
	char key;
#ifdef _IN_PRE_BOOT_
	serial_puts ( "Hit enter key to stop preboot\n" );
#else
	serial_puts ( "Hit enter key to stop autoboot\n" );
#endif

	if ( bootdelay >= 0 ) {
#ifdef CONFIG_SUPPORT_MULTI_SERAIL

		if ( multi_tstc() ) {
			switch ( multi_getc() )
#else
		if ( serial_tstc() ) {
			switch ( serial_getc() )
#endif
			{
#ifdef _IN_PRE_BOOT_

				/* ^C - Ctrl+C */
				case 0x03:
					abort = 1;
					break;

				case 0x20: /* Space */
					abort = 1;
					break;
#else

				case 0x0d: /* Enter */
					abort = 1;
					break;
#endif

				default:
					break;
			}
		}
	}

	/* Disable Watchdog */
	if ( abort == 1 )
		;

	while ( ( bootdelay > 0 ) && ( !abort ) ) {
		int i;
		--bootdelay;

		/* delay 100 * 10ms */
		for ( i = 0; !abort && i < 100; ++i ) {
			/* we got a key press   */
#ifdef CONFIG_SUPPORT_MULTI_SERAIL
			if ( multi_tstc() ) {
				key = multi_getc();
#else

			if ( serial_tstc() ) {
				key = serial_getc();
#endif
#ifdef _IN_PRE_BOOT_

				/* only "ctrl c or space"
				 key can triger abort */
				if ( 0x03 == key || 0x20 == key ) {
#else

				/* only "enter" key can triger abort */
				if ( 0x0d == key ) {
#endif
					abort = 1; /* don't auto boot  */
					bootdelay = 0; /* no more delay    */
					break;
				}
			}

			udelay ( 100 );
		}
	}

	return abort;
}
