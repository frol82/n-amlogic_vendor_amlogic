#include <string.h>
#include <common.h>
#include <command.h>
#include <reboot.h>
#include <panel.h>

int do_reboot ( cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[] )
{
	serial_puts ( "enter reboot\n" );

	save_custom_uart_params( 0, 0 );

	if ( argc == 1 ) {
		return reboot ( 0 );

	} else {
		if ( argc >= 3 ) {
			if ( !strcmp ( "-r", argv[1] ) ) {
				if ( !strcmp ( "upgrade", argv[2] ) ) {
					unsigned stage = get_boot_stage();
					unsigned reson;

/*
					if ( argc > 3 && !strcmp ( "uart1", argv[3] ) ) {
						FIR_BOOT_STAGE == stage ? reson = REBOOT_FLAG_LITE_UPGRADE2 : reson = REBOOT_FLAG_UPGRADE2;
#ifdef IN_FBC_MAIN_CONFIG
						panel_enable();
#endif
						return reboot ( reson );
					}
*/

					if ( argc > 3 )
						save_custom_uart_params(0, atoi(argv[3]));

					if ( FIR_BOOT_STAGE == stage ) {
						reson = REBOOT_FLAG_LITE_UPGRADE1;

					} else {
						reson = REBOOT_FLAG_UPGRADE1;
					}

#ifdef IN_FBC_MAIN_CONFIG
					panel_enable();
#endif
					return reboot ( reson );

				} else if ( !strcmp ( "upgrade_lite", argv[2] ) ) {
					unsigned stage = get_boot_stage();

					if ( MAIN_STAGE == stage ) {
#ifdef IN_FBC_MAIN_CONFIG
						panel_enable();
#endif
					}

					return reboot ( REBOOT_FLAG_LITE_UPGRADE1 );

				} else if ( !strcmp ( "suspend", argv[2] ) ) {
					return reboot ( REBOOT_FLAG_SUSPEND );

				} else if ( !strcmp ( "sw_reboot", argv[2] ) ) {
					return reboot_sw ( REBOOT_FLAG_NORMAL );
				}

#ifndef IN_FBC_MAIN_CONFIG

				else if ( !strcmp ( "loadu", argv[2] ) ) {
					jump_0();
					/* *P_PREG_STICKY_REG0 = 0x80808080; */
					return 0x80808080;

				} else if ( !strcmp ( "loadm", argv[2] ) ) {
					jump_0();
					return 0;
				}

#endif
			}
		}
	}

	return cmd_usage ( cmdtp );
}
