#include <user_setting.h>
#include <task_priority.h>
#include <string.h>
#include <malloc.h>
#include <spi_flash.h>
#include <task.h>
#include <timer.h>
#include <cmd.h>
#include <vpp_api.h>
#include <inputdata.h>
#include <XYmemoryMapping.h>
#include <gpio.h>
#include <project.h>
#include <reboot.h>
#include <panel.h>
#include <crc.h>
#include <data_struct.h>

#include <vpp.h>
#include <common.h>
#include <board_config.h>
#include <relocate.h>

#ifdef CONFIG_CUSTOMER_PROTOCOL
#include <handle_cmd.h>
#endif

//extern unsigned int handle_setting_cmd ( unsigned char *s, int *rets );
extern unsigned int vpp_cmd_user_setting ( unsigned char *s );
extern unsigned char vpp_cmd_read_user_setting ( unsigned char *s, int *returns );
extern void set_lcd_clk_ss(int level);


extern const systems_t system_default;
extern const audio_control_t audio_state_def;
extern const user_pq_t user_pq_default;
extern const unsigned char default_db;

extern audio_control_t audio_state;

int gbDBCRC_OK;

LIST_HEAD ( setting_list );

//struct systems cur_system;


//struct wb_setting g_wb_setting = { 0 };

//struct user_setting g_user_setting = { 0 };

nvm_user_t g_nvm_user = { 0 };

factory_data_t g_factory_user = {0};

static int setting_task_id = -1;


int nvm_check_prj_id_valid ( unsigned int id )
{
	int panel_max_cnt = get_panel_max_count();

	if ( id >= panel_max_cnt || id == 0xFFFFFFFF ) {
		printf ( "project id %d is unvalid.\n", id );
		return -1;
	}

	return 0;
}

unsigned nvm_read_project_id(void)
{
	unsigned tmp_def_id = 0;
	unsigned int tmp_crc = 0;
	cri_data_t* pCriData = &gCriData;

	if (DYNAMIC_SET_PANEL_TYPE == 1)
		tmp_def_id = get_panel_type();
	else {
		if ( nvm_check_prj_id_valid ( pCriData->prj_id.prj_id_ori ) == 0 ) {
			tmp_crc = crc32 ( 0, ( unsigned char * ) &(pCriData->prj_id.prj_id_ori), sizeof ( unsigned int ) );

			if ( tmp_crc == pCriData->prj_id.prj_id_chksum ) {
				printf ( "cri data project id (%d) check sum ok.\n", pCriData->prj_id.prj_id_ori );
				return pCriData->prj_id.prj_id_ori;

			} else {
				printf ( "cri data project id checksum error" );
				printf ( "(%d, 0x%08X, 0x%08X)!\n", pCriData->prj_id.prj_id_ori, tmp_crc, pCriData->prj_id.prj_id_chksum );
			}
		}

		tmp_def_id = get_panel_def_id();
		printf ( "use default panel id (%d).\n", tmp_def_id );
	}

	return tmp_def_id;
}

int nvm_write_project_id ( unsigned val )
{
	unsigned int tmp_offset = 0;
	struct cri_prj_id tmp_id;

	if ( nvm_check_prj_id_valid ( val ) == 0 ) {
		memset ( ( void * ) &tmp_id, 0, sizeof ( struct cri_prj_id ) );
		tmp_id.prj_id_ori = val;
		tmp_id.prj_id_chksum = crc32 ( 0, ( unsigned char * ) &val, sizeof ( unsigned int ) );
		tmp_offset = 0;

		if ( spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_CRI_DATA_START + tmp_offset, sizeof ( struct cri_prj_id ), &tmp_id ) > 0 ) {
			printf ( "project id %d save is OK!\n", val );
			gCriData.prj_id = tmp_id;
			return 0;
		}
	}

	printf ( "project id %d save is error!\n", val );
	return -1;
}

char *nvm_read_device_id(void)
{
	unsigned int tmp_crc = 0;
	cri_data_t *pCriData = &gCriData;

	tmp_crc = crc32 ( 0, ( unsigned char * ) &gCriData.dev_id.dev_id_len, sizeof ( unsigned int ) );

	if ( tmp_crc == pCriData->dev_id.dev_id_l_chksum ) {
		printf ( "cri data device id length checksum ok.\n" );

		if ( pCriData->dev_id.dev_id_len < CC_CRI_DEVICE_ID_LEN ) {
			printf ( "cri data device id length ok.\n" );
			tmp_crc = crc32 ( 0, pCriData->dev_id.dev_id_buf, pCriData->dev_id.dev_id_len );

			if ( tmp_crc == pCriData->dev_id.dev_id_b_chksum ) {
				printf ( "cri data device id buffer " );
				printf ( "check sum ok.\n" );
				return ( char * ) pCriData->dev_id.dev_id_buf;

			} else {
				printf ( "cri data device id " );
				printf ( "buffer content chksum error" );
				printf ( "(0x%08X, 0x%08X)!\n", tmp_crc, pCriData->dev_id.dev_id_b_chksum );
			}

		} else {
			printf ( "cri data device id length error(%d, %d)!\n", pCriData->dev_id.dev_id_len, CC_CRI_DEVICE_ID_LEN );
		}

	} else {
		printf ( "cri data device id length checksum error" );
		printf ( "(%d, 0x%08X, 0x%08X)!\n", pCriData->dev_id.dev_id_len, tmp_crc, pCriData->dev_id.dev_id_l_chksum );
	}

	printf ( "device id read from old system method.\n" );

	return g_nvm_user.system.device_id;
}

