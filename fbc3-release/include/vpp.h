#ifndef VPP_H
#define VPP_H
#include <cmd.h>
#include <panel.h>
#include <vpp_api.h>
#include <listop.h>
#include <data_struct.h>



#define VPU_VER "Ref.2014/12/22"

#define K_PC_MODE_ON		1
#define K_PC_MODE_OFF		0


typedef enum vpu_lutidx_e {
	GAMMA_R0 = 0,
	GAMMA_G0,
	GAMMA_B0,
	GAMMA_R1,
	GAMMA_G1,
	GAMMA_B1,
	DEMURE_BLREF_ODD_TABLE = 0x10,
	DEMURE_R_GAMMA_ODD_TABLE,
	DEMURE_R_DE_GAMMA_ODD_TABLE,
	DEMURE_LU_COEFF_ODD_TABLE,
	DEMURE_G_GAMMA_ODD_TABLE,
	DEMURE_G_DE_GAMMA_ODD_TABLE,
	DEMURE_B_GAMMA_ODD_TABLE,
	DEMURE_B_DE_GAMMA_ODD_TABLE,
	DEMURE_BLREF_EVEN_TABLE,
	DEMURE_R_GAMMA_EVEN_TABLE,
	DEMURE_R_DE_GAMMA_EVEN_TABLE,
	DEMURE_G_GAMMA_EVEN_TABLE,
	DEMURE_G_DE_GAMMA_EVEN_TABLE,
	DEMURE_B_GAMMA_EVEN_TABLE,
	DEMURE_B_DE_GAMMA_EVEN_TABLE,
	DEMURE_LU_COEFF_EVEN_TABLE,
	OSD_FRONT_RAM = 0X20,
	OSD_CHAR_RAM,
	CM2_DATA_RAM,
	VPU_LUTIDX_MAX,
} vpu_lutidx_t;

//===== sub-unit: PatternGen
//mode == 10, default value, it's ramp
//mode == 11, grid9
//mode == 12, cir15
//mode == 13, palette
//mode == 14, triangle
//mode == 15, colorbar
//mode == 16, colorbar, LR
//mode == 17, two color TB
//mode == 18, two color line by line
//mode == 19, two color pixel by pixel
//===new pattern for client==
//mode == 0, off
//mode == 1, cir9
//mode == 2, 100% grey
//mode == 3, 20% grey
//mode == 4, black
//mode == 5, red
//mode == 6, green
//mode == 7, blue
//mode == 8, grey level
//mode == 9, pallet

typedef enum pattern_mode_e {
	PATTERN_MODE_OFF = 0,
	PATTERN_MODE_CIR9,
	PATTERN_MODE_GREY100,
	PATTERN_MODE_GREY20,
	PATTERN_MODE_BLACK,
	PATTERN_MODE_RED,
	PATTERN_MODE_GREEN,
	PATTERN_MODE_BLUE,
	PATTERN_MODE_GREYLEVEL,
	PATTERN_MODE_PALLET,
	PATTERN_MODE_RAMP,
	PATTERN_MODE_GRID9,
	PATTERN_MODE_CIR15,
	PATTERN_MODE_PALETTE,
	PATTERN_MODE_TRIANGLE,
	PATTERN_MODE_COLORBAR,
	PATTERN_MODE_COLORBAR_LR,
	PATTERN_MODE_COLORBAR_TB,
	PATTERN_MODE_TWO_COLOR_LINE_BY_LINE,
	PATTERN_MODE_TWO_COLOR_PIXEL_BY_PIXEL,
	PATTERN_MODE_AX,
} pattern_mode_t;

typedef enum vsAction {
	eVSACTION_PCMODE_ON = 0x00000001,
	eVSACTION_PCMODE_OFF = 0x00000002,
	eVSACTION_PATTERN_MODE = 0x00000004,
	eVSACTION_HDMI_MODE = 0x00000008,
	eVSACTION_PATTERN_SW = 0x00000010,
} vsAction_e;

typedef struct vpu_timing_table_s {
	vpu_timing_t timing;
	unsigned int hactive;
	unsigned int vactive;
	unsigned int htotal;
	unsigned int vtotal;
} vpu_timing_table_t;

typedef enum vpu_source_e {
	SOURCE_NULL = 0,
	SOURCE_AV,
	SOURCE_HDMI,
	SOURCE_MAX,
} vpu_source_t;

typedef enum vpu_gammamod_e {
	GAMMA_BEFORE = 0,   //0:before osd blender;
	GAMMA_AFTER,        //1:after osd blender
} vpu_gammamod_t;

typedef enum vpu_gammacolor_e {
	GAMMA_R = 0,
	GAMMA_G,
	GAMMA_B,
	GAMMA_MAX,
} vpu_gammacolor_t;
typedef enum vpu_wbsel_e {
	WBSEL_R = 0,
	WBSEL_G,
	WBSEL_B,
} vpu_wbsel_t;
typedef enum vpu_wboffset_pos_e {
	WBOFFSET_PRE = 0,
	WBOFFSET_POST,
} vpu_wboffset_pos_t;

typedef enum vpu_gammalever_e {
	GAMMA_LEVER0 = 0,
	GAMMA_LEVER1,
	GAMMA_LEVER2,
	GAMMA_LEVERMAX,
} vpu_gammalever_t;

#define GAMMA_ITEM 257

typedef enum vpu_hdrcolor_e {
	HDR_R = 0x28,
	HDR_G,
	HDR_B,
	HDR_MAX,
} vpu_hdrcolor_t;
#define HDR_ITEM 289

