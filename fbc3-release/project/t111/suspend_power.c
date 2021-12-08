/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * suspend_power.c: impletment suspend power control and context save/store.
 */

#include <pwm.h>
#include <remote.h>
#include <customer_key_conf.h>
#include <gpio.h>
#include <sar_adc.h>


static unsigned int pin_mux_1_value = 0;

static void udelay ( int us )
{
	register int n = ( us ) * 105;

	while ( n-- )
		;
}

void power_off_at_24M(void)
{
	/*Power of backlight*/
	/*backlight_power_off_aml();*//*It will be off when panel off in main*/
	/*Power off wifi/usb 5v*/
	*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 13));
	*P_PREG_PAD_GPIO0_O &= (~(1 << 13));
	/*set POWERSTB off*/
	*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 12));
	*P_PREG_PAD_GPIO0_O |= (1 << 12);
	/*Power off T966 5v*/
	*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 1));
	*P_PREG_PAD_GPIO0_O &= (~(1 << 1));
	udelay(100);
}

void power_on_at_24M(void)
{
	/*Power on T966 5v*/
	//*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 1));/*It will be on when bootup*/
	//*P_PREG_PAD_GPIO0_O |= (1 << 1);
	//udelay(300);
	/*set POWERSTB on*/
	*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 12));
	*P_PREG_PAD_GPIO0_O &= (~(1 << 12));
	/*Power on wifi/usb 5v*/
	*P_PREG_PAD_GPIO0_EN_N &= (~(1 << 13));
	*P_PREG_PAD_GPIO0_O &= (~(1 << 13));
	/*Power on backlight*/
	//backlight_power_on_aml();/*It will be on when bootup*/
}

void power_off_at_32K(void)
{}

void power_on_at_32K(void)
{}

void save_pinmux(void)
{
	//pin_mux_1_value = *P_PERIPHS_PIN_MUX_1;
	//*P_PERIPHS_PIN_MUX_1 = 0x0;
}

void store_pinmux(void)
{
	//*P_PERIPHS_PIN_MUX_1 = pin_mux_1_value;
}

/*call device init for 24M clock*/
void prepare_suspend(void)
{/*
	led_pwm_init();
	led_bl_level_set ( 128 );
	udelay ( 20 );

	sar_adc_init();
	set_redetect_flag();
	resume_remote ( REMOTE_TYPE );*/
}

unsigned int detect_wakeup(void)
{
	INPUTDATA inputdata;
	int key_value = 0;

	do {
		udelay ( 1000 * 10 );
		/* serial_puts("."); */
		key_value = query_key_value();

		if ( customer_key_map[10][0] == key_value ) {
			break;
		}

		if ( !detect_adc_key ( 1, &inputdata ) ) {
			if ( inputdata.input_type == 0 ) {
				break;
			}
		}
	} while ( 1 );

	return 0;
}