int nvm_write_device_id ( char *device )
{
	unsigned int tmp_len = 0, tmp_offset = 0, tmp_crc = 0;
	struct cri_dev_id tmp_id;
	tmp_len = strlen ( device );

	if ( tmp_len < CC_CRI_DEVICE_ID_LEN ) {
		memset ( ( void * ) &tmp_id, 0, sizeof ( struct cri_dev_id ) );
		tmp_id.dev_id_len = tmp_len;
		tmp_id.dev_id_l_chksum = crc32 ( 0, ( unsigned char * ) &tmp_id.dev_id_len, sizeof ( unsigned int ) );
		strcpy ( tmp_id.dev_id_buf, device );
		tmp_id.dev_id_b_chksum = crc32 ( 0, tmp_id.dev_id_buf, tmp_id.dev_id_len );
		tmp_offset = sizeof ( struct cri_prj_id ) + sizeof ( struct cri_rev );

		if ( spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_CRI_DATA_START + tmp_offset, sizeof ( struct cri_dev_id ), &tmp_id ) > 0 ) {
			printf ( "cri data device id \"%s\" save is OK!\n", tmp_id.dev_id_buf );
			gCriData.dev_id = tmp_id;
		}

		//strcpy ( cur_system.device_id, device );
		strcpy ( g_nvm_user.system.device_id, device );
		//memcpy ( ( void * ) &g_nvm_user.system, ( void * ) &cur_system, sizeof ( struct systems ) );
		if ( spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_SETTING_START, sizeof ( nvm_user_t ), &g_nvm_user ) > 0 ) {
			printf ( "system device id \"%s\" save is OK!\n", g_nvm_user.system.device_id );
		}

		return 0;
	}

	printf ( "device id \"%s\" length error(%d, %d)!\n", tmp_id.dev_id_buf, tmp_len, CC_CRI_DEVICE_ID_LEN );
	return -1;
}

char *nvm_read_factory_sn(void)
{
	unsigned int tmp_crc = 0;
	tmp_crc = crc32 ( 0, ( unsigned char * ) &gCriData.fac_sn.fac_sn_len, sizeof ( unsigned int ) );

	if ( tmp_crc == gCriData.fac_sn.fac_sn_l_chksum ) {
		printf ( "cri data factory sn length checksum ok.\n" );

		if ( gCriData.fac_sn.fac_sn_len < CC_CRI_FAC_SN_LEN ) {
			printf ( "cri data factory sn length ok.\n" );
			tmp_crc = crc32 ( 0, gCriData.fac_sn.fac_sn_buf, gCriData.fac_sn.fac_sn_len );

			if ( tmp_crc == gCriData.fac_sn.fac_sn_b_chksum ) {
				printf ( "cri data factory sn buffer " );
				printf ( "check sum ok.\n" );
				return ( char * ) gCriData.fac_sn.fac_sn_buf;

			} else {
				printf ( "cri data factory sn buffer c" );
				printf ( "ontent chksum error" );
				printf ( "(0x%08X, 0x%08X)!\n", tmp_crc, gCriData.fac_sn.fac_sn_b_chksum );
			}

		} else {
			printf ( "cri data factory sn length error(%d, %d)!\n", gCriData.fac_sn.fac_sn_len, CC_CRI_FAC_SN_LEN );
		}

	} else {
		printf ( "cri data factory sn length checksum error" );
		printf ( "(%d, 0x%08X, 0x%08X)!\n", gCriData.fac_sn.fac_sn_len, tmp_crc, gCriData.fac_sn.fac_sn_l_chksum );
	}

	printf ( "factory sn read from old system method.\n" );
	return g_nvm_user.system.factory_sn;
}

int nvm_write_factory_sn ( char *pSn )
{
	unsigned int tmp_len = 0, tmp_offset = 0, tmp_crc = 0;
	struct cri_fac_sn tmp_sn;
	tmp_len = strlen ( pSn );

	if ( tmp_len < CC_CRI_FAC_SN_LEN ) {
		memset ( ( void * ) &tmp_sn, 0, sizeof ( struct cri_fac_sn ) );
		tmp_sn.fac_sn_len = tmp_len;
		tmp_sn.fac_sn_l_chksum = crc32 ( 0, ( unsigned char * ) &tmp_sn.fac_sn_len, sizeof ( unsigned int ) );
		strcpy ( tmp_sn.fac_sn_buf, pSn );
		tmp_sn.fac_sn_b_chksum = crc32 ( 0, tmp_sn.fac_sn_buf, tmp_sn.fac_sn_len );
		tmp_offset = sizeof ( struct cri_prj_id ) + sizeof ( struct cri_rev ) + sizeof ( struct cri_dev_id );

		if ( spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_CRI_DATA_START + tmp_offset, sizeof ( struct cri_fac_sn ), &tmp_sn ) > 0 ) {
			printf ( "cri data device id \"%s\" save is OK!\n", tmp_sn.fac_sn_buf );
			gCriData.fac_sn = tmp_sn;
		}

		if ( tmp_len < CC_FACTORY_SN_SIZE ) {
			strcpy ( g_nvm_user.system.factory_sn, pSn );
			//memcpy ( ( void * ) &g_nvm_user.system, ( void * ) &cur_system, sizeof ( struct systems ) );
			if ( spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_SETTING_START, sizeof ( nvm_user_t ), &g_nvm_user ) > 0 ) {
				printf ( "system factory sn \"%s\" save is OK!\n", g_nvm_user.system.factory_sn );
			}
		}

		return 0;
	}

	printf ( "factory sn \"%s\" length error(%d, %d)!\n", tmp_sn.fac_sn_buf, tmp_len, CC_CRI_FAC_SN_LEN );
	return -1;
}

char *nvm_read_hdcpkey()
{
	char *key = calloc ( USER_HDCPKEY_SIZE, 1 );

	if ( key <= 0 ) {
		return NULL;
	}

	if ( spi_flash_read ( get_spi_flash_device ( 0 ), USER_HDCPKEY_START, USER_HDCPKEY_SIZE, ( void * ) key ) <= 0 ) {
		printf ( "nvm_read_hdcpkey: spi read failed!\n" );
		free ( key );
		return NULL;
	}

	for ( int i = 0; i < USER_HDCPKEY_SIZE; i++ ) {
		key[i] = key[i] ^ HDCP_KEY_PARA;
	}

	return key;
}

int nvm_write_hdcpkey ( const char *key, int len )
{
	char *p = ( char * ) calloc ( USER_HDCPKEY_SIZE, 1 );
	int length = len > USER_HDCPKEY_SIZE ? USER_HDCPKEY_SIZE : len;

	for ( int i = 0; i < length; i++ ) {
		p[i] = key[i] ^ HDCP_KEY_PARA;
	}

#ifdef CONFIG_RANDOM_WRITE
	int ret = spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_HDCPKEY_START, USER_HDCPKEY_SIZE, ( void * ) p );
	free ( p );
	return ret;
