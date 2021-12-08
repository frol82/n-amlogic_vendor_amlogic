#include <string.h>
#include <stdlib.h>
#include <serial.h>
#include <common.h>
#include <spi_flash.h>
#include <update.h>
#include <reboot.h>
#include <uart_api.h>
#include <relocate.h>
#include <update.h>
//#include <register.h>

int static updateui_title = 0;
int static updateui_time = 0;
int static updateui_status = 0;
int static gmsg_title_type = 0;
int static gmsg_status_type = 0;

#ifndef _USE_WITHOUT_UI_UPDATE_
	#include <timer.h>
	#include <panel.h>
	#include <osd.h>
	#include <fonts_update.h>
	#include <timer.h>
	#include <vpp.h>
#ifdef ENABLE_LOCAL_DIMMING
	#include <ldim_drv.h>
#endif
#endif

#define DEVICE_UART_PORT_0       0
#define DEVICE_UART_PORT_1       1
#define DEVICE_UART_PORT_2       2

#ifndef _USE_WITHOUT_UI_UPDATE_
int gTitleMsgHandle = -1;
int gUpdateMsgHandle = -1;
int gUpdateTimeHandle = -1;

extern void vbyone_training_Handle();

#ifdef HAVE_PRO_LOGO
void show_logo(unsigned line, unsigned position)
{
    for (int i = 0; i < LOGO_V_FONTS; i++) {
        OSD_InitialRegion(line + i, position, logo[i],
                     logo_color[i]);
    }
    printf("show_logo.\n");
}
#endif

void init_osd ( void )
{
	int i;
	/* hide_logo(); */
	OSD_Enable ( 0 );

	if ( IS_1080P ( panel_param->timing ) ) {
		OSD_Initial ( 1920, 1080, 0, 0, 1919, 1079 );
		OSD_SetFontScale ( 1, 1 );

	} else {
		OSD_Initial ( 3840, 2160, 0, 0, 3839, 2159 );
		OSD_SetFontScale ( 2, 2 );
	}

	OSD_Set3DMode ( OSD_3D_MODE_NORMAL );

	/* OSD_SetFontScale(1, 1); */
	/* OSD_SetMirror(1); */
	if ( 1 == panel_param->reverse ) {
		OSD_SetMirror ( 1 );

	} else {
		OSD_SetMirror ( 0 );
	}
#ifndef HAVE_PRO_LOGO
	OSD_SetSpacing ( 2, 2, 2, 2 );

	if ( OSD_GetMirror() == 1 ) {
		OSD_ConfigFonts ( FONT_NUM, FONT_WIDTH, FONT_HEIGHT, sosd_font_lib_lut_hvflip, font_mapping, 1 );

	} else if ( OSD_GetMirror() == 0 ) {
		OSD_ConfigFonts ( FONT_NUM, FONT_WIDTH, FONT_HEIGHT, sosd_font_lib_lut, font_mapping, 1 );
	}

	for ( i = 0; i < FONT_NUM; i++ ) {
		OSD_SetFontCut ( i, sosd_cut_table[i] );
	}

	for ( i = 0; i < 16; i++ ) {
		OSD_SetColor ( i, nRGBA[i][0], nRGBA[i][1], nRGBA[i][2], nRGBA[i][3] );
	}

	OSD_SetBackground ( 1, FONT_BG_COLOR_INDEX );
	/* osd_init = 1; */
	OSD_CleanScreen ( NULL, 0 );
	/* OSD_InitialRegionSimple(1, 1, "test update",
	 FONT_NORMAL_COLOR_INDEX, FONT_BG_COLOR_INDEX); */
	OSD_Enable ( 1 );
#else
	OSD_SetSpacing ( 0, 0, 0, 0 );
	if ( OSD_GetMirror() == 1 ) {
		OSD_ConfigFonts ( LOGO_FONT_NUM, FONT_WIDTH, FONT_HEIGHT,
			sosd_font_lib_lut_logo_hvflip, font_mapping_logo, 1 );
	} else if ( OSD_GetMirror() == 0 ) {
		OSD_ConfigFonts ( LOGO_FONT_NUM, FONT_WIDTH, FONT_HEIGHT,
			sosd_font_lib_lut_logo, font_mapping_logo, 1 );
	}

	for ( i = 0; i < LOGO_FONT_NUM; i++ )
		OSD_SetFontCut ( i, sosd_cut_table_logo[i] );

	for ( i = 0; i < 16; i++ ) {
		OSD_SetColor ( i, nRGBA_logo[i][0], nRGBA_logo[i][1],
			nRGBA_logo[i][2], nRGBA_logo[i][3] );
	}
	OSD_SetBackground ( 1, 0 );
	/* Delay_ms(20); */
	OSD_CleanScreen ( NULL, 0 );
	/* Delay_ms(20); */
	show_logo ( 12, 705 );
	OSD_Enable ( 1 );

#endif
}

