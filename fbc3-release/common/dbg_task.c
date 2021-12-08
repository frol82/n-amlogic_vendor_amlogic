#include <common.h>
#include <log.h>
#include <listop.h>
#include <inputdata.h>
#include <task.h>
#include <input.h>
#include <malloc.h>
#include <command.h>
#include <cmd.h>
#include <spi_flash.h>
#include <task_priority.h>
#include <reboot.h>
#include <user_setting.h>
#include <panel.h>
#include <string.h>
#include <stdlib.h>

#include <version.h>
#include <board_config.h>
#ifdef CONFIG_CUSTOMER_PROTOCOL
#include <handle_cmd.h>
#endif
LIST_HEAD ( dbg_list );
int dbg_task_id = -1;

#ifndef CONFIG_CUSTOMER_PROTOCOL
static int check_dbg_cmd_is_supported ( int cmd )
{
	switch ( cmd ) {
		case FBC_USER_SETTING_DEFAULT:
		case FBC_USER_SETTING_SET:
		case FBC_GET_HDCP_KEY:
		case FBC_REBOOT_UPGRADE:
		case CMD_SET_DEVICE_ID:
		case CMD_DEVICE_ID:
		case CMD_SET_FACTORY_SN:
		case CMD_GET_FACTORY_SN:
		case CMD_COMMUNICATION_TEST:
		case CMD_CLIENT_TYPE:
		case CMD_DEVICE_NUM:
		case CMD_ACTIVE_KEY:
		case CMD_ACTIVE_STATUS:
		case CMD_DBG_REGISTER_ACCESS:
		case CMD_DBG_MEMORY_ACCESS:
		case CMD_DBG_SPI_ACCESS:
		case CMD_DBG_VPU_MEMORY_ACCESS:
		case CMD_DBG_MEMORY_TRANSFER:
		case CMD_PANEL_INFO:
		case FBC_PANEL_POWER:
		case FBC_SUSPEND_POWER:
		case CMD_FBC_MAIN_CODE_VERSION:
		case CMD_CLR_SETTINGS_DEFAULT:
		case CMD_SET_PROJECT_SELECT:
		case CMD_GET_PROJECT_SELECT:
		case CMD_SET_AUTO_BACKLIGHT_ONFF:
		case CMD_GET_AUTO_BACKLIGHT_ONFF:
		case FBC_REBOOT_UPGRADE_AUTO_SPEED:
			return 1;

		default:
			return 0;
	}
}