#else
	free ( p );
	return 0;
#endif
}

/*
int nvm_write_wb_setting ( vpu_wb_e mode, vpu_wb_t *val )
{
	nvm_write_wb_mode(mode,1);
	nvm_write_wb_param(mode,val,1);
	return 0;
}

vpu_wb_t *nvm_read_wb_setting ( vpu_wb_e mode,vpu_wb_t *val )
{
	mode = nvm_read_wb_mode();
	nvm_read_wb_param ( mode, val);
	return val;
}
*/

int nvm_read_eye_protect(void)
{
	return g_nvm_user.system.vpp.eye_protect_enable;
}

int nvm_write_eye_protect(int cur_eye_flag,int save)
{
	g_nvm_user.system.vpp.eye_protect_enable = cur_eye_flag;
	if (save) {
		nvm_user_t *pspiaddr = (nvm_user_t *)USER_SETTING_START;
		unsigned int dest = (unsigned int )(&pspiaddr->system.vpp.eye_protect_enable);
		void *src = (void *)&g_nvm_user.system.vpp.eye_protect_enable;
		spi_flash_random_write ( get_spi_flash_device ( 0 ),dest, sizeof ( unsigned char ), src );
	} else {
		printf("nvm_write_eye_protect is not saved\n");
	}
	return 0;
}


int nvm_read_backlight_level(void)
{
	return g_nvm_user.system.misc.panel_backlight_level;
}

void nvm_write_backlight_level(int level,int save)
{
	if ( level != g_nvm_user.system.misc.panel_backlight_level) {
		g_nvm_user.system.misc.panel_backlight_level = level;
		if (save) {
			nvm_user_t *pst_userset = (nvm_user_t *)USER_SETTING_START;

			spi_flash_random_write ( get_spi_flash_device ( 0 ), (unsigned int)&(pst_userset->system.misc.panel_backlight_level),
							sizeof ( int ),&(g_nvm_user.system.misc.panel_backlight_level) );
		}
	}
}

void nvm_read_pq_db_data ( int idx ,int db_flag)
{
	partition_info_t* info = get_partition_info(SECTION_0, PARTITION_MAIN);
	int curIdx = idx;

	if (idx >= K_MAX_DB_SIZE) {
		curIdx = 0;
		printf("err: db idx %d\n",idx);
	}

	if (db_flag) {
		vpu_db_config_t *src = (vpu_db_config_t *)(PQ_BINARY_START + curIdx * PQ_BINARY_UNIT_SIZE);
		unsigned int src_part1_start = (PQ_BINARY_START + curIdx * PQ_BINARY_UNIT_SIZE);

		//gamma index 0
		//unsigned int src_part2_start = ( unsigned int )&(src->gamma[0]);
		unsigned int src_part3_start = ( unsigned int )&(src->cm2[0]);
		int size;

		printf("load pq db ... \n");
		//load part 1
		size = ( int ) (&vpu_config_table.gamma) - ( int ) ( &vpu_config_table );
		spi_flash_read(get_spi_flash_device(0), src_part1_start, size , (void*)&vpu_config_table);

		//load part 3
		size = (sizeof(vpu_config_t) - (( int ) (&vpu_config_table.cm2) - ( int ) ( &vpu_config_table ) ));
		if ( size < 0) {
			printf(" err: spi data load err\n");
		}
		spi_flash_read(get_spi_flash_device(0), src_part3_start, size, (void*)&vpu_config_table.cm2[0]);

	}else {

		vpu_db_config_t *src = (vpu_db_config_t *)default_db;
		int size;

		printf("load default pq db >> \n");
		//load part 1
		size = ( int ) (&vpu_config_table.gamma) - ( int ) ( &vpu_config_table );
		memcpy((void*)&vpu_config_table,src,size);

		//load part 3
		size = (sizeof(vpu_config_t) - (( int ) (&vpu_config_table.cm2) - ( int ) ( &vpu_config_table ) ));
		if ( size < 0) {
			printf(" err: spi data load err\n");
		}
		memcpy((void*)&vpu_config_table.cm2[0],&(src->cm2[0]),size);
	}

	printf("PQ DB version %d\n",vpu_config_table.version);

#if 0
	// check db data
	printf("\n");
	int i = 0;
	int length = sizeof(vpu_config_t);
	unsigned char *des = ( unsigned char * ) &vpu_config_table;
	unsigned char *des2;

	des2 = des;

	while ( i < length ) {
		printf("0x%x ",des[i]);
		i++;
		des2++;
		if ( (i % 16) == 0) {
			printf("\n");
		}

		if (((int)des2 == (int)(&vpu_config_table.gamma) ) || ((int)des2 == (int)(&vpu_config_table.cm2))) {
			printf(" @@ ");
		}
	}

	printf(" \n --- end ---\n");
#endif
}

void nvm_read_pq_db_byid ( unsigned int id, unsigned char *buf )
{
	int rd_first_end_id = 0, rd_real_id = 0;
	unsigned int rd_pq_bin_start = 0;
	rd_first_end_id = ( FBC_USER_START - PQ_BINARY_START ) / PQ_BINARY_UNIT_SIZE;

	if ( id <= rd_first_end_id ) {
		rd_pq_bin_start = PQ_BINARY_START;
		rd_real_id = id;

	} else {
		rd_pq_bin_start = FBC_USER_START + FBC_USER_SIZE;
		rd_real_id = id - rd_first_end_id - 1;
	}

	printf ( "id = %d, ", id );
	printf ( "rd_real_id = %d, ", rd_real_id );
	printf ( "rd_first_end_id = %d, ", rd_first_end_id );
	printf ( "rd_pq_bin_start = 0x%08X\n", rd_pq_bin_start );
	spi_flash_read ( get_spi_flash_device ( 0 ), rd_pq_bin_start + rd_real_id * PQ_BINARY_UNIT_SIZE, PQ_BINARY_UNIT_SIZE, buf );
}

