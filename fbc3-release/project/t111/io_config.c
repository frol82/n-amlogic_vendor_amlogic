/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * io_config.c: impletment io control.
 */
#include <serial.h>
#include <gpio.h>

#include <string.h>
#include <malloc.h>
#include <common.h>
#include <task.h>
#include <timer.h>
#include <uart_api.h>
#include <customer_key_conf.h>
#include <user_setting.h>
#include <ui.h>
#include <panel.h>
#include <sar_adc.h>
#include <user_setting.h>
#include <project.h>
#include <hdmirx.h>
#include <hdmirx_parameter.h>
#include <board_config.h>


void io_init(void)
{
	printf("io_init\n");
}

void panel_gpio_config(void) {
	if ( panel_param->panel_id == PANEL_ID_4K_SAMSUNG_LSC550FN04) {
		/* code */
		/*add gpio6 control for panel*/
		( *P_PERIPHS_PIN_MUX_3 ) &= ( ~( 1 << 6 ) );
		( *P_PREG_PAD_GPIO3_EN_N ) &= ( ~( 1 << 6 ));
		( *P_PREG_PAD_GPIO3_O ) |= ( 1 << 6 );
	} else if( panel_param->panel_id == PANEL_ID_4K_INNOLUX_V400DJ1) {
		/* code */
	}
}

char io_hdmi_5v_pw ( void )
{
	return hdmi_5v_pw();
}


/* set hpd mode  0: GPIO low 1:GPIO high 2:input mode */
void hdmi_rx_hpd(int HighLow)
{
	if (HighLow == HDMI_HPD_HIGH) {
		( *P_PERIPHS_PIN_MUX_2 ) |= ( 1 << 3 );//pm_gpioB_3_hdmirx_hpd
		if ( HDMIRX_HPD_LVL == HDMIRX_HPD_LOW ) {
			hdmirx_wr_top ( HDMIRX_TOP_HPD_PWR5V, 0x10 );

		} else {
			hdmirx_wr_top ( HDMIRX_TOP_HPD_PWR5V, 0x11 );
		}
	} else if (HighLow == HDMI_HPD_LOW) {
		( *P_PERIPHS_PIN_MUX_2 ) |= ( 1 << 3 );//pm_gpioB_3_hdmirx_hpd
		if ( HDMIRX_HPD_LVL == HDMIRX_HPD_LOW ) {
			hdmirx_wr_top ( HDMIRX_TOP_HPD_PWR5V, 0x11 );

		} else {
			hdmirx_wr_top ( HDMIRX_TOP_HPD_PWR5V, 0x10 );
		}
	} else {
		( *P_PERIPHS_PIN_MUX_2 ) &= 0xfffffff7;//pm_gpioB_3_hdmirx_hpd
	}
}


