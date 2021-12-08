#ifndef PANEL_H
#define PANEL_H
//#include <lcd_drv.h>

#define PANEL_ID_1080P_NORMAL						0
#define PANEL_ID_4K_NORMAL_3B						1
#define PANEL_ID_4K_NORMAL_4B						2

#define PANEL_ID_4K_SAMSUNG_LSC550FN04				4
#define PANEL_ID_4K_INNOLUX_V400DJ1					5


#define CC_PANEL_MAX_COUNT	(35)

enum panel_interface_e {
	PANEL_IF_LVDS = 0,
	PANEL_IF_VBYONE,
	PANEL_IF_MAX,
};

#if 0
typedef enum panel_format_e {
	PANEL_NORMAL = 0,
	PANEL_YUV420,
	PANEL_YUV422,
	PANEL_YUV444,
	PANEL_MAX,
} panel_format_t;
#endif

typedef enum vpu_outputmode_e {
	T_1080P50HZ = 0,
	T_2160P50HZ420,
	T_1080P50HZ44410BIT,
	T_2160P50HZ42010BIT,
	T_2160P50HZ42210BIT,
	T_2160P50HZ444,
} vpu_outputmode_t;

typedef enum vpu_timing_e {
	TIMING_NULL = 0,
	TIMING_1366x768P60,
	TIMING_1920x1080P50,
	TIMING_1920x1080P60,
	TIMING_1920x1080P100,
	TIMING_1920x1080P120,
	TIMING_1920x1080P60_3D_SG,
	TIMING_1920x1080P240,
	TIMING_1920x1080P120_3D_SG,
	TIMING_3840x2160P60,
	TIMING_3840x2160P50,
	TIMING_3840x2160P24,
	TIMING_3840x2160P30,
	TIMING_4kx1kP120_3D_SG,
	TIMING_4kxd5kP240_3D_SG,
	TIMING_MAX,
} vpu_timing_t;

enum bl_pwm_channel_e {
	PWM_PWM0 = 0,
	PWM_PWM1,
	PWM_PWM2,
	PWM_PWM3,
	PWM_BL_PWM0,
	PWM_BL_PWM1,
	PWM_BL_PWM2,
	PWM_BL_PWM3,
	PWM_BL_PWM4,
	PWM_BL_PWM5,
	PWM_BL_PWM6,
	PWM_BL_PWM7,
	PWM_MAX,
};