int nvm_switch_pq_db_byid ( unsigned int id )
{
	int i = 0;
	unsigned int cal_pq_crc = 0, bin_pq_crc = 0;
	unsigned char *buf = ( unsigned char * ) malloc ( PQ_BINARY_UNIT_SIZE );

	if ( buf != NULL ) {
		if ( nvm_check_prj_id_valid ( id ) == 0 ) {
			nvm_read_pq_db_byid ( id, buf );
			cal_pq_crc = crc32 ( 0, buf, PQ_BINARY_UNIT_SIZE - 4 );
			bin_pq_crc = * ( ( unsigned int * ) &buf[PQ_BINARY_UNIT_SIZE - 4] );

			if ( cal_pq_crc != bin_pq_crc ) {
				printf ( "pq binary checksum error" );
				printf ( "(%d, 0x%08X, 0x%08X), load pq failed!\n", id, cal_pq_crc, bin_pq_crc );
				free ( buf );
				buf = NULL;
				return -1;
			}

			partition_info_t* info = get_partition_info(SECTION_0, PARTITION_PQ);
			spi_flash_erase(get_spi_flash_device(0),
					info->data_offset,
					info->data_size);

			spi_flash_write(get_spi_flash_device(0),
					info->data_offset,
					PQ_BINARY_UNIT_SIZE, buf);

			free ( buf );
			buf = NULL;
			printf ( "id(%d) load pq done!\n", id );
			return 0;
		}
	}

	printf ( "malloc memory error!\n" );
	return -1;
}

int nvm_check_pq_db_byid ( unsigned int id )
{
	unsigned int def_pq_crc = 0, bin_pq_crc = 0, bin_pq_ori_crc = 0;
	unsigned char *buf = ( unsigned char * ) malloc ( PQ_BINARY_UNIT_SIZE );

	if ( buf != NULL ) {
		if ( nvm_check_prj_id_valid ( id ) == 0 ) {
			memset ( buf, 0, PQ_BINARY_UNIT_SIZE );

			partition_info_t* info = get_partition_info(SECTION_0, PARTITION_PQ);
			spi_flash_read(get_spi_flash_device(0),
				       info->data_offset,
				       PQ_BINARY_UNIT_SIZE, buf);

			def_pq_crc = crc32 ( 0, buf, PQ_BINARY_UNIT_SIZE - 4 );
			memset ( buf, 0, PQ_BINARY_UNIT_SIZE );
			nvm_read_pq_db_byid ( id, buf );
			bin_pq_crc = crc32 ( 0, buf, PQ_BINARY_UNIT_SIZE - 4 );
			bin_pq_ori_crc = * ( ( unsigned int * ) &buf[PQ_BINARY_UNIT_SIZE - 4] );
			free ( buf );
			buf = NULL;
			printf ( "id(%d) \n", id );
			printf ( "def pq crc is 0x%08x \n", def_pq_crc );
			printf ( "bin pq crc is 0x%08x \n", bin_pq_crc );
			printf ( "bin pq ori crc is 0x%08x\n ", bin_pq_ori_crc );

			if ( bin_pq_crc != bin_pq_ori_crc ) {
				printf ( "pq binary checksum error\n" );
				printf ( "(%d, 0x%08X, 0x%08X)!\n", id, bin_pq_crc, bin_pq_ori_crc );
				return -1;
			}

			if ( def_pq_crc != bin_pq_crc ) {
				printf ( "id(%d) pq crc error!!!, lets load it's pq!\n", id );
				return nvm_switch_pq_db_byid ( id );

			} else {
				printf ( "id(%d) pq crc OK.\n", id );
			}

			return 0;
		}

		return -1;
	}

	printf ( "malloc memory error!\n" );
	return -1;
}

int nvm_switch_project_id ( unsigned int id )
{
	if ( nvm_check_prj_id_valid ( id ) == 0 ) {
		if ( nvm_write_project_id ( id ) == 0 ) {
			return nvm_switch_pq_db_byid ( id );
		}
	}

	return -1;
}

int nvm_read_light_enable ( void )
{
	return g_nvm_user.system.vpp.nature_light_enable;
}

void nvm_write_nature_lihgt_en ( unsigned int on_off )
{
	g_nvm_user.system.vpp.nature_light_enable = on_off;
	spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_SETTING_START, sizeof ( nvm_user_t ), &g_nvm_user );
}

int nvm_read_cursource(void)
{
	if (g_nvm_user.system.misc.cur_source < ePQSrc_MAX) {
		return g_nvm_user.system.misc.cur_source ;
	} else {
		printf("err: cur source %d \n",g_nvm_user.system.misc.cur_source);
		g_nvm_user.system.misc.cur_source = ePQSrc_HDMI;
		return ePQSrc_HDMI;
	}
}

int nvm_read_pic_mode(void)
{
	if (g_nvm_user.system.misc.cur_source < ePQSrc_MAX) {
		return g_nvm_user.pquser.pic_mode[g_nvm_user.system.misc.cur_source];
	} else {
		printf("err:nvm_read_pic_mode\n");
		g_nvm_user.pquser.pic_mode[g_nvm_user.system.misc.cur_source] = PICMOD_STD;
		return g_nvm_user.pquser.pic_mode[g_nvm_user.system.misc.cur_source];
	}
}

int nvm_write_pic_mode(int cur_pic_mode,int save)
{
	if ((g_nvm_user.system.misc.cur_source < ePQSrc_MAX) && (cur_pic_mode < PICMOD_MAX) ) {
		g_nvm_user.pquser.pic_mode[g_nvm_user.system.misc.cur_source] = cur_pic_mode;
		if (save) {
			nvm_user_t *pspiaddr = (nvm_user_t *)USER_SETTING_START;
			unsigned int dest = (unsigned int )(&pspiaddr->pquser.pic_mode[g_nvm_user.system.misc.cur_source]);
			void *src = (void *)&g_nvm_user.pquser.pic_mode[g_nvm_user.system.misc.cur_source];
			spi_flash_random_write ( get_spi_flash_device ( 0 ),dest, sizeof ( vpu_picmod_e ), src );
		}
	} else {
		printf("err:nvm_write_pic_mode\n");
		return -1;
	}
	return 0;
}