int show_msg ( int *ui_handle, int msg_type, int line, int row, const char *msg )
{
	int forecolor = FONT_NORMAL_COLOR_INDEX;
	int bgcolor = FONT_BG_COLOR_INDEX;

	if ( msg_type == 0 ) {
		forecolor = FONT_NORMAL_COLOR_INDEX;

	} else if ( msg_type == 1 ) {
		forecolor = FONT_HILIGHT_COLOR_INDEX;
	}

	if ( *ui_handle < 0 ) {
		*ui_handle = OSD_InitialRegionSimple ( line, row, ( char * ) msg, forecolor, FONT_BG_COLOR_INDEX );

	} else {
		OSD_UpdateRegionContentSimple ( *ui_handle, ( char * ) msg, forecolor, FONT_BG_COLOR_INDEX );
	}

	return 0;
}


char msg_buf[256];
char msg_buf2[256];
extern unsigned int strnlen ( const char *s, unsigned int count );
extern void stringcopy(const char *s,char *d);


int show_title_msg ( int msg_type, const char *msg )
{
#if 0
	gmsg_title_type = msg_type;
	stringcopy(msg,msg_buf);
	updateui_title = 1;
	return 1;
#else
	return show_msg ( &gTitleMsgHandle, msg_type, 0, 100, msg );
#endif
}

int show_update_msg ( int msg_type, const char *msg )
{
#if 1
	//update in vs
	stringcopy(msg,msg_buf2);
	gmsg_status_type = msg_type;
	updateui_status = 1;
	return 1;//show_msg ( &gUpdateMsgHandle, msg_type, 3, 100, msg );
#else
	return show_msg ( &gUpdateMsgHandle, msg_type, 3, 100, msg );
#endif
}

#define CLOCKS_PER_SECOND (1000000)

int show_update_timer_step1( unsigned int tm_lapse )
{
	tm_lapse = tm_lapse / CLOCKS_PER_SECOND;
	sprintf ( msg_buf, "Time lapse: %02d:%02d:%02d", tm_lapse / 3600, tm_lapse / 60, tm_lapse % 60 );
}

int show_update_time ( unsigned int tm_lapse )
{
#if 0
	tm_lapse = tm_lapse / CLOCKS_PER_SECOND;
	sprintf ( msg_buf, "Time lapse: %02d:%02d:%02d", tm_lapse / 3600, tm_lapse / 60, tm_lapse % 60 );
	return show_msg ( &gUpdateTimeHandle, 0, 5, 100, msg_buf );
#else
	//update in vs
	show_update_timer_step1(tm_lapse);
	return show_msg ( &gUpdateTimeHandle, 0, 5, 100, msg_buf );
#endif
}

#if 0
void fn ( void )
{
	unsigned char buf[100];
	int i;
	serial_puts ( "\nenter fn\n" );
	memset ( buf, 0, sizeof ( buf ) );
	int len = uart_ports_read ( DEVICE_UART_PORT_1, buf, 10 );

	if ( len > 0 ) {
		serial_puts ( "\nlen > 0\n" );
	}

	for ( i = 0; i < len; i++ ) {
		serial_putc ( buf[i] );
	}

	/* destory_timer(TIMERA_INDEX, fn); */
}
#endif

#else

int show_update_msg ( int msg_type, const char *msg )
{
	return 0;
}

void udelay ( int us )
{
	int n = us * ( 80 / 3 );

	while ( n-- )
		;
}