static unsigned int handle_dbg_cmd ( unsigned char *s, int *rets )
{
	int i = 0, tmp_val = 0, tmp_ind = 0;
	unsigned char tmp_ch = 0;
	char *tmp_ptr = NULL;
	int *params = NULL;

	if ( s == NULL ) {
		return -1;
	}

	switch ( *s ) {
		case FBC_REBOOT_UPGRADE:
			params = GetParams ( s );

			if ( params != NULL ) {
				panel_disable();
				reboot ( ( unsigned ) params[0] );
				free ( params );
				params = NULL;
			}

			break;
		case FBC_REBOOT_UPGRADE_AUTO_SPEED:
			params = GetParams ( s );

			if ( params != NULL ) {
				panel_disable();

				save_custom_uart_params( 2, 0 );
				save_custom_uart_params( 2, (unsigned) params[1]);

				printf ("%s(), BaudRate:%d\n", __func__, (unsigned) params[1]);

				reboot ( ( unsigned ) params[0] );
				free ( params );
				params = NULL;
			}
			break;
		case FBC_USER_SETTING_DEFAULT:
			load_default_user_setting ( 0 );
			break;

		case FBC_USER_SETTING_SET:
			break;

		case FBC_GET_HDCP_KEY:
			char *key = nvm_read_hdcpkey();

			if ( key != NULL ) {
				for ( i = 0; i < Ret_NumParam ( s ); i++ ) {
					rets[i] = key[i];
				}

				free ( key );
			}

			break;

		case FBC_PANEL_POWER:
			params = GetParams ( s );

			if ( params != NULL ) {
				if ( 0 == params[0] ) {
					panel_disable();

				} else if ( 1 == params[0] ) {
					panel_enable();
				}

				free ( params );
				params = NULL;
			}

			break;

		case FBC_SUSPEND_POWER:
			panel_disable();
			reboot ( REBOOT_FLAG_SUSPEND );
			break;

		case CMD_SET_DEVICE_ID:
			char *device = ( char * ) malloc ( 33 );
			memset ( device, 0, 33 );

			for ( int i = 0; i < 32; i++ ) {
				device[i] = s[i + 1];
			}

			nvm_write_device_id ( device );
			free ( device );
			break;

		case CMD_SET_FACTORY_SN:
			char *pFactorySn = ( char * ) malloc ( 512 );

			if ( pFactorySn != NULL ) {
				memset ( pFactorySn, 0, 512 );

				for ( int j = 0; j < 512; j++ ) {
					if ( s[j + 1] == '\0' ) {
						pFactorySn[j] = '\0';
						break;
					}

					pFactorySn[j] = s[j + 1];
				}

				pFactorySn[j] = '\0';
				nvm_write_factory_sn ( pFactorySn );
				free ( pFactorySn );
				pFactorySn = NULL;
			}

			break;

		case CMD_GET_FACTORY_SN:
			char *pReadFactorySn;
			pReadFactorySn = nvm_read_factory_sn();
			int len = strlen ( pReadFactorySn );
			printf ( "\nget %d sn = %s\n", len, pReadFactorySn );

			if ( NULL == rets ) {
				printf ( "rets is NULL\n" );
				break;
			}

			rets[0] = 0;

			for ( i = 0; i < len; i++ ) {
				rets[i] = pReadFactorySn[i];
			}

			break;

		case CMD_COMMUNICATION_TEST:
			tmp_val = Ret_NumParam ( s );

			for ( i = 0; i < tmp_val; i++ ) {
				tmp_ch = s[i + 1];
				tmp_ch = ( ( tmp_ch & 0x55 ) << 1 ) | ( ( tmp_ch & 0xAA ) >> 1 );
				tmp_ch = ( ( tmp_ch & 0x33 ) << 2 ) | ( ( tmp_ch & 0xCC ) >> 2 );
				tmp_ch = ( ( tmp_ch & 0x0F ) << 4 ) | ( ( tmp_ch & 0xF0 ) >> 4 );
				rets[i] = tmp_ch;
			}

			break;

		case CMD_CLR_SETTINGS_DEFAULT:
			params = GetParams ( s );

			if ( params != NULL ) {
				if ( params[0] == 0x1 ) {
					load_default_user_setting ( 1 );

				} else if ( params[0] == 0x2 ) {
					clr_default_wb_setting();

				} else if ( params[0] == 0x3 ) {
					load_default_user_setting ( 1 );
					clr_default_wb_setting();

				} else {
					load_default_user_setting ( 1 );
					clr_default_wb_setting();
				}

				free ( params );
				params = NULL;

			} else {
				load_default_user_setting ( 1 );
				clr_default_wb_setting();
			}

			break;

		case CMD_FBC_MAIN_CODE_VERSION:
			if ( rets == NULL ) {
				printf ( "return buffer is null!!!\n" );
				break;
			}

			tmp_ind = 0;
			rets[tmp_ind++] = 0x88;
			rets[tmp_ind++] = 0x99;
			tmp_ptr = ( char * ) fbc_get_version_info();
			tmp_val = strlen ( tmp_ptr );

			for ( i = 0; i < tmp_val; i++ ) {
				rets[tmp_ind++] = tmp_ptr[i];
			}

			rets[tmp_ind++] = 0;
			tmp_ptr = ( char * ) fbc_get_build_time_info();
			tmp_val = strlen ( tmp_ptr );

			for ( i = 0; i < tmp_val; i++ ) {
				rets[tmp_ind++] = tmp_ptr[i];
			}

			rets[tmp_ind++] = 0;
			tmp_ptr = ( char * ) fbc_get_git_version_info();
			tmp_val = strlen ( tmp_ptr );

			for ( i = 0; i < tmp_val; i++ ) {
				rets[tmp_ind++] = tmp_ptr[i];
			}

			rets[tmp_ind++] = 0;
			tmp_ptr = ( char * ) fbc_get_git_branch_info();
			tmp_val = strlen ( tmp_ptr );

			for ( i = 0; i < tmp_val; i++ ) {
				rets[tmp_ind++] = tmp_ptr[i];
			}

			rets[tmp_ind++] = 0;
			tmp_ptr = ( char * ) fbc_get_build_name_info();
			tmp_val = strlen ( tmp_ptr );

			for ( i = 0; i < tmp_val; i++ ) {
				rets[tmp_ind++] = tmp_ptr[i];
			}

			rets[tmp_ind++] = 0;
			break;

		case CMD_DEVICE_ID:
			char *device_id;
			device_id = nvm_read_device_id();
			int n = strlen ( device_id );
			printf ( "deviceid=%s\n", device_id );

			for ( i = 0; i < n; i++ ) {
				rets[i] = device_id[i];
			}

			rets[n] = 0;
			break;

		case CMD_CLIENT_TYPE:
			break;

		case CMD_DEVICE_NUM:
			break;

		case CMD_ACTIVE_KEY:
			params = GetParams ( s );

			if ( params != NULL ) {
				AddIputdataToTaskList ( INPUT_REMOTE, params[0] );
				free ( params );
				params = NULL;
			}

			break;

		case CMD_ACTIVE_STATUS:
			break;

		case CMD_DBG_REGISTER_ACCESS:
		case CMD_DBG_MEMORY_ACCESS:
			params = GetParams ( s );

			if ( params != NULL ) {
				*rets = * ( int * ) ( ( unsigned ) params[0] & ( ~ ( 0x3 ) ) );
				free ( params );
				params = NULL;
			}

			break;

		case CMD_DBG_SPI_ACCESS:
			params = GetParams ( s );

			if ( params != NULL ) {
				printf ( "parameters are 0x%08x, 0x%08x\n", params[0], params[1] );
				unsigned offset = params[0];
				unsigned size = params[1];

				if ( size <= 0 ) {
					size = 1;
				}

				if ( size > 8 ) {
					size = 8;
				}

				printf ( "rets address is 0x%08x\n", rets );
				spi_flash_read ( get_spi_flash_device ( 0 ), offset, size * 4, ( unsigned * ) ( ( unsigned ) rets ) );
				printf ( "read spi flash ok!\n" );
				free ( params );
				params = NULL;
			}

			break;

		case CMD_DBG_VPU_MEMORY_ACCESS:
			break;

		case CMD_DBG_MEMORY_TRANSFER:
			break;

		case CMD_PANEL_INFO:
			params = GetParams ( s );

			if ( params != NULL ) {
				rets[0] = params[0];

				switch ( params[0] ) {
					case 0: /* panel reverse */
						rets[1] = panel_param->reverse;
						break;

					case 1: /* panel output_mode */
						rets[1] = panel_param->output_mode;
						break;

					case 2: /* panel byte num */
						rets[1] = panel_param->byte_num;

					default:
						break;
				}

				free ( params );
				params = NULL;
			}

			break;

		case CMD_SET_PROJECT_SELECT:
			params = GetParams ( s );

			if ( params != NULL ) {
				nvm_switch_project_id ( params[0] );
				free ( params );
			}

			break;

		case CMD_GET_PROJECT_SELECT:
			rets[0] = ( char ) nvm_read_project_id();
			break;

		case CMD_SET_AUTO_BACKLIGHT_ONFF:
			params = GetParams ( s );

			if ( params != NULL ) {
				nvm_write_nature_lihgt_en ( params[0] );
				free ( params );
			}

			break;

		case CMD_GET_AUTO_BACKLIGHT_ONFF:
			rets[0] = nvm_read_light_enable();
			break;

		default:
			break;
	}

	return 0;
}