int nvm_read_pic_param(vpu_picmod_e cur_pic_mode,vpu_picmod_table_t* tab)
{
	if ((g_nvm_user.system.misc.cur_source < ePQSrc_MAX) && ( cur_pic_mode < PICMOD_MAX )) {
		memcpy ( ( void * )tab, ( void * ) &(g_nvm_user.pquser.picmode[g_nvm_user.system.misc.cur_source][cur_pic_mode]), sizeof ( vpu_picmod_table_t) );
	} else {
		printf("err: read pic mode param\n");
		memcpy ( ( void * )tab, ( void * ) &(g_nvm_user.pquser.picmode[ePQSrc_HDMI][PICMOD_STD]), sizeof ( vpu_picmod_table_t) );
		return -1;
	}
	return 0;
}

int nvm_write_pic_param(vpu_picmod_e cur_pic_mode,vpu_picmod_table_t* tab,int save)
{
	if ((g_nvm_user.system.misc.cur_source < ePQSrc_MAX) && ( cur_pic_mode < PICMOD_MAX )) {
		memcpy (( void * ) &(g_nvm_user.pquser.picmode[g_nvm_user.system.misc.cur_source][cur_pic_mode]), ( void * )tab, sizeof ( vpu_picmod_table_t) );
		if (save) {
			nvm_user_t *pspiaddr = (nvm_user_t *)USER_SETTING_START;

			unsigned int dest = (unsigned int )(&pspiaddr->pquser.picmode[g_nvm_user.system.misc.cur_source][cur_pic_mode]);
			void *src = (void *)&g_nvm_user.pquser.picmode[g_nvm_user.system.misc.cur_source][cur_pic_mode];
			spi_flash_random_write ( get_spi_flash_device ( 0 ),dest, sizeof ( vpu_picmod_table_t ), src );
		}
	} else {
		printf("err: write pic mode param\n");
		return -1;
	}

	return 0;
}

int nvm_read_wb_mode(void)
{
	if (g_nvm_user.system.misc.cur_source < ePQSrc_MAX) {
		return g_nvm_user.pquser.wb_mode[g_nvm_user.system.misc.cur_source];
	} else {
		printf("err:nvm_read_wb_mode\n");
		g_nvm_user.pquser.wb_mode[g_nvm_user.system.misc.cur_source] = eWB_STD;
		return g_nvm_user.pquser.wb_mode[g_nvm_user.system.misc.cur_source];
	}
}

int nvm_write_wb_mode(int cur_wb_mode,int save)
{
	if ((g_nvm_user.system.misc.cur_source < ePQSrc_MAX) && (cur_wb_mode < PICMOD_MAX) ) {
		g_nvm_user.pquser.wb_mode[g_nvm_user.system.misc.cur_source] = cur_wb_mode;
		if (save) {
			nvm_user_t *pspiaddr = (nvm_user_t *)USER_SETTING_START;

			unsigned int dest = (unsigned int )(&pspiaddr->pquser.wb_mode[g_nvm_user.system.misc.cur_source]);
			void *src = (void *)&g_nvm_user.pquser.wb_mode[g_nvm_user.system.misc.cur_source];
			spi_flash_random_write ( get_spi_flash_device ( 0 ),dest, sizeof ( vpu_wb_e ), src );
		}
	} else {
		printf("err:nvm_write_wb_mode\n");
		return -1;
	}
	return 0;
}

int nvm_read_wb_param(int cur_wb_mode,vpu_wb_t *ptabWb)
{
	if ( cur_wb_mode < eWB_MAX) {
		*ptabWb = g_factory_user.pqfac.wb[cur_wb_mode];
	} else {
		printf("err:nvm_read_wb_param\n");
		*ptabWb = g_factory_user.pqfac.wb[eWB_STD];
		return -1;
	}
	return 0;
}

int nvm_write_wb_param(int cur_wb_mode,vpu_wb_t *ptabWb,int save)
{
	if ( cur_wb_mode < eWB_MAX) {
		g_factory_user.pqfac.wb[cur_wb_mode] = *ptabWb;
		if (save) {
			factory_data_t *pspiaddr = (factory_data_t *)FBC_FACTORY_PART2_START;
			unsigned int dest = (unsigned int )(&pspiaddr->pqfac.wb[cur_wb_mode]);
			void *src = (void *)&g_factory_user.pqfac.wb[cur_wb_mode];
			spi_flash_random_write ( get_spi_flash_device ( 0 ),dest, sizeof ( vpu_wb_t ), src );

			//only is different ,save it
			if (g_factory_user.factory_data_flag != 2 ) {
				g_factory_user.factory_data_flag = 2;
				dest = (unsigned int )(&pspiaddr->factory_data_flag);
				void *src = (void *)&g_factory_user.factory_data_flag;
				spi_flash_random_write ( get_spi_flash_device( 0 ),dest,sizeof (unsigned int ),src);
			}
		}
	} else {
		printf("err:nvm_write_wb_param\n");
		return -1;
	}

	return 0;
}

int nvm_read_pattern_mode(void)
{
	return g_nvm_user.system.misc.pattern_mode;
}

void nvm_write_pattern_mode(int ptm_md,int save)
{
	g_nvm_user.system.misc.pattern_mode = ptm_md;
}


void nvm_read_gammatab(int idx,int dbIdx)
{
	vpu_db_config_t *src = (vpu_db_config_t *)(PQ_BINARY_START + dbIdx * PQ_BINARY_UNIT_SIZE);

	if ( idx >= K_MAX_GAMMA_SIZE || dbIdx >= K_MAX_DB_SIZE) {
		printf("err:gamma idx %d , db idx %d \n",idx,dbIdx);
		return;
	}

	if (gbDBCRC_OK) {
		//load part 2 gamma (load only one)
		spi_flash_read(get_spi_flash_device(0), (unsigned int)(&src->gamma[idx]), sizeof(vpu_gamma_t), (void*)&vpu_config_table.gamma[0]);
	} else {
		vpu_db_config_t *src = (vpu_db_config_t *)default_db;
		memcpy( (void*)&vpu_config_table.gamma[0],&src->gamma[idx],sizeof(vpu_gamma_t));
	}
}

