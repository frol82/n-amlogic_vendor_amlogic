#ifndef VPP_API_H
#define VPP_API_H

#include <cmd.h>

/*for factory menu begin*/
#define FBC_BRI_RANGE_UI 255		//brightness range:0~255
#define FBC_CON_RANGE_UI 255		//contrast range:0~255
#define FBC_SAT_RANGE_UI 255		//saturation range:0~255
#define FBC_HUE_RANGE_UI 255		//hue range:0~255
#define FBC_SHARPNESS_RANGE_UI 255	//sharpness range:0~255
#define FBC_BACKLIGHT_RANGE_UI 255	//backlight range:0~255
#define FBC_WB_GAIN_RANGE_UI 255	//wihte balance gain range:0~255
#define FBC_WB_OFFSET_RANGE_UI 255	//wihte balance offset range:0~255
#define FBC_BACKLIGHT_RANGE_REG 255	//backlight reg_val for naturelight range:0~255
/*for factory menu end*/

typedef enum demura_lutidx_e {
	FBC_DEMURE_BLREF_ODD_TABLE = 0x10,
	FBC_DEMURE_R_GAMMA_ODD_TABLE,
	FBC_DEMURE_R_DE_GAMMA_ODD_TABLE,
	FBC_DEMURE_LU_COEFF_ODD_TABLE,
	FBC_DEMURE_G_GAMMA_ODD_TABLE,
	FBC_DEMURE_G_DE_GAMMA_ODD_TABLE,
	FBC_DEMURE_B_GAMMA_ODD_TABLE,
	FBC_DEMURE_B_DE_GAMMA_ODD_TABLE,//0x17
	FBC_DEMURE_BLREF_EVEN_TABLE,
	FBC_DEMURE_R_GAMMA_EVEN_TABLE,
	FBC_DEMURE_R_DE_GAMMA_EVEN_TABLE,
	FBC_DEMURE_G_GAMMA_EVEN_TABLE,
	FBC_DEMURE_G_DE_GAMMA_EVEN_TABLE,
	FBC_DEMURE_B_GAMMA_EVEN_TABLE,
	FBC_DEMURE_B_DE_GAMMA_EVEN_TABLE,
	FBC_DEMURE_LU_COEFF_EVEN_TABLE,//0x1f
} demura_lutidx_t;
typedef struct fbc_hist_s {
	unsigned int  hist_data[34];
} fbc_hist_t;

//for PQ

typedef enum vpu_modules_e {
	VPU_MODULE_NULL = 0,
	VPU_MODULE_VPU,//vpu uint
	VPU_MODULE_TIMGEN,
	VPU_MODULE_PATGEN,
	VPU_MODULE_GAMMA,
	VPU_MODULE_WB,//WhiteBalance
	VPU_MODULE_BC,//Brightness&Contrast
	VPU_MODULE_BCRGB,//RGB Brightness&Contrast
	VPU_MODULE_CM2,
	VPU_MODULE_CSC1,
	VPU_MODULE_DNLP,
	VPU_MODULE_CSC0,
	VPU_MODULE_CSC2,
	VPU_MODULE_XVYCC_LUT,
	VPU_MODULE_OSD,
	VPU_MODULE_BLEND,
	VPU_MODULE_DEMURE,//15
	VPU_MODULE_OUTPUT,//LVDS/VX1 output
	VPU_MODULE_OSDDEC,//OSD decoder
	VPU_MODULE_MAX,
} vpu_modules_t;

typedef struct vpu_gamma_s {
	unsigned short gamma_r[257];
	unsigned short gamma_g[257];
	unsigned short gamma_b[257];
} vpu_gamma_t;

typedef struct vpu_bri_con_s {
	unsigned int bri_con;
} vpu_bri_con_t;