typedef struct panel_config_s {
	char *model_name;
	char panel_id;
	enum panel_interface_e interface;
	vpu_outputmode_t output_mode;
	vpu_timing_t timing;
	//panel_format_t format;

	unsigned char reverse;
	unsigned char scaler;

	unsigned char clk;
	unsigned char repack;		/* 0:normal, 1,2:repack */
	unsigned char ports;		/* 0: single port;  1: dual ports */
	unsigned char bit_size;		/* 0:10bits, 1:8bits, 2:6bits, 3:4bits */
	unsigned char odd_even;	/* 0:normal, 1:swap */
	unsigned char pn_swap;	/* positive and negative swap */
	unsigned char hv_invert;	/* invert hs and vs */
	unsigned char lvds_swap;
	unsigned char clk_pin_swap;

	int lane_num;
	int region_num;
	int byte_num;
	int color_fmt;
	/*vx1_lockn_option:
	-1: register lockn process in function start_vpp();
	n: register lockn process after n S;
	0:don't register lockn irq process*/
	short vx1_lockn_option;
	char vx1_counter_option;
	char vx1_hpd_wait;
	char hsync_pol;
	char vsync_pol;

	/*vx1 lvds combo ctl used by eye chart*/
	unsigned int vx1_lvds_phy_vswing;
	unsigned int vx1_lvds_phy_preem;
	/* clock spread spectrum */
	unsigned char clk_ss_level;

	unsigned char bl_ctrl_method;  /* 1=pwm, 2=ldim */
	unsigned short bl_level_default;
	unsigned char bl_pwm_port; /* select pwm channel */
	unsigned char bl_pwm_pol;   /*  1=positive, 0=negative */
	unsigned short bl_pwm_hz;
	unsigned short bl_pwm_duty_max; /* range 0-255*/
	unsigned short bl_pwm_duty_min; /* range 0-255*/
	/* local diming config */
	unsigned char bl_ldim_mode;  /* 1=single_side(top, bottom, left or right), 2=uniform(top/bottom, left/right) */
	unsigned char bl_ldim_region_row;
	unsigned char bl_ldim_region_col;
	/*unsigned short bl_ldim_mapping[384]; ldim LED location mapping */
	unsigned char bl_ldim_dev_index;

	unsigned short panel_power_on_delay;
	unsigned short panel_power_off_delay;
	unsigned short signal_enable_delay;
	unsigned short signal_disable_delay;
	unsigned short pwm_enable_delay;
	unsigned short pwm_disable_delay;
	unsigned short backlight_power_on_delay;
	unsigned short backlight_power_off_delay;

	unsigned int hdr_support;
	unsigned int hdr_lumi_max;
	unsigned int hdr_lumi_min;
	unsigned int hdr_lumi_avg;
	unsigned int hdr_primaries[3][2]; /* Rx,y, Gx,y, Bx,y */
	unsigned int hdr_white_point[2]; /* Wx,y */

	char ManufactureID[2];
	char ProductID[2];
	//machine serial number
	char SerialNumber[4];
	//week,year
	char ManufactureDate[2];
	//0xfb,0x0c(fbc) || 0xff,0xff(non-fbc)
	char ChipID[2];
	//=0 1080p
	//=1 4k2k
	//=2 1366x768
	char PanelInfo: 4;
	//=0 non-3D
	//=1 frame packing
	//=2 top bottom
	//=3 side by side full
	//=4 side byte side half (horz sub-sampling)
	//=5 Line alternative
	//=6 side byte side half (all quincunx sub-sampling)
	//=7 L+depth
	//=8 L+depth+graphics-depth
	//=9 Field alternative
	char ThreeDinfo: 4;
	//bit0=1 panel screen upside-down; bit0=0 normal
	char SpecicalInfo;

	void (*panel_power_ctrl)(char flag);
	void (*backlight_power_ctrl)(char flag);
} panel_config_t;

/*
typedef struct Usr_EDID_s{
	char ManufactureID[2];
	char ProductID[2];
	//machine serial number
	char SerialNumber[4];
	//week,year
	char ManufactureDate[2];
	//0xfb,0x0c(fbc) || 0xff,0xff(non-fbc)
	char ChipID[2];
	//=0 1080p
	//=1 4k2k
	//=2 1366x768
	char PanelInfo:4;
	//=0 non-3D
	//=1 frame packing
	//=2 top bottom
	//=3 side by side full
	//=4 side byte side half (horz sub-sampling)
	//=5 Line alternative
	//=6 side byte side half (all quincunx sub-sampling)
	//=7 L+depth
	//=8 L+depth+graphics-depth
	//=9 Field alternative
	char ThreeDinfo:4;
	//bit0=1 panel screen upside-down; bit0=0 normal
	char SpecicalInfo;
}Usr_EDID_t;
*/
extern int project_id;
extern panel_config_t *panel_param;
extern int hdmi_420Mode;

void mdelay ( int ms );

extern enum panel_interface_e get_panel_interface ( void );
extern vpu_timing_t get_timing_mode ( void );

extern int panel_disable ( void );
extern int panel_enable ( void );

void power_on_aml ( void );
void card_system_pw(void);

extern void backlight_power_ctrl(char val);
extern void backlight_set_level(int level);
//extern int backight_get_level(void);

extern int get_panel_def_id ( void );
extern int get_panel_max_count(void);

extern void panel_pre_load ( void );

#define IS_1080P(timing)  ((timing == TIMING_1920x1080P60) || \
			  (timing == TIMING_1920x1080P100) || \
			  (timing == TIMING_1920x1080P120) || \
			  (timing == TIMING_1920x1080P60_3D_SG) || \
			  (timing == TIMING_1920x1080P240) || \
			  (timing == TIMING_1920x1080P120_3D_SG))

#endif

