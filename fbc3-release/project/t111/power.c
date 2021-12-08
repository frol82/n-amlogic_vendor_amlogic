/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * power.c: impletment power control.
 */
#include <serial.h>
#include <gpio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <board_config.h>


static void udelay ( int us )
{
	register int n = ( us ) * 105;

	while ( n-- )
		;
}

static void mdelay ( int ms )
{
	udelay ( 1000 * ms );
}


void power_on_aml ( void )
{
	*P_PREG_PAD_GPIO0_EN_N |= ( 1 << 11 );

	if ( *P_PREG_PAD_GPIO0_I & ( 1 << 11 ) ) {
		serial_puts ( "Not detect P311!!!" );
	}

	if ( strcmp ( switch_p, "PA" ) == 0 ) {
		//112

	} else {
		// 111 966 power on
		// 311 power on
		*P_PREG_PAD_GPIO0_EN_N &= ( ~ ( 1 << 1 ) );
		*P_PREG_PAD_GPIO0_O |= ( 1 << 1 );
	}
	//mdelay ( 500 );
	//*P_PREG_PAD_GPIO0_EN_N &= ( ~ ( 1 << 12 ) );
	//*P_PREG_PAD_GPIO0_O &= ( ~ ( 1 << 12 ) );
}


void card_system_pw(void)
{
	//p311 card system power
	if ( strcmp ( switch_p, "PA" ) == 0 ) {
		//112

	} else {
		mdelay ( 400 );
		// 111 stand by
		*P_PREG_PAD_GPIO0_EN_N &= ( ~ ( 1 << 12 ) );
		*P_PREG_PAD_GPIO0_O &= ( ~ ( 1 << 12 ) );
	}
}

void set_led_onoff ( unsigned char vcValue )
{
//todo if need
}