static int ConsoleHandler ( int task_id, void *param )
{
	list_t *plist = list_dequeue ( &dbg_list );

	if ( plist != NULL ) {
		CMD_LIST *clist = list_entry ( plist, CMD_LIST, list );

		if ( clist != NULL ) {
			unsigned char *cmd = ( unsigned char * ) ( clist->cmd_data.data );

			if ( cmd != NULL ) {
				int rcmd_len = Ret_NumParam ( cmd );

				if ( rcmd_len > 0 ) {
					int *params = ( int * ) malloc ( rcmd_len * sizeof ( int ) );
					handle_dbg_cmd ( cmd, params );
					SendReturn ( dbg_task_id, clist->cmd_data.cmd_owner, *cmd, ( int * ) params );
					free ( params );
					params = NULL;

				} else {
					handle_dbg_cmd ( cmd, NULL );
				}
			}

			freeCmdList ( clist );
		}
	}

	return 0;
}
#endif//CONFIG_CUSTOMER_PROTOCOL

void dbg_task_init ( void )
{
	dbg_task_id = RegisterTask ( ConsoleHandler, NULL, 0, TASK_PRIORITY_DBG );
	RegisterCmd ( &dbg_list, dbg_task_id, INPUT_CEC | INPUT_UART_HOST | INPUT_UART_CONSOLE, check_dbg_cmd_is_supported, handle_dbg_cmd );
	return;
}
