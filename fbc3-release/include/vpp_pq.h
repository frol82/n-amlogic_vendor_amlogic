#ifndef VPP_PQ_H
#define VPP_PQ_H

#include <vpp_api.h>
#include <vpp.h>


void vpu_pq_init_csc01 (int csc0_mode, int csc1_mode);
void vpu_pq_init_cm2 ( int hsize, int vsize , vpu_config_t *pdb_table);
unsigned int pq_data_mapping ( unsigned int wb_gain, unsigned int ui_range, unsigned int max_range, int enable );

void cfg_xvycc_inv_lut ( int y_en,
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


void cfg_xvycc_lut ( int r_en,
							int g_en,
							int b_en,
							int pos_scale,
							int neg_scale,
							int *r_lut_reg,  //s12
							int *g_lut_reg,
							int *b_lut_reg
						  );


void xvycc_reg_check();
void set_xvycc ( int xvycc_mode, int vadj_en, int line_lenm1 );


void vpu_pq_init_gamma ( vpu_config_t *pdb_table );
void d2d3_select ( int mode );

void backlight_pwm_first_config ( void );
void backlight_pwm_second_config ( void );





#endif
