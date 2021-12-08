#ifndef _VPU_UTIL_H_
#define _VPU_UTIL_H_
#include <vpp.h>

#define K_SWCLK_IN_VS	0

typedef enum src_mode_e {
	SRC_MODE_OSC = 0,
	SRC_MODE_HDMI,
} src_mode_t;

int vpu_write_lut ( int *pData, int mode_2data, int lut_idx, int lut_offs, int lut_size );
int vpu_read_lut ( int *pData, int mode_2data, int lut_idx, int lut_offs, int lut_size );

int vpu_read_lut_new ( int lut_idx, int lut_size, int lut_offs, int *pData, int mode_2data );
int vpu_write_lut_new ( int lut_idx, int lut_size, int lut_offs, int *pData, int mode_2data );
//int vpu_read_lut_new (int lutIdx, int size, int saddr, int *pData, int twoDatas);
//int vpu_write_lut_new(int lutIdx, int size, int saddr, int *pData, int twoDatas);

//=======================================
void vpu_initial ( int hsize, int vsize );
void enable_vpu ( int enable );
void set_init_tm_rcvy ( int enable, int hs_width, int vs_width );
void set_tm_rcvy ( int enable, int hs_width, int vs_width );


void enable_patgen ( int enable );
void set_patgen ( pattern_mode_t mode );
void set_patgen_yuv ( pattern_mode_t mode );
//void enable_csc0_byVs ( int enable );
void enable_csc0 ( int enable );
void enable_dpln ( int enable );
void dnlp_config ( int hsize, int vsize, int dnlp_en, int hist_win_en, int luma_hist_spl_en );
void dnlp_set_3dmode ( int hist_3dmode, int hist_3dmodelr_xmid );
void enable_cm2 ( int enable );
void config_cm2_lut ( int *pBuf, int sizeItem );
void cm2_config ( int hsize, int vsize, const int CM2_SatGLBgain_via_Y[9], int hue, int sat, int cm2_en, int cm2_filter_en, int hue_adj_en, int sat_adj_en, int luma_adj_en, int cm2_Sat_adj_via_hs, int cm2_Sat_adj_via_y, int cm2_Lum_adj_via_h, int cm2_Hue_adj_via_hsv );
void enable_bst ( int enable );
void enable_csc1 ( int enable );
void enable_rgbbst ( int enable );
void enable_blend ( int enable );
void enable_xvycc_lut ( int enable );
void enable_csc2 ( int enable );
void vpu_demo_ctrl ( void );

void hdmirx_switch_clocksrc( src_mode_t  Srcmode );

#if 0
	void enable_sosd ( int enable );
	void config_sosd_char_lut ( int *pBuf, int sizeItem );
	void config_sosd_font_lut ( int *pBuf, int sizeItem );
	void config_sosd_char_ram ( int ram_char_sel );
	void sosd_config ( int hsize, int  vsize, int sosd_3d_mode, int ram_char_sel, int ram_char_sync_mode );
#endif
void enable_wb ( int enable );
void enable_gamma ( int enable );
void config_gamma_mod ( vpu_gammamod_t mode );
void config_gamma_lut ( int Idx, int *pBuf, int sizeItem );
void enable_demura ( int enable );

void config_pwm ( unsigned short pwm_freq, vpu_timing_t timing, unsigned short pwm_duty );
void config_3dsync_3dgls();

