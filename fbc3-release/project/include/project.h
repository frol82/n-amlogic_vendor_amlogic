#ifndef _PROJECT_H_
#define _PROJECT_H_

//#include <board_config.h>
//#define PROJECT_ID 2    //define in Makefile
//#define FBC_USER_WITHOUT_SAVE

#define POWER_ON_MODE_SUSPEND	0
#define POWER_ON_MODE_MAIN		1
#define USER_POWER_ON_MODE  	POWER_ON_MODE_MAIN

//#ifdef CONFIG_CUSTOMER_PROTOCOL
//#define WB_DATA_FROM_DB	        0
//#else
#define WB_DATA_FROM_DB	        1
//#endif

#define LDIM_DATA_FROM_DB       1

#define FBC_PATTERN_MODE		PATTERN_MODE_BLACK

#define	LOCKN_TYPE_A			0
#define	LOCKN_TYPE_B			1//new method as T868's
#define	LOCKN_TYPE_C			2//new method as GXTVBB's
#define LOCKN_TYPE_SEL			LOCKN_TYPE_B

#define ENABLE_10BIT_MODE	1


#define HDMIRX_HPD_HIGH	1	//for other boards
#define HDMIRX_HPD_LOW		0	//for T111/112 socket board
#define HDMIRX_HPD_LVL		HDMIRX_HPD_HIGH

#endif