int nvm_read_gamma_idx(void)
{
	if (g_nvm_user.system.misc.gammaIdx < K_MAX_GAMMA_SIZE) {
		return g_nvm_user.system.misc.gammaIdx;
	} else {
		printf("err get: gamma_idx %d\n",g_nvm_user.system.misc.gammaIdx);
		g_nvm_user.system.misc.gammaIdx = 0;
		return 0;
	}
}

int nvm_write_gamma_idx(int idx,int save)
{
	if ( idx != g_nvm_user.system.misc.gammaIdx) {
		g_nvm_user.system.misc.gammaIdx = idx;
		if (save) {
			nvm_user_t *pst_userset = (nvm_user_t *)USER_SETTING_START;

			spi_flash_random_write ( get_spi_flash_device ( 0 ), (unsigned int)&(pst_userset->system.misc.gammaIdx),
							sizeof (unsigned int ),&(g_nvm_user.system.misc.gammaIdx) );
		}
	} else {
		printf("err write : gammaIdx=%d \n",idx);
		return -1;
	}
	return 0;
}

int nvm_read_pqDB_idx(void)
{
	if (g_nvm_user.system.misc.pqDBIdx < K_MAX_DB_SIZE) {
		return g_nvm_user.system.misc.pqDBIdx;
	} else {
		printf("err:read pqDBIdx %d\n",g_nvm_user.system.misc.pqDBIdx);
		g_nvm_user.system.misc.pqDBIdx = 0;
		return 0;
	}
}

int nvm_write_pqDB_idx(int idx,int save)
{
	if ( idx != g_nvm_user.system.misc.pqDBIdx) {
		g_nvm_user.system.misc.pqDBIdx = idx;
		if (save) {
			nvm_user_t *pst_userset = (nvm_user_t *)USER_SETTING_START;

			spi_flash_random_write ( get_spi_flash_device ( 0 ), (unsigned int)&(pst_userset->system.misc.pqDBIdx),
							sizeof (unsigned int ),&(g_nvm_user.system.misc.pqDBIdx) );
		}
	} else {
		printf("err: pqDB_idx=%d \n",idx);
		return -1;
	}
	return 0;
}

int nvm_read_monitor_mode(void)
{
	//return 0:off 1:on
	return g_nvm_user.system.misc.monitor_mode;
}

int nvm_write_monitor_mode(int OnOff,int save)
{
	if ( OnOff != g_nvm_user.system.misc.monitor_mode) {
		g_nvm_user.system.misc.monitor_mode = OnOff;
		if (save) {
			nvm_user_t *pst_userset = (nvm_user_t *)USER_SETTING_START;

			spi_flash_random_write ( get_spi_flash_device ( 0 ), (unsigned int)&(pst_userset->system.misc.monitor_mode),
							sizeof (unsigned int ),&(g_nvm_user.system.misc.monitor_mode) );
		}
	}
	return 0;
}

#ifndef CONFIG_CUSTOMER_PROTOCOL
int check_cmd_is_supported ( int cmd )
{
	switch ( cmd ) {
		case ( VPU_CMD_ENABLE | VPU_CMD_READ ) :
		case VPU_CMD_WB_VALUE:
		case ( VPU_CMD_WB_VALUE | VPU_CMD_READ ) :
		case CMD_LVDS_SSG_SET:
			return 1;

		default:
			return 0;
	}
}

unsigned int handle_setting_cmd ( unsigned char *s, int *rets )
{
	vpu_wb_t wbs;
	int *params = GetParams ( s );

	if ( params == NULL ) {
		return -1;
	}

	switch ( CmdID ( s ) ) {
		case VPU_CMD_WB_VALUE:
			int mode = params[0];
			nvm_read_wb_param( mode, &wbs);
			//wbs = ( vpu_wb_t * ) malloc ( sizeof ( vpu_wb_t ) );
			wbs.gain_r = params[1];
			wbs.gain_g = params[2];
			wbs.gain_b = params[3];
			wbs.pre_offset_r = params[4];
			wbs.pre_offset_g = params[5];
			wbs.pre_offset_b = params[6];
			//nvm_write_wb_setting ( mode, &wbs );
			nvm_write_wb_param( mode, &wbs, 1);
			//free ( wbs );
			break;

		case ( VPU_CMD_WB_VALUE | VPU_CMD_READ ) :
			//nvm_read_wb_setting ( params[0], &wbs );
			nvm_read_wb_param( params[0], &wbs);

			//if ( wbs == NULL ) {
			//	break;
			//}

			rets[0] = params[0];
			rets[1] = wbs.gain_r;
			rets[2] = wbs.gain_g;
			rets[3] = wbs.gain_b;
			rets[4] = wbs.pre_offset_r;
			rets[5] = wbs.pre_offset_g;
			rets[6] = wbs.pre_offset_b;
			//free ( wbs );
			break;

		case ( VPU_CMD_ENABLE | VPU_CMD_READ ) :
			vpp_cmd_read_user_setting ( s, rets );
			break;

		case CMD_LVDS_SSG_SET:
			set_lcd_clk_ss ( params[0] );
			break;

		default:
			break;
	}

	free ( params );
	return 0;
}

int setting_task_handle ( int task_id, void *param )
{
	list_t *plist = list_dequeue ( &setting_list );

	if ( plist != NULL ) {
		CMD_LIST *clist = list_entry ( plist, CMD_LIST, list );

		if ( clist != NULL ) {
			unsigned char *cmd = ( unsigned char * ) ( clist->cmd_data.data );

			if ( cmd != NULL ) {
				int rcmd_len = Ret_NumParam ( cmd );

				if ( rcmd_len > 0 ) {
					int *params = ( int * ) malloc ( rcmd_len * sizeof ( int ) );
					handle_setting_cmd ( cmd, params );
					SendReturn ( setting_task_id, clist->cmd_data.cmd_owner, *cmd, ( int * ) params );
					free ( params );
					params = NULL;

				} else {
					handle_setting_cmd ( cmd, NULL );
				}
			}

			freeCmdList ( clist );
		}
	}

	return 0;
}
#endif