typedef struct vpu_dnlp_table_s {
	unsigned int hist_ctrl;
	char ve_dnlp_mvreflsh;
	char ve_dnlp_schg_sft;
	char ve_dnlp_cuvbld_min;
	char ve_dnlp_cuvbld_max;
	char ve_dnlp_bin0_absmax;
	char ve_dnlp_bin0_sbtmax;
	char ve_dnlp_gmma_rate;
	char ve_dnlp_lowrange;
	char ve_dnlp_hghrange;
	char ve_dnlp_sbgnbnd;
	char ve_dnlp_sendbnd;
	char ve_dnlp_lowalpha_new;
	char ve_dnlp_hghalpha_new;
	char ve_dnlp_adpalpha_lrate;
	char ve_dnlp_adpalpha_hrate;
	char ve_dnlp_cliprate_new;
	char ve_dnlp_cliprate_min;
	char ve_dnlp_adpcrat_lbnd;
	char ve_dnlp_adpcrat_hbnd;
	char ve_dnlp_clashBgn;
	char ve_dnlp_clashEnd;
	char ve_dnlp_hghbin;
	char ve_dnlp_hghnum;
	char ve_dnlp_lowbin;
	char ve_dnlp_lownum;
	char ve_mtdbld_rate;
	char ve_dnlp_adpmtd_lbnd;
	char ve_dnlp_adpmtd_hbnd;
	char ve_dnlp_blkext;
	char ve_dnlp_bextmx;
	char ve_dnlp_wextmx;
	char ve_dnlp_wext_autorat;
	char ve_dnlp_bkgend;
	char ve_dnlp_bkgert;
	char ve_dnlp_pst_gmarat;
	char ve_dnlp_pstgma_brghtrate;
	char ve_dnlp_pstgma_brghtrat1;
	char ve_dnlp_slow_end;
	char ve_dnlp_almst_wht;
	char ve_dnlp_brght_add;
	char ve_dnlp_satur_rat;
	char ve_dnlp_satur_max;
	char ve_blk_prct_rng;
	char ve_blk_prct_max;
} vpu_dnlp_table_t;

typedef struct vpu_sat_hue_s {
	unsigned short hue;
	unsigned short sat;
} vpu_sat_hue_t;

typedef struct vpu_hdr_param_s {
	int group_knee[11];
	unsigned int kneefactor;
	unsigned int interpolatmode;
	unsigned int rghtshift;
	unsigned int pre_offset0;
	unsigned int pre_offset1;
	unsigned int pre_offset2;
	unsigned int coef[9];
	unsigned int pst_offset0;
	unsigned int pst_offset1;
	unsigned int pst_offset2;
} vpu_hdr_param_t;

typedef struct vpu_ldim_param_s {
	/* beam model */
	short rgb_base;
	short boost_gain;
	short lpf_res;
	short fw_ld_th_sf; /* spatial filter threshold */
	/* beam curve */
	short ld_vgain;
	short ld_hgain;
	short ld_litgain;
	short ld_lut_vdg_lext;
	short ld_lut_hdg_lext;
	short ld_lut_vhk_lext;
	short ld_lut_hdg[32];
	short ld_lut_vdg[32];
	short ld_lut_vhk[32];
	/* beam shape minor adjustment */
	short ld_lut_vhk_pos[32];
	short ld_lut_vhk_neg[32];
	short ld_lut_hhk[32];
	char ld_lut_vho_pos[32];
	char ld_lut_vho_neg[32];
	/* remapping */
	short lit_idx_th;
	short comp_gain;
}vpu_ldim_param_t;

typedef struct vpu_wb_s {
	unsigned short gain_r;
	unsigned short gain_g;
	unsigned short gain_b;
	unsigned short pre_offset_r;
	unsigned short pre_offset_g;
	unsigned short pre_offset_b;
	unsigned short post_offset_r;
	unsigned short post_offset_g;
	unsigned short post_offset_b;
} vpu_wb_t;