void mdelay ( int ms )
{
	udelay ( 4000 * ms );
}

#endif

static void vs_interrupt_handle ( void *arg )
{
	if ( updateui_time ) {
		updateui_time = 0;
		show_msg ( &gUpdateTimeHandle, 0, 5, 100, msg_buf );
		return;
	}

	if ( updateui_status ) {
		updateui_status = 0;
		show_msg ( &gUpdateMsgHandle, gmsg_status_type, 3, 100, msg_buf2 );
		return;
	}

	if (updateui_title) {
		updateui_title = 0;
		show_msg ( &gTitleMsgHandle, gmsg_title_type, 0, 100, msg_buf );
		return;
	}
}

void interrupt_init(void)
{
	int ret;

	#define INT_VPU_VSYNC	0x10

	ret = RegisterInterrupt ( INT_VPU_VSYNC, INT_TYPE_IRQ, ( interrupt_handler_t ) vs_interrupt_handle );

	if ( ret == 0 ) {
		SetInterruptEnable ( INT_VPU_VSYNC, 1 );
		printf("vs interrupt_init \n");
	}
}

int main ( int argc, char *argv[] )
{
	unsigned int tm_start = 0, tm_lapse = 0;
	int tmp_val = 0;
	unsigned int boot_flag = get_boot_flag();
	update_ctrl_t update_ctrl;

	enable_custom_baudRate(1);

	serial_init ( CONSOLE_CHANNEL_DEV );
	serial_puts ( "serial uart port 0 init success!\n" );

	struct customParams_t customParams = get_custom_uart_params();
	printf ("%s(), Custom baudRate is %u\n", __func__, customParams.baudRate);

	serial_dev = get_serial_device ( CONSOLE_CHANNEL_DEV );
	serial_puts ( "init spi flash!\n" );
	init_spi_flash();
	printfx ( "bpflag = 0x%x\n", boot_flag );
#ifndef _USE_WITHOUT_UI_UPDATE_
	serial_puts ( "Power on.\n" );
	power_on_aml();
	serial_puts ( "Init Panel info.\n" );
	panel_pre_load(); /* get pid & panel info */
	serial_puts ( "Init Vpp.\n" );
	init_vpp();
	interrupt_init();
	card_system_pw();
	init_osd();
#ifndef HAVE_PRO_LOGO
	show_title_msg ( 0, "Updating..." );
#endif
	panel_enable();
#if 0
	uart_ports_open ( DEVICE_UART_PORT_1 );
	create_timer ( TIMERA_INDEX, TIMER_TICK_1MS, 10, fn ); /* 10ms */
#endif
#ifndef HAVE_PRO_LOGO
	show_update_msg ( 0, "starting to update fbc." );
	tm_start = OSTimeGet();
	show_update_time ( 0 );
#endif
#endif
	init_update_ctrl_t ( &update_ctrl );
	int ret = 1;
	serial_puts ( "enter while.\n" );
	//backup section 0 to section 1
	int backup_time = OSTimeGet();
	do {
		ret = section_backup(get_spi_flash_device(0));
	} while (ret);
	backup_time = OSTimeGet() - backup_time;
	printf ( "backup time = %d\n", backup_time);

	do {

		//start_lockn_flag = 1;
		if ( PANEL_IF_VBYONE == panel_param->interface ) {
			//LOCKN_IRQ_Handle_new();
			vbyone_training_Handle();
		}

		ret = handle_cmd(&update_ctrl);
#ifndef HAVE_PRO_LOGO
#ifndef _USE_WITHOUT_UI_UPDATE_
		tm_lapse = OSTimeGet() - tm_start;
		#if 0
		show_update_time ( tm_lapse );
		#else
		//update in vs interrupt
		show_update_timer_step1(tm_lapse);
		updateui_time = 1;
		#endif
#endif
#endif
	} while ( !ret );

#ifndef HAVE_PRO_LOGO
	mdelay ( 1000 );
	show_update_msg ( 0, "Update done." );
#endif
	mdelay ( 1000 );
	reboot ( REBOOT_FLAG_FROM_UPGRADE );
	return 0;
}