typedef enum vpu_srcif_mode_e {
	SRCIF_PURE_HARDWARE = 0,
	SRCIF_HYBRID,
	SRCIF_PURE_SOFTWARE,
} vpu_srcif_mode_t;

typedef enum metal_revision_e {
	REV_A = 0,
	REV_B,
	REV_C,
} metal_revision_t;

typedef struct vpu_message_s {
	fbc_command_t cmd_id;
	unsigned int parameter1;
	unsigned int parameter2;
	unsigned int parameter3;
} vpu_message_t;

typedef struct vpu_message_m {
	unsigned int param1;
	unsigned int param2;
	unsigned int param3;
	unsigned int param4;
	unsigned int param5;
	unsigned int param6;
	unsigned int param7;
	unsigned int param8;
} vpu_message_n;

typedef struct vpu_debug_s {
	char *name;
	vpu_message_t vpu_para;
} vpu_debug_t;

//PQ TOOL no release
typedef struct vpu_cm2_s {
	unsigned short rw_flag;//0:read;1:write
	unsigned int addr;
	unsigned int data[5];
} vpu_cm2_t;


typedef struct stswitch_clkSrc
{
	int gswitch_clk_src_flag;
	int gswitch_clk_src_mode;
	int gswitch_clk_src_delay;
}gstSwClkSrc;

extern gstSwClkSrc gswitchClk;

extern void init_vpp ( void );
extern void start_vpp ( void );

#if 0
	extern void vpu_factory_init ( void );
#endif


extern void init_display ( void );
extern int get_ic_version ( void );


#define FBC_TOOL_EN  1
#define DEMURA_DEBUG_DATA_NUM 200

#define LOW_TEMP     0
#define NORMAL_TEMP  1

//
extern void cfg_xvycc_inv_lut ( int y_en,
								int y_pos_scale, //u2
								int y_neg_scale,
								int *y_lut_reg,  //s12
								int u_en,
								int u_pos_scale,
								int u_neg_scale,
								int *u_lut_reg,
								int v_en,
								int v_pos_scale,
								int v_neg_scale,
								int *v_lut_reg
							  );


extern void cfg_xvycc_lut ( int r_en,
							int g_en,
							int b_en,
							int pos_scale,
							int neg_scale,
							int *r_lut_reg,  //s12
							int *g_lut_reg,
							int *b_lut_reg
						  );



void xvycc_reg_check();

extern int vpp_task_handle ( int task_id, void *param );
//extern int vpp_check_cmd_is_supported ( int cmd );
extern unsigned int vpp_handle_cmd ( unsigned char *s, int *rets );

extern void set_xvycc ( int xvycc_mode, int vadj_en, int line_lenm1 );

extern vpu_config_t vpu_config_table;
extern vpu_colortemp_table_t colortemp_table[eWB_MAX];
//extern const vpu_picmod_table_t picmod_table[PICMOD_MAX];
extern unsigned int pq_data_mapping ( unsigned int wb_gain, unsigned int ui_range, unsigned int max_range, int enable );
extern void vpp_dump_hist(void);
extern void vpp_dump_gamma(void);
extern void vpp_dump_hdr(void);
extern void vpu_pattern_Switch_byVs( pattern_mode_t mode );
extern int burn_mode(int enable);
extern void vpp_set_gamma_pattern(int r, int g, int b);
extern void init_load_user_setting(void);
extern void vpu_pattern_Switch ( pattern_mode_t mode );

extern void vpu_pc_mode_onoff_pro(int onoff);

extern void vpu_set_vs_action(vsAction_e mode);

extern void vpu_clear_vs_action(vsAction_e mode);


/*parameter define*/
//extern int dnlp_en_flag;

extern int lockn_en;
extern int vpp_task_id;
extern vpu_timing_t timing_cur;
extern enum panel_interface_e panel_if;
extern unsigned int vpu_tm_revy_data;
extern unsigned char uc_switch_freq; /* 0: 4k2k60hz420 1:4k2k50hz420 */
#ifdef ENABLE_AVMUTE_CONTROL
	extern char get_avmute_flag;
#endif
extern int csc0_flag;
extern int csc0_value;
extern int srcif_fsm_on_flag;
extern int srcif_fsm_off_flag;
extern int srcif_pure_ptn_flag;
extern int srcif_pure_hdmi_flag;
extern int srcif_pure_hdmi_counter;
extern gstSwClkSrc gswitchClk;

extern int knee_factor; /* 0 ~ 256, 128 = 0.5 */
extern int knee_interpolation_mode;  /* 0: linear, 1: cubic */
extern int knee_setting[11];
extern int num_knee_setting;

extern unsigned int dnlp_printk;

extern unsigned char *vpp_debug;

extern const vpu_timing_table_t timing_table[TIMING_MAX];
extern unsigned int ( *save ) ( unsigned char *s );

#ifdef IN_FBC_MAIN_CONFIG
extern int panel_id;
extern vpu_source_t source_cur;
extern vpu_gammamod_t gammamod_cur;
extern unsigned int auto_backlight_en;/* for nature light on/off */
//extern vpu_wb_e colortemp_cur;
extern vpu_picmod_e picmod_cur;
extern vpu_gammalever_t gammalever_cur;
extern int setpatternflag;
extern int csc_config;/* 0:default setting,1:force output black;2:force set csc1 to default;3:pretect mode */
extern int csc_config_cnt;
extern unsigned char con_dimming_pwm;

extern int burn_task_id;
extern int burn_pattern_mode;
extern int ret_burn_id;
extern struct list_head vpp_cmd_list;

//extern vpu_fac_pq_t vpu_fac_pq_setting;
#endif

extern void register_vpp_task_mid();

#endif