int load_default_user_setting ( int clr_version_flag )
{
	printf("reset user default setting \n");

	nvm_read_pq_db_data(system_default.misc.pqDBIdx,0);

	//reset nvm ...
	nvm_resetusersetting();

	//need after nvm initial
	nvm_read_pq_db_data(nvm_read_pqDB_idx(),0);
	printf("warning : need reboot >>>>>>>>>\n");
	return 0;
}

int clr_default_wb_setting ( void )
{
	load_default_user_setting(0);

	return 0;
}

int nvm_check_ver_not_active(void)
{
	if (g_nvm_user.system.ver1 != system_default.ver1) {
		return 1;
	} else {
		return 0;
	}
}

void nvm_readallusersetting(nvm_user_t *dist)
{
	//read user setting from user section
	spi_flash_read ( get_spi_flash_device ( 0 ), USER_SETTING_START, sizeof ( nvm_user_t ), ( void * )dist );

	memset ( ( void * ) &gCriData, 0, sizeof ( cri_data_t ) );
	spi_flash_read ( get_spi_flash_device ( 0 ), USER_CRI_DATA_START, sizeof ( cri_data_t ), ( void * ) &gCriData );
}

void nvm_readallfactorysetting(factory_data_t *dist)
{

	//read factory setting from factory section
	spi_flash_read ( get_spi_flash_device ( 0 ), FBC_FACTORY_PART2_START, sizeof ( factory_data_t ), ( void * )dist );

}


int nvm_reset_factory_setting(void)
{
	printf("reset factory setting\n");
	//g_factory_user.db_ver_factory = factory_default.db_ver_factory;
	g_factory_user.factory_data_flag = 1;//setp 1

	//load wb from pq db
	memcpy ( ( void * ) &(g_factory_user.pqfac.wb[eWB_COLD].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_COLD].gain_r), sizeof ( vpu_wb_t) );
	memcpy ( ( void * ) &(g_factory_user.pqfac.wb[eWB_STD].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_STD].gain_r), sizeof ( vpu_wb_t) );
	memcpy ( ( void * ) &(g_factory_user.pqfac.wb[eWB_WARM].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_WARM].gain_r), sizeof ( vpu_wb_t) );
	memcpy ( ( void * ) &(g_factory_user.pqfac.wb[eWB_USER].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_STD].gain_r), sizeof ( vpu_wb_t) );

	return spi_flash_random_write ( get_spi_flash_device ( 0 ), FBC_FACTORY_PART2_START, sizeof ( factory_data_t ), &g_factory_user);
}

int nvm_resetusersetting (void)
{
	printf("reset all user setting >>> \n");

	//load cri default data
	//memcpy ( ( void * ) &g_nvm_user.cri, ( void * ) &gCriData, sizeof ( cri_data_t ) );

	//load system default data
	memcpy ( ( void * ) &g_nvm_user.system, ( void * ) &system_default, sizeof ( systems_t ) );
	//load audio default data
	memcpy ( ( void * ) &g_nvm_user.audio, ( void * ) &audio_state_def, sizeof ( audio_control_t ) );
	//load pq user default data
	memcpy ( ( void * ) &g_nvm_user.pquser, ( void * ) &user_pq_default, sizeof ( user_pq_t ) );

	//load wb from pq db
	g_nvm_user.system.db_ver = vpu_config_table.version;
	//memcpy ( ( void * ) &(g_nvm_user.pqfac.wb[eWB_COLD].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_COLD].gain_r), sizeof ( vpu_wb_t) );
	//memcpy ( ( void * ) &(g_nvm_user.pqfac.wb[eWB_STD].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_STD].gain_r), sizeof ( vpu_wb_t) );
	//memcpy ( ( void * ) &(g_nvm_user.pqfac.wb[eWB_WARM].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_WARM].gain_r), sizeof ( vpu_wb_t) );
	//memcpy ( ( void * ) &(g_nvm_user.pqfac.wb[eWB_USER].gain_r), ( void * ) &(vpu_config_table.wb[eDB_WB_STD].gain_r), sizeof ( vpu_wb_t) );

	return spi_flash_random_write ( get_spi_flash_device ( 0 ), USER_SETTING_START, sizeof ( nvm_user_t ), &g_nvm_user );
}