/*kuka@20140618 add begin*/
typedef struct vframe_hist_s {
	unsigned int   hist_pow;
	unsigned int   luma_sum;
	unsigned int   chroma_sum;
	unsigned int   pixel_sum;  // [31:30] POW [21:0] PIXEL_SUM
	unsigned int   height;
	unsigned int   width;
	unsigned int   bin_mode;	 // 1:32bin mode;0:64bin mode
	unsigned char  luma_max;
	unsigned char  luma_min;
	unsigned int	gamma[66];
} vframe_hist_t;
extern void vpu_get_hist_info ( void );
extern void init_dnlp_para ( void );
extern void ve_dnlp_cal ( void );
extern void vpu_get_gamma_lut ( int Idx, int *pBuf, int sizeItem );
extern void vpu_get_gamma_lut_pq ( int Idx, int *pBuf, int sizeItem );
extern void vpu_set_gamma_lut ( int Idx, int *pBuf, int sizeItem );
extern void enable_output ( int enable );
extern void enable_timgen ( int enable );
extern void enable_dnlp ( int enable );
extern void set_dnlp_parm1 ( unsigned int *param );
extern void set_dnlp_parm2 ( unsigned int *param );
extern void get_dnlp_parm ( void );
extern void vpu_backlight_adj ( unsigned int val_ui, vpu_timing_t timing );
extern void vpu_bri_adj ( unsigned int bri_ui );
extern void vpu_con_adj ( unsigned int con_ui );
extern void vpu_wb_gain_adj ( unsigned int gain, vpu_wbsel_t rgb_sel );
extern void vpu_wb_offset_adj ( int offset, vpu_wbsel_t rgb_sel, vpu_wboffset_pos_t pre_post );
extern void vpu_saturation_adj ( unsigned int val_ui );
extern void vpu_hue_adj ( unsigned int val_ui );
extern void vpu_pq_picmod_adj ( vpu_picmod_e val_ui );
//extern void vpu_colortemp_adj ( vpu_wb_e val_ui );
extern void vpu_colortemp_adj ( vpu_wb_t *wb_data );
extern void vpu_color_bar_mode ( void );
extern void vpu_patgen_bar_set ( unsigned int r_val, unsigned int g_val, unsigned int b_val );
extern void vpu_testpat_def ( void );
extern void vpu_whitebalance_init ( void );
extern int vpu_whitebalance_status ( void );
extern void vpu_timing_change_process ( void );
extern void vpu_csc_config ( unsigned int sel );
extern void vpu_srcif_debug ( unsigned int mode, unsigned int mux );
extern vpu_srcif_mode_t vpu_get_srcif_mode ( void );
extern void fbc_demura_set ( unsigned int leak_light, unsigned int threshold );
extern void fbc_adj_bri ( unsigned int bri_ui );
extern void fbc_adj_con ( unsigned int con_ui );
extern void fbc_adj_sat ( unsigned int sat_ui );
extern void fbc_adj_hue ( unsigned int hue_ui );
extern void set_color_gamut ( int mode );
extern void vpu_set_color_surge ( unsigned int mode, vpu_timing_t timing_cur );
extern void vpu_csc_config_ext ( unsigned int sel );

//extern int nvm_write_gamma_idx ( int temp_value );


/*kuka@20140618 add end*/
//
void cfg_vadj ( int vadj_en,
				int vadj_minus_black_en,
				int vadj_bri, //9 bit
				int vadj_con, //8 bit
				int vadj_ma,  //10 bit
				int vadj_mb,  //10 bit
				int vadj_mc,  //10 bit
				int vadj_md,  //10 bit
				int soft_curve_0_a,  //12 bit
				int soft_curve_0_b,  //12 bit
				int soft_curve_0_ci, //8 bit
				int soft_curve_0_cs, //3 bit
				int soft_curve_0_g,  //9 bit
				int soft_curve_1_a,
				int soft_curve_1_b,
				int soft_curve_1_ci,
				int soft_curve_1_cs,
				int soft_curve_1_g
			  );

void cfg_csc1 ( int mat_conv_en,
				int coef00,
				int coef01,
				int coef02,
				int coef10,
				int coef11,
				int coef12,
				int coef13,
				int coef14,
				int coef15,
				int coef20,
				int coef21,
				int coef22,
				int coef23,
				int coef24,
				int coef25,
				int offset0,
				int offset1,
				int offset2,
				int pre_offset0,
				int pre_offset1,
				int pre_offset2,
				int conv_cl_mod,
				int conv_rs,
				int clip_enable,
				int probe_x,
				int probe_y,
				int highlight_color,
				int probe_post,
				int probe_en,
				int line_lenm1,
				int highlight_en
			  );

void cfg_csc2 ( int mat_conv_en,
				int coef00,
				int coef01,
				int coef02,
				int coef10,
				int coef11,
				int coef12,
				int coef13,
				int coef14,
				int coef15,
				int coef20,
				int coef21,
				int coef22,
				int coef23,
				int coef24,
				int coef25,
				int offset0,
				int offset1,
				int offset2,
				int pre_offset0,
				int pre_offset1,
				int pre_offset2,
				int conv_cl_mod,
				int conv_rs,
				int clip_enable,
				int probe_x,
				int probe_y,
				int highlight_color,
				int probe_post,
				int probe_en,
				int line_lenm1,
				int highlight_en
			  );

void cfg_clip ( int r_top,
				int r_bot,
				int g_top,
				int g_bot,
				int b_top,
				int b_bot
			  );


#endif