#pragma pack(2)
//PQ DB data global buffer (for reduce space)
typedef struct vpu_config_s {
	//db crc
	unsigned int crc;
	//db length
	unsigned int db_len;
	//version info
	unsigned int version;
	//top ctrl flag
	unsigned int ctrl_flag;
	//brihtness&contrast
	unsigned int bri_con_index;
	vpu_bri_con_t bri_con[11];
	//wb
	vpu_wb_t wb[3];
	//gamma
	unsigned int gamma_index;
	vpu_gamma_t gamma[1];
	//cm2
	//unsigned int cm2[188];
	unsigned int cm2[169];
	int temp_data;
	unsigned int cm2_rsv[19];
	//dnlp
	vpu_dnlp_table_t dnlp;
	//saturation
	unsigned int sat_hue_index;
	vpu_sat_hue_t sat_hue[11];
	/* local dimming */
	struct vpu_ldim_param_s ldim_param;
	//HDR
	vpu_hdr_param_t hdr_param;
} vpu_config_t;

//PQ DB data structure
typedef struct vpu_db_config_s {
	//db crc
	unsigned int crc;
	//db length
	unsigned int db_len;
	//version info
	unsigned int version;
	//top ctrl flag
	unsigned int ctrl_flag;
	//brihtness&contrast
	unsigned int bri_con_index;
	vpu_bri_con_t bri_con[11];
	//wb
	vpu_wb_t wb[3];
	//gamma
	unsigned int gamma_index;
	vpu_gamma_t gamma[9];
	//cm2
	//unsigned int cm2[188];
	unsigned int cm2[169];
	int temp_data;
	unsigned int cm2_rsv[19];
	//dnlp
	vpu_dnlp_table_t dnlp;
	//saturation
	unsigned int sat_hue_index;
	vpu_sat_hue_t sat_hue[11];
	/* local dimming */
	struct vpu_ldim_param_s ldim_param;
	//HDR
	vpu_hdr_param_t hdr_param;
} vpu_db_config_t;

typedef struct vpu_pwm_channel_set_s {
	unsigned char pwm_channel; ///< pwm channel
	unsigned int period; ///< pwm period
	unsigned int duty; ///< pwm duty
} vpu_pwm_channel_set_t;

#pragma pack()

//extern void register_vpp_save(save_parameter func, read_parameter read_func);
extern void register_vpp_save ( save_parameter func );
extern void register_backlight_func ( backlight_func func );

extern fbc_hist_t *fbc_hist_info ( void );
extern int fbc_avg_lut ( void );
extern int fbc_bri_convert ( int ui_val );
extern int fbc_con_convert ( int ui_val );
extern void fbc_bri_set ( int reg_val );
extern void fbc_con_set ( int reg_val );
extern void fbc_demura_enable ( unsigned int en );
extern void fbc_demura_set ( unsigned int leak_light, unsigned int threshold );
extern void fbc_demura_load_table ( demura_lutidx_t table_index, int *data, int sizeItem );
//extern void fbc_dynamic_contrast_enable ( int enable );
//extern void fbc_switch_to_hdmi ( int enable );
extern void fbc_set_gamma ( int index );
//extern int fbc_get_gamma ( void );
extern void fbc_adj_bri ( unsigned int bri_ui );
extern void fbc_adj_con ( unsigned int con_ui );
extern void fbc_adj_sat ( unsigned int sat_ui );
extern void fbc_adj_hue ( unsigned int hue_ui );
extern void vpu_wb_gain_r ( unsigned int gain_ui );
extern void vpu_wb_gain_g ( unsigned int gain_ui );
extern void vpu_wb_gain_b ( unsigned int gain_ui );
extern void vpu_wb_preoffset_r ( unsigned int offset_ui );
extern void vpu_wb_preoffset_g ( unsigned int offset_ui );
extern void vpu_wb_preoffset_b ( unsigned int offset_ui );
//extern char fbc_hdmirx_5v_power ( void );
#endif