void nvm_check_data(void)
{
	printf("@pattern %d\n",nvm_read_pattern_mode());
	printf("@source %d\n",nvm_read_cursource());
	printf("@wb mode %d\n",nvm_read_wb_mode());
	printf("@pic mode %d\n",nvm_read_pic_mode());

	//vpu_wb_t stwb;
	//nvm_read_wb_param(nvm_read_wb_mode(),&stwb);

	printf("@wb cold gain r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_COLD].gain_r,g_factory_user.pqfac.wb[eWB_COLD].gain_g,g_factory_user.pqfac.wb[eWB_COLD].gain_b);
	printf("@wb cold preoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_COLD].pre_offset_r,g_factory_user.pqfac.wb[eWB_COLD].pre_offset_g,g_factory_user.pqfac.wb[eWB_COLD].pre_offset_b);
	printf("@wb cold postoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_COLD].post_offset_r,g_factory_user.pqfac.wb[eWB_COLD].post_offset_g,g_factory_user.pqfac.wb[eWB_COLD].post_offset_b);

	printf("@wb std gain r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_STD].gain_r,g_factory_user.pqfac.wb[eWB_STD].gain_g,g_factory_user.pqfac.wb[eWB_STD].gain_b);
	printf("@wb std preoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_STD].pre_offset_r,g_factory_user.pqfac.wb[eWB_STD].pre_offset_g,g_factory_user.pqfac.wb[eWB_STD].pre_offset_b);
	printf("@wb std postoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_STD].post_offset_r,g_factory_user.pqfac.wb[eWB_STD].post_offset_g,g_factory_user.pqfac.wb[eWB_STD].post_offset_b);

	printf("@wb warm gain r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_WARM].gain_r,g_factory_user.pqfac.wb[eWB_WARM].gain_g,g_factory_user.pqfac.wb[eWB_WARM].gain_b);
	printf("@wb warm preoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_WARM].pre_offset_r,g_factory_user.pqfac.wb[eWB_WARM].pre_offset_g,g_factory_user.pqfac.wb[eWB_WARM].pre_offset_b);
	printf("@wb warm postoffset r:%d g:%d b%d\n",g_factory_user.pqfac.wb[eWB_WARM].post_offset_r,g_factory_user.pqfac.wb[eWB_WARM].post_offset_g,g_factory_user.pqfac.wb[eWB_WARM].post_offset_b);

	vpu_picmod_table_t stpicTab;
	nvm_read_pic_param(nvm_read_pic_mode(),&stpicTab);
	printf("@Pic %d: bri %d , con %d , sat %d \n",stpicTab.picmod, stpicTab.bright,stpicTab.contrast,stpicTab.saturation);

	printf("@read gamma idx %d\n",nvm_read_gamma_idx());
	printf("@read PQ DB idx %d\n",nvm_read_pqDB_idx());

	printf("@bri con idx %d\n",vpu_config_table.bri_con_index);
	int i = 0;
	for (i = 0; i < 11 ; i++) {
		printf("@bri_con idx %d val=0x%x\n",i,vpu_config_table.bri_con[i]);
	}
}


int nvm_check_db_crc(unsigned int curIdx)
{
	partition_info_t* info = get_partition_info(SECTION_0, PARTITION_PQ);
	//partition_info_t* info = get_partition_info(SECTION_0, PARTITION_MAIN);

	vpu_db_config_t *dbStart;
	unsigned int dblen;
	unsigned int curCRC,orgCRC;

	dbStart = (vpu_db_config_t *)(PQ_BINARY_START + curIdx * PQ_BINARY_UNIT_SIZE + 0x41000000);
	orgCRC = dbStart->crc;
	dblen = dbStart->db_len;
	printf("dbstruct size %d , crc len=%d \n",sizeof (vpu_db_config_t),dblen);

	curCRC = crc32(0,(unsigned char*)&(dbStart->version),dblen);
	printf("check db crc: cur= 0x%x ,org= 0x%x , length=0x%0x\n",curCRC,orgCRC,dblen);

	if ( (curCRC != orgCRC) || (dblen == 0) || (dblen == 0xffffffff)) {
		#if 0
		//the following is not debug
		printf("err:db crc error cur 0x%x, org 0x%x \n",curCRC,orgCRC);
		dbStart = (vpu_db_config_t *)(PQ_BINARY_START + curIdx * PQ_BINARY_UNIT_SIZE);

		//recovery default db to DB section
		printf("PQ section: data_offset=0x%x,data_size=0x%x\n",info->data_offset,info->data_size);
		spi_flash_erase(get_spi_flash_device(0),
								(info->data_offset),
								PQ_BINARY_UNIT_SIZE);

		spi_flash_write ( get_spi_flash_device ( 0 ),
								(info->data_offset),
								sizeof (vpu_db_config_t ),(const void*)default_db );
		#endif
		printf("err: PQ db crc err \n");
		return 0;
	} else {
		printf("db match !!\n");
		return 1;
	}
}


void nvm_init_task ( void )
{
	register_vpp_save ( vpp_cmd_user_setting );
	register_audio_save ( vpp_cmd_user_setting );

	unsigned int userCurSize;
	nvm_user_t *pnvm_spi_start = (nvm_user_t *)USER_SETTING_START;

	userCurSize = sizeof (nvm_user_t );
	printf("userCurSize %d (total 7k) \n",userCurSize);

	gbDBCRC_OK = nvm_check_db_crc(system_default.misc.pqDBIdx);

	//load nvm user setting data from spi
	nvm_readallusersetting(&g_nvm_user);

	//load factory setting data from spi
	nvm_readallfactorysetting(&g_factory_user);

	//need after nvm initial
	if (nvm_check_ver_not_active()) {
		printf("user setting version changed \n");
		printf("current version is 0x%08x, default version is 0x%08x\n", g_nvm_user.system.ver1, system_default.ver1 );

		//read default db data
		nvm_read_pq_db_data(system_default.misc.pqDBIdx, gbDBCRC_OK);

		//reset nvm ...
		nvm_resetusersetting();

		//reset factory data..
		//if ( ( vpu_config_table.version != g_nvm_user.system.db_ver) &&
		//	(g_factory_user.factory_data_flag <= 1))
		if ( g_factory_user.factory_data_flag == 0 ) {
			printf("PQ db changed pre=0x%x,cur=0x%x",g_nvm_user.system.db_ver,vpu_config_table.version);
			nvm_reset_factory_setting();
		}
	} else {
		printf("read db according db idx %d\n",nvm_read_pqDB_idx());
		//read db according to DB nvm index
		nvm_read_pq_db_data(nvm_read_pqDB_idx(),gbDBCRC_OK);

		//reset factory data..
		//if ( ( vpu_config_table.version != g_nvm_user.system.db_ver) &&
		//	(g_factory_user.factory_data_flag <= 1)) {
		if ( g_factory_user.factory_data_flag == 0 ) {
			printf("PQ db changed pre=0x%x,cur=0x%x",g_nvm_user.system.db_ver,vpu_config_table.version);
			nvm_reset_factory_setting();
		}
	}

	//load gamma table
	nvm_read_gammatab(nvm_read_gamma_idx(),nvm_read_pqDB_idx());

	//check read back data
	nvm_check_data();

	nvm_check_pq_db_byid ( nvm_read_pqDB_idx() );


	setting_task_id = RegisterTask ( setting_task_handle, NULL, 0, TASK_PRIORITY_USER );

	if ( setting_task_id > 0 ) {
		RegisterCmd ( &setting_list, setting_task_id, INPUT_CEC | INPUT_UART_HOST, check_cmd_is_supported, handle_setting_cmd );
	}

#if 0//ndef FBC_USER_WITHOUT_SAVE
	save_task_id = RegisterTask ( save_task_handle, NULL, 0, TASK_PRIORITY_USER );

	if ( save_task_id > 0 ) {
		request_timer ( save_task_id, 500 );    /* 5s */
	}

#endif
}
