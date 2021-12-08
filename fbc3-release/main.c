#include <string.h>
#include <malloc.h>
#include <common.h>
#include <task.h>
#include <timer.h>
#include <input.h>
#include <remote.h>
#include <uart_api.h>
#include <version.h>
#include <log.h>
#include <i2c.h>
#include <XYmemoryMapping.h>
#include <pwm.h>
#include <customer_key_conf.h>
#include <panel.h>
#include <reboot.h>
#include <user_setting.h>
#include <ui.h>
#include <sar_adc.h>
#include <vpp.h>
#include <spi_flash.h>
#include <user_setting.h>
#include <hdmirx.h>
#include <vpu_util.h>
#include <project.h>
#include <v_protocol.h>
#include <board_config.h>

#ifdef ENABLE_LOCAL_DIMMING
	#include <ldim_drv.h>
#endif

#define __TEST_RUNNING_ON_SPI_CODE__
/* #define CLK_TEST */

#ifdef __TEST_RUNNING_ON_SPI_CODE__

void __attribute__ ( ( section ( ".running.on.spi" ) ) ) test_section_attr()
{
	printf ( "test_section_attr is running on spi\n" );
	return;
}

#endif

static const char TAG[] = "main";

extern int i2s_audio_init ( void );

/* static __attribute__((section("check.info")))
 char check_info[CHECK_INFO_SIZE]="xxxxxxxxxxxxxxxxx"; */

int main ( int argc, char *argv[] )
{
	char *p = NULL;
	int cur_id = 0;
	int tmp_val = 0;
	/* lvds_phy_disable(); */
	unsigned boot_flag = get_boot_flag();
	set_boot_flag ( REBOOT_FLAG_FROM_WATCHDOG );
	disable_watchdog();
	set_watchdog_threshold ( 50000 ); //50000*128us=6.4s
	enable_watchdog();
	reset_watchdog();
	init_configures();
	printf ( "%s\n\n", FBC_BOOT_VERSION );
	set_boot_stage ( MAIN_STAGE );
	printf ( "fbc main code version:\n" );
	print_build_version_info();
	io_init();
	printf ( "Power on.\n" );
	power_on_aml();
	printf ( "Enter main.\n" );
	/*
	p = calloc ( 1024, 1 );
	printf ( "calloc returned %x\n", ( unsigned ) p );

	if ( p ) {
		strcpy ( p, "Hello World!\n" );
		printf ( " %x:%s", ( unsigned ) p, p );
	}
	*/
	printf ( "Init spi flash.\n" );
	init_spi_flash();
#ifdef __TEST_RUNNING_ON_SPI_CODE__
	test_section_attr();
	test_running_spi_code ( 12345 );
#endif
	printf ( "Init timer.\n" );
	init_timer();
	printf ( "Init task.\n" );
	InitTask();
	printf ( "Init uart.\n" );
	console_enable();

	printf("v_protocol_init\n");
	v_protocol_init();

	printf("Init log.\n");
	init_log();
	printf ( "Init user setting.\n" );
	nvm_init_task();
	printf ( "Init Panel info.\n" );
	panel_pre_load(); /* get pid & panel info */
	/* printf("in main phy register = 0x%08x\n",
	 (* (volatile unsigned long *)0x80030628)); */
	printf ( "Init Display.\n" );
	init_display();
	printf ( "Init Vpp.\n" );
	init_vpp();
	printf ( "Panel power on.\n" );
	panel_gpio_config();
	panel_enable();
	init_load_user_setting();
	printf ( "Init OSD.\n" );
	init_ui();
	card_system_pw();
	/* printf("set bri con sat hue & wb.\n"); */
	/* vpu_pq_set(); */
	/* mdelay(400); */
#if CONFIG_ENABLE_REMOTE
	printf ( "Init remote.\n" );
	remote_init();
	set_remote_mode ( REMOTE_TYPE );
#endif
#if CONFIG_ENABLE_SARADC
	printf ( "Init saradc.\n" );
	sar_adc_init();
#endif

	printf ( "Init i2s audio\n" );
	int ret = i2s_audio_init();

	if ( ret < 0 ) {
		printf ( "Init i2s audio failed\n" );
	}

	printf ( "Init debug task.\n" );
	dbg_task_init();
	printf ( "Init led_out PWM.\n" );
	/* led_pwm_init(); */
	/* led_bl_level_set(128); */
	printf ( "Start Vpp.\n" );
	start_vpp();

#ifdef ENABLE_AUTO_BACKLIGHT
	printf ( "Start auto_backlight\n" );
	opc_task_init();
#endif

	printf("Init key func\n");
	registKeyProcess(KeyFunc);

	printf("Enter task main loop.\n");
#ifdef CLK_TEST
	printf
	( "**********************clock**********************************\n" );
	unsigned int vx1_clk = hdmirx_get_clock ( 33 );
	unsigned int vid_clk = hdmirx_get_clock ( 30 );
	int test_ok = 0;

	if ( ( ( vx1_clk / 1000 ) == 292 ) && ( ( vid_clk / 1000 ) == 585 ) ) {
		test_ok = 1;
	}

	printf ( "vx1_fifo_clk = %d\n", vx1_clk );
	printf ( "vid_pll_clk = %d\n", vid_clk );

	/* printf("vx1 %d\n",(vx1_clk / (1000000))); */
	/* printf("vid %d\n",(vid_clk / (1000000))); */
	if ( 1 == test_ok ) {
		printf
		( "OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK OK\n" );

	} else {
		printf
		( "Failed Failed Failed Failed Failed Failed Failed\n" );
	}

	printf
	( "*************************************************************\n" );
#endif
	/* register tasks */
	/* int uart_console_task_id =
	 RegisterTask(uart_console, NULL, 1<<IRQ_UART_A, 1); */
	/* int audio_in_task_id =
	 RegisterTask(audio_in_process, NULL, 1<<IRQ_I2S_IN, 8); */
	/* int led_task_id =
	 RegisterTask(led_precess, 1<<IRQ_TIMER, 15); */
	if (( boot_flag > REBOOT_FLAG_FROM_WATCHDOG )
	&& ( boot_flag <= REBOOT_FLAG_FROM_MAIN_WATCHDOG )) {
		printf ( "error: %dth reboot from main watchdog!\n", REBOOT_FLAG_FROM_MAIN_WATCHDOG - boot_flag + 1 );
		set_boot_flag ( boot_flag - 1 );
	}
	else
		set_boot_flag ( REBOOT_FLAG_FROM_MAIN_WATCHDOG );
	disable_watchdog();
	set_watchdog_threshold ( 23438 ); //3sec
	enable_watchdog();
	reset_watchdog();
	MainTask();
	return 0;
}
