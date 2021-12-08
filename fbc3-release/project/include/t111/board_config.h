/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__
#include <panel.h>

/*
PANEL_ID_1080P_NORMAL						0
PANEL_ID_4K_NORMAL_3B						1
PANEL_ID_4K_NORMAL_4B						2

PANEL_ID_4K_SAMSUNG_LSC550FN04				4
PANEL_ID_4K_INNOLUX_V400DJ1					5
*/
//#define PANEL_ID PANEL_ID_1080P_NORMAL
#define PANEL_ID PANEL_ID_4K_SAMSUNG_LSC550FN04

/*
1080: 0
4k:   1
*/
#define CONFIG_BIT_MODE 1

#define PROJECT_ID PANEL_ID
#define PANEL_MODULE "HV550QU2-305"

#define DYNAMIC_SET_PANEL_TYPE 0
#define BOOT_DEBUG_VERSION 1
#define UI_HAVE_LOGO 1
#define ENABLE_AVMUTE_CONTROL 1
#define ENABLE_AUTO_BACKLIGHT_CONTROL 0

#define K_NO_SIGNAL_DEBOUNCE_CNT	100
#define K_NO_SIGNAL_INFO			0

#define LAYOUT_VERSION 0x00010000
#define CONFIG_SPI_SIZE 0x200000

//pq bin section
#define PQ_BINARY_START 0x1A3000
#define PQ_BINARY_UNIT_SIZE 0x4000

//sn/mn section
#define SN_MN_SIZE 0x200

//factory section
#define FBC_FACTORY_START (CONFIG_SPI_SIZE - FBC_FACTORY_SIZE)
#define FBC_FACTORY_SIZE 0x10000

//factory wb_setting section
#define FBC_FACTORY_PART2_START (FBC_FACTORY_START + SN_MN_SIZE)
#define FBC_FACTORY_PART2_SIZE 0x100

//user section
#define FBC_USER_START (FBC_FACTORY_START - FBC_USER_SIZE)
#define FBC_USER_SIZE 0x2000

	//user part 1
	#define USER_HDCPKEY_START FBC_USER_START
	#define USER_HDCPKEY_SIZE 0x400
	//user part 2
	#define USER_CRI_DATA_START (USER_HDCPKEY_START+USER_HDCPKEY_SIZE)
	#define USER_CRI_DATA_SIZE 0x200

	//user part 3
	#define USER_SETTING_START (USER_CRI_DATA_START+USER_CRI_DATA_SIZE)
	#define USER_SETTING_SIZE 0x100
	//user part 4
	#define USER_WB_SETTING_START (USER_SETTING_START+USER_SETTING_SIZE)
	#define USER_WB_SETTING_SIZE 0x100
	//user part 5
	//#define USER_PIC_SETTING_START (USER_WB_SETTING_START+USER_WB_SETTING_SIZE)
	//#define USER_PIC_SETTING_SIZE 0x100

#define CONFIG_HDMITx_YUV420	0

/*
PB:T111
PA:T112
*/
#define CONFIG_SWITCH_P "PB"

extern  const char* switch_p;
extern  int bit10_mode;

extern void init_configures(void);

#endif//__BOARD_CONFIG_H__
