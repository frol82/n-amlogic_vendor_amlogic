#include <common.h>
#include <vpu_util.h>
#include <vpp.h>
#include <vpp_api.h>
#include <log.h>
#include <listop.h>
#include <inputdata.h>
#include <task.h>
#include <input.h>
#include <malloc.h>
#include <command.h>
#include <stdlib.h>
#include <cmd.h>
#include <task_priority.h>
#include <project.h>
#include <user_setting.h>
#include <hdmirx.h>
#include <hdmirx_parameter.h>
#include <board_config.h>
#include <panel.h>
#include <vpp_pq.h>

/* ----------------------------------------------- */
/* Global variables */
/* ----------------------------------------------- */
unsigned int ( *save ) ( unsigned char *s );

//extern int setting_task_id;


/* static read_parameter vpp_read_param = NULL; */
/* void register_vpp_save(save_parameter save_func, read_parameter read_func) */
void register_vpp_save ( save_parameter save_func )
{
	save = save_func;
	/* vpp_read_param = read_func; */
}

/*use for console debug below*/

void vpp_module_enable_bypass ( vpu_modules_t module, int enable )
{
	printf ( "[vpp.c]%s,module:%d,enable:%d.\n", __func__, module, enable );

	switch ( module ) {
		case VPU_MODULE_VPU:
			enable_vpu ( enable );
			break;

		case VPU_MODULE_TIMGEN:
			enable_timgen ( enable );
			break;

		case VPU_MODULE_PATGEN:
			enable_patgen ( enable );
			break;

		case VPU_MODULE_GAMMA:
			enable_gamma ( enable );
			break;

		case VPU_MODULE_WB:
			enable_wb ( enable );
			break;

		case VPU_MODULE_BC:
			enable_bst ( enable );
			break;

		case VPU_MODULE_BCRGB:
			enable_rgbbst ( enable );
			break;

		case VPU_MODULE_CM2:
			enable_cm2 ( enable );
			break;

		case VPU_MODULE_CSC1:
			enable_csc1 ( enable );
			break;

		case VPU_MODULE_DNLP:
			//dnlp_en_flag = enable;
			enable_dnlp(enable);
			break;

		case VPU_MODULE_CSC0:
			enable_csc0 ( enable );
			break;

		case VPU_MODULE_CSC2:
			enable_csc2 ( enable );
			break;

		case VPU_MODULE_XVYCC_LUT:
			enable_xvycc_lut ( enable );

		case VPU_MODULE_OSD:
			break;

		case VPU_MODULE_BLEND:
			enable_blend ( enable );
			break;

		case VPU_MODULE_DEMURE:
			enable_demura ( enable );
			break;

		case VPU_MODULE_OUTPUT:
			enable_output ( enable );
			break;

		case VPU_MODULE_OSDDEC:
			break;

		default:
			break;
	}
}

void vpp_process_wb_getting ( vpu_message_t message, int *rets )
{
	//vpu_fac_pq_t* pvpu_fac_pq = &vpu_fac_pq_setting;
	vpu_wb_t tabWb;

	nvm_read_wb_param(nvm_read_wb_mode(),&tabWb);

	switch ( message.cmd_id & 0x7f ) {
		case VPU_CMD_RED_GAIN_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.gain_r;
			rets[0] = tabWb.gain_r;
			break;

		case VPU_CMD_GREEN_GAIN_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.gain_g;
			rets[0] = tabWb.gain_g;
			break;

		case VPU_CMD_BLUE_GAIN_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.gain_b;
			rets[0] = tabWb.gain_b;
			break;

		case VPU_CMD_PRE_RED_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.pre_offset_r;
			rets[0] = tabWb.pre_offset_r;
			break;

		case VPU_CMD_PRE_GREEN_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.pre_offset_g;
			rets[0] = tabWb.pre_offset_g;
			break;

		case VPU_CMD_PRE_BLUE_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.pre_offset_b;
			rets[0] = tabWb.pre_offset_b;
			break;

		case VPU_CMD_POST_RED_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.post_offset_r;
			rets[0] = tabWb.post_offset_r;
			break;

		case VPU_CMD_POST_GREEN_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.post_offset_g;
			rets[0] = tabWb.post_offset_g;
			break;

		case VPU_CMD_POST_BLUE_OFFSET_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_user.post_offset_b;
			rets[0] = tabWb.post_offset_b;
			break;

		case VPU_CMD_WB:
			rets[0] = vpu_whitebalance_status();
			break;

		default:
			break;
	}
}


void vpp_process_wb_setting ( vpu_message_t message )
{
	//vpu_fac_pq_t* pvpu_fac_pq = &vpu_fac_pq_setting;
	vpu_wb_t tabWb;

	nvm_read_wb_param(nvm_read_wb_mode(),&tabWb);

	switch ( message.cmd_id ) {
		case VPU_CMD_RED_GAIN_DEF:
			//pvpu_fac_pq->colortemp_user.gain_r = message.parameter1;
			tabWb.gain_r = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_gain_adj ( message.parameter1, WBSEL_R );
			break;

		case VPU_CMD_GREEN_GAIN_DEF:
			//pvpu_fac_pq->colortemp_user.gain_g = message.parameter1;
			tabWb.gain_g = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_gain_adj ( message.parameter1, WBSEL_G );
			break;

		case VPU_CMD_BLUE_GAIN_DEF:
			//pvpu_fac_pq->colortemp_user.gain_b = message.parameter1;
			tabWb.gain_b = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_gain_adj ( message.parameter1, WBSEL_B );
			break;

		case VPU_CMD_PRE_RED_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.pre_offset_r = message.parameter1;
			tabWb.pre_offset_r = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_R, WBOFFSET_PRE );
			break;

		case VPU_CMD_PRE_GREEN_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.pre_offset_g = message.parameter1;
			tabWb.pre_offset_g = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_G, WBOFFSET_PRE );
			break;

		case VPU_CMD_PRE_BLUE_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.pre_offset_b = message.parameter1;
			tabWb.pre_offset_b = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_B, WBOFFSET_PRE );
			break;

		case VPU_CMD_POST_RED_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.post_offset_r = message.parameter1;
			tabWb.post_offset_r = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_R, WBOFFSET_POST );
			break;

		case VPU_CMD_POST_GREEN_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.post_offset_g = message.parameter1;
			tabWb.post_offset_g = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_G, WBOFFSET_POST );
			break;

		case VPU_CMD_POST_BLUE_OFFSET_DEF:
			//pvpu_fac_pq->colortemp_user.post_offset_b = message.parameter1;
			tabWb.post_offset_b = message.parameter1;
			nvm_write_wb_param(nvm_read_wb_mode(),&tabWb,0);
			vpu_wb_offset_adj ( message.parameter1, WBSEL_B, WBOFFSET_POST );
			break;

		case VPU_CMD_WB:
			vpu_whitebalance_init();
			break;

		default:
			break;
	}
}

void vpp_process_debug ( vpu_message_t message )
{
	switch ( message.cmd_id ) {
		case VPU_CMD_HIST:
			vpp_dump_hist();
			break;

		case VPU_CMD_BLEND:
			enable_blend ( message.parameter1 );
			break;

		case VPU_CMD_DEMURA:
			break;

		case VPU_CMD_CSC:
			break;

		case VPU_CMD_CM2:
			break;

		case VPU_CMD_GAMMA:
			vpp_dump_gamma();
			break;

		case VPU_CMD_SRCIF:
			vpu_srcif_debug ( message.parameter1, message.parameter2 );
			break;

		case VPU_CMD_D2D3:
			d2d3_select ( message.parameter1 );
			break;

		default:
			break;
	}
}

void vpp_process_pq_getting ( vpu_message_t message, int *rets )
{
	//vpu_fac_pq_t* pvpu_fac_pq = &vpu_fac_pq_setting;
	vpu_picmod_table_t picTab;

	nvm_read_pic_param(nvm_read_pic_mode(),&picTab);

	switch ( message.cmd_id & 0x7f ) {
		case VPU_CMD_NATURE_LIGHT_EN:
			break;

		case VPU_CMD_BACKLIGHT_EN:
			break;

		case VPU_CMD_BRIGHTNESS:
		case VPU_CMD_BRIGHTNESS_DEF:
			//rets[0] = pvpu_fac_pq->bri_ui;
			rets[0] = picTab.bright;
			break;

		case VPU_CMD_CONTRAST:
		case VPU_CMD_CONTRAST_DEF:
			//rets[0] = pvpu_fac_pq->con_ui;
			rets[0] = picTab.contrast;
			break;

		case VPU_CMD_BACKLIGHT:
		case VPU_CMD_BACKLIGHT_DEF:
			//rets[0] = pvpu_fac_pq->backlight_ui;
			rets[0] = nvm_read_backlight_level();
			break;

		case VPU_CMD_SATURATION:
		case VPU_CMD_COLOR_DEF:
			//rets[0] = pvpu_fac_pq->satu_ui;
			rets[0] = picTab.saturation;
			break;

		case VPU_CMD_HUE_DEF:
			//rets[0] = pvpu_fac_pq->hue_ui;
			rets[0] = picTab.hue;
			break;

		case VPU_CMD_DYNAMIC_CONTRAST:
		case VPU_CMD_AUTO_LUMA_EN: /* can be reserved */
			break;

		case VPU_CMD_PICTURE_MODE:
			//rets[0] = pvpu_fac_pq->picmod;
			rets[0] = nvm_read_pic_mode();
			break;

		case VPU_CMD_PATTERN_EN:
			break;

		case VPU_CMD_PATTEN_SEL:
			//rets[0] = pvpu_fac_pq->test_pattern_mod;
			rets[0] = nvm_read_pattern_mode();
			break;

		case VPU_CMD_USER_GAMMA:
			break;

		case VPU_CMD_COLOR_TEMPERATURE_DEF:
			//rets[0] = pvpu_fac_pq->colortemp_mod;
			rets[0] = nvm_read_wb_mode();
			break;

		case CMD_HDR_KNEE_FACTOR:
			printf ( "get knee factor \n" );
			break;

		case CMD_HDR_KNEE_INTERPOLATION_MODE:
			printf ( "get knee interpolation mode \n" );
			break;

		case CMD_HDR_KNEE_SETTING:
			printf ( "get knee setting \n" );
			break;

		default:
			vpp_process_wb_getting ( message, rets );
			break;
	}
}

int temp_id;
int temp_value;
void vpp_process_pq_setting ( vpu_message_t message )
{
	//vpu_fac_pq_t* pvpu_fac_pq = &vpu_fac_pq_setting;
	vpu_picmod_e curPicMode = nvm_read_pic_mode();

	vpu_picmod_table_t picTab;
	nvm_read_pic_param(curPicMode,&picTab);

	//printf(" %s id=%d \n",__func__,message.cmd_id);
	switch ( message.cmd_id ) {
		case VPU_CMD_NATURE_LIGHT_EN:
			auto_backlight_en = message.parameter1 & 0x1;
			break;

		case VPU_CMD_BACKLIGHT_EN:
			backlight_power_ctrl ( message.parameter1 & 0x1 );
			break;

		/*case VPU_CMD_AUTO_ELEC_MODE:
		 printf("-------------------------auto save electricity mode:%d\n",message.parameter1);
		 break;*/
		case VPU_CMD_BRIGHTNESS:
		case VPU_CMD_BRIGHTNESS_DEF:
			//pvpu_fac_pq->bri_ui = message.parameter1;
			fbc_adj_bri ( message.parameter1 );
			picTab.bright = message.parameter1;
			nvm_write_pic_param(curPicMode,&picTab,0);
			break;

		case VPU_CMD_CONTRAST:
		case VPU_CMD_CONTRAST_DEF:
			//pvpu_fac_pq->con_ui = message.parameter1;
			fbc_adj_con ( message.parameter1 );
			picTab.contrast = message.parameter1;
			nvm_write_pic_param(curPicMode,&picTab,0);
			break;

		case VPU_CMD_BACKLIGHT:
		case VPU_CMD_BACKLIGHT_DEF:
			//pvpu_fac_pq->backlight_ui = message.parameter1;
			vpu_backlight_adj ( message.parameter1 & 0xff, timing_cur );
			nvm_write_backlight_level(message.parameter1,0);
			break;

		case VPU_CMD_SWITCH_5060HZ:
			uc_switch_freq = message.parameter1 & 0x01;
			break;

		case VPU_CMD_SATURATION:
		case VPU_CMD_COLOR_DEF:
			//pvpu_fac_pq->satu_ui = message.parameter1;
			fbc_adj_sat ( message.parameter1 & 0xff );
			picTab.saturation = message.parameter1;
			nvm_write_pic_param(curPicMode,&picTab,0);
			break;

		case VPU_CMD_HUE_DEF:
			//pvpu_fac_pq->hue_ui = message.parameter1;
			fbc_adj_hue ( message.parameter1 & 0xff );
			picTab.hue = message.parameter1;
			nvm_write_pic_param(curPicMode,&picTab,0);
			break;

		case VPU_CMD_DYNAMIC_CONTRAST:
		case VPU_CMD_AUTO_LUMA_EN: /* can be reserved */
			enable_dnlp ( message.parameter1 & 0x1 );
			break;

		case VPU_CMD_PICTURE_MODE:
			//pvpu_fac_pq->picmod = message.parameter1;
			curPicMode = message.parameter1;
			nvm_write_pic_mode(curPicMode,0);
			vpu_pq_picmod_adj ( ( vpu_picmod_e ) message.parameter1 & 0xff );
			picmod_cur = ( vpu_picmod_e ) message.parameter1;
			break;

		case VPU_CMD_PATTERN_EN:
			enable_patgen ( message.parameter1 & 0x1 );
			break;

		case VPU_CMD_PATTEN_SEL:
			vpu_pattern_Switch_byVs ( ( pattern_mode_t ) message.parameter1 );
			break;

		case VPU_CMD_USER_GAMMA:
			fbc_set_gamma(message.parameter1);
			break;

		case VPU_CMD_COLOR_TEMPERATURE_DEF:
			//pvpu_fac_pq->colortemp_mod = ( vpu_wb_e ) message.parameter1;
			nvm_write_wb_mode(( vpu_wb_e ) message.parameter1, 1);
			vpu_wb_t ptabWb ;
			nvm_read_wb_param(( vpu_wb_e ) message.parameter1, &ptabWb);
			vpu_colortemp_adj ( &ptabWb );
			//colortemp_cur = ( vpu_wb_e ) message.parameter1;
			break;

		case VPU_CMD_GRAY_PATTERN:
			vpu_patgen_bar_set ( message.parameter1, message.parameter2, message.parameter3 );
			vpu_color_bar_mode();
			break;

		case VPU_CMD_BURN:
			burn_mode ( message.parameter1 );
			break;

		case VPU_CMD_COLOR_SURGE:
			vpu_set_color_surge ( message.parameter1, timing_cur );
			break;

		case VPU_CMD_AVMUTE:
			//Qy debug get_avmute_flag = message.parameter1 & 0x03;
			hdmirx_fsm_to_unstable();/* set hdmi fsm to redebounce */
			/* printf("get flag-%d\n", get_avmute_flag); */
			break;

		case CMD_HDR_KNEE_FACTOR:
			knee_factor = message.parameter1;
			break;

		case CMD_HDR_KNEE_INTERPOLATION_MODE:
			knee_interpolation_mode = message.parameter1;
			break;

		case CMD_HDR_KNEE_SETTING:
			num_knee_setting = message.parameter1;
			knee_setting[num_knee_setting] = message.parameter2;
			break;

		case VPU_CMD_GAMMA_PATTERN:
			vpp_set_gamma_pattern(message.parameter1, message.parameter2, message.parameter3);
			break;

		case CMD_DNLP_PRINTK:
			#if K_DNLP_ON
			dnlp_printk = message.parameter1;
			#endif
			break;

		default:
			vpp_process_wb_setting ( message );
			break;
	}
}
void vpp_process_pq ( vpu_message_t message, int *rets )
{
	if ( message.cmd_id & VPU_CMD_READ ) {
		vpp_process_pq_getting ( message, rets );

	} else {
		vpp_process_pq_setting ( message );
	}
}
void vpp_process_top_getting ( vpu_message_t message, int *rets )
{
	switch ( message.cmd_id & 0x7f ) {
		case VPU_CMD_INIT:
		case VPU_CMD_ENABLE:
		case VPU_CMD_BYPASS:
		case VPU_CMD_OUTPUT_MUX:
			break;

		case VPU_CMD_TIMING:
			rets[0] = timing_cur;
			break;

		case VPU_CMD_SOURCE:
			rets[0] = source_cur;
			break;

		case VPU_CMD_GAMMA_MOD:
			rets[0] = gammamod_cur;
			break;

		case CMD_HDMI_STAT:
			//rets[0] = get_hdmi_stat();
			/* printf("hdmi stat = [%d]", rets[0]); */
			break;

		case CMD_HDMI_REG:
			rets[0] = ( int ) hdmirx_rd_top ( message.parameter1 );
			break;

		default:
			break;
	}
}
void vpp_process_top_setting ( vpu_message_t message )
{
	int temp;

	switch ( message.cmd_id ) {
		case VPU_CMD_INIT:
			init_display();
			init_vpp();
			break;

		case VPU_CMD_ENABLE:
			vpp_module_enable_bypass ( ( vpu_modules_t ) message.parameter1 & 0x7f, message.parameter2 );
			break;

		case VPU_CMD_BYPASS:
			break;

		case VPU_CMD_OUTPUT_MUX:
			temp = ( enum panel_interface_e ) message.parameter1;
			temp = (temp == 0) ? PANEL_IF_LVDS : PANEL_IF_VBYONE;
			if (panel_if == temp) {
				lcd_module_init();
				panel_enable();
			}

			break;

		case VPU_CMD_TIMING:
			if ( timing_cur != ( vpu_timing_t ) message.parameter1 ) {
				timing_cur = ( vpu_timing_t ) message.parameter1;
				vpu_timing_change_process();
			}

			break;

		case VPU_CMD_SOURCE:
			source_cur = ( vpu_source_t ) message.parameter1;
			break;

		case VPU_CMD_GAMMA_MOD:
			gammamod_cur = ( vpu_gammamod_t ) message.parameter1;
			config_gamma_mod ( gammamod_cur );
			break;

		case CMD_HDMI_STAT:
			dpll_ctr2 = message.parameter1;
			hdmirx_wr_top ( HDMIRX_TOP_DPLL_CTRL_2, dpll_ctr2 );
			break;

		case CMD_HDMI_REG:
			hdmirx_wr_top ( message.parameter1, message.parameter2 );
			break;

		default:
			break;
	}
}
void vpp_process_top ( vpu_message_t message, int *rets )
{
	if ( message.cmd_id & VPU_CMD_READ ) {
		vpp_process_top_getting ( message, rets );

	} else {
		vpp_process_top_setting ( message );
	}
}

void vpp_process ( vpu_message_t message, int *rets )
{
	switch ( message.cmd_id & 0x7f ) {
		/* top */
		case VPU_CMD_INIT:
		case VPU_CMD_ENABLE:
		case VPU_CMD_BYPASS:
		case VPU_CMD_OUTPUT_MUX:
		case VPU_CMD_TIMING:
		case VPU_CMD_SOURCE:
		case VPU_CMD_GAMMA_MOD:
		case CMD_HDMI_STAT:
		case CMD_HDMI_REG:
			vpp_process_top ( message, rets );
			break;

		/* PQ */
		case VPU_CMD_NATURE_LIGHT_EN:
		case VPU_CMD_BACKLIGHT_EN:
		case VPU_CMD_BRIGHTNESS:
		case VPU_CMD_CONTRAST:
		case VPU_CMD_BACKLIGHT:
		case VPU_CMD_SWITCH_5060HZ:
		case VPU_CMD_AVMUTE:
		case VPU_CMD_SATURATION:
		case VPU_CMD_DYNAMIC_CONTRAST:
		case VPU_CMD_PICTURE_MODE:
		case VPU_CMD_PATTERN_EN:
		case VPU_CMD_PATTEN_SEL:
		case VPU_CMD_USER_GAMMA:
		case VPU_CMD_COLOR_TEMPERATURE_DEF:
		case VPU_CMD_BRIGHTNESS_DEF:
		case VPU_CMD_CONTRAST_DEF:
		case VPU_CMD_COLOR_DEF:
		case VPU_CMD_HUE_DEF:
		case VPU_CMD_BACKLIGHT_DEF:
		case VPU_CMD_AUTO_LUMA_EN:

		/* case VPU_CMD_AUTO_ELEC_MODE: */
		/* wb */
		case VPU_CMD_RED_GAIN_DEF:
		case VPU_CMD_GREEN_GAIN_DEF:
		case VPU_CMD_BLUE_GAIN_DEF:
		case VPU_CMD_PRE_RED_OFFSET_DEF:
		case VPU_CMD_PRE_GREEN_OFFSET_DEF:
		case VPU_CMD_PRE_BLUE_OFFSET_DEF:
		case VPU_CMD_POST_RED_OFFSET_DEF:
		case VPU_CMD_POST_GREEN_OFFSET_DEF:
		case VPU_CMD_POST_BLUE_OFFSET_DEF:
		case VPU_CMD_WB:
		case VPU_CMD_GRAY_PATTERN:
		case VPU_CMD_BURN:
		case VPU_CMD_COLOR_SURGE:
		case CMD_HDR_KNEE_FACTOR:
		case CMD_HDR_KNEE_INTERPOLATION_MODE:
		case CMD_HDR_KNEE_SETTING:
		case VPU_CMD_GAMMA_PATTERN:
		case CMD_DNLP_PRINTK:
			vpp_process_pq ( message, rets );
			break;

		/* debug */
		case VPU_CMD_HIST:
		case VPU_CMD_BLEND:
		case VPU_CMD_DEMURA:
		case VPU_CMD_CSC:
		case VPU_CMD_CM2:
		case VPU_CMD_GAMMA:
		case VPU_CMD_SRCIF:
		case VPU_CMD_D2D3:
			vpp_process_debug ( message );
			break;

		default:
			break;
	}
}
unsigned int vpp_handle_cmd ( unsigned char *s, int *rets )
{
	vpu_message_t cmd = {0};
	int i, type, charIndex;
	unsigned int *paramter;
	cmd.cmd_id = ( fbc_command_t ) s[0];
	charIndex = 1;

	for ( i = 0; i < cmd_def[cmd.cmd_id].num_param; i++ ) {
		if ( i == 0 ) {
			paramter = &cmd.parameter1;

		} else if ( i == 1 ) {
			paramter = &cmd.parameter2;

		} else if ( i == 2 ) {
			paramter = &cmd.parameter3;

		} else {
			break;
		}

		type = ( cmd_def[cmd.cmd_id].type >> ( i * 2 ) ) & 0x03;

		switch ( type ) {
			case 1:
				*paramter = ( unsigned int ) s[charIndex];
				charIndex++;
				break;

			case 2:
				*paramter = ( ( unsigned int ) s[charIndex] & 0xff ) |
							( ( ( unsigned int ) s[charIndex + 1] & 0xff ) << 8 );
				charIndex += 2;
				break;

			case 3:
				*paramter = ( ( unsigned int ) s[charIndex] & 0xff ) |
							( ( ( unsigned int ) s[charIndex + 1] & 0xff ) << 8 ) |
							( ( ( unsigned int ) s[charIndex + 2] & 0xff ) << 16 ) |
							( ( ( unsigned int ) s[charIndex + 3] & 0xff ) << 24 );
				charIndex += 4;
				break;

			default:
				break;
		}
	}

	vpp_process ( cmd, rets );

	if ( NULL != save ) {
		save ( s );
	}

	LOGI ( TAG_VPP, "vpu_cmd_id:0x%x process done.\n", cmd.cmd_id );
	return 0;
}


int vpp_task_handle ( int task_id, void *param )
{
	list_t *plist = list_dequeue ( &vpp_cmd_list );

	if ( plist != NULL ) {
		CMD_LIST *clist = list_entry ( plist, CMD_LIST, list );

		if ( clist != NULL ) {
			unsigned char *cmd = ( unsigned char * ) ( clist->cmd_data.data );

			if ( cmd != NULL ) {
				int rcmd_num = Ret_NumParam ( cmd );

				if ( rcmd_num > 0 ) {
					void *params = ( void * ) malloc ( ( rcmd_num + 1 ) * sizeof ( int ) );
					vpp_handle_cmd ( cmd, ( int * ) params );
					SendReturn ( vpp_task_id, clist->cmd_data.cmd_owner, *cmd, ( int * ) params );
					free ( params );
					params = NULL;

				} else {
					vpp_handle_cmd ( cmd, NULL );
				}
			}

			freeCmdList ( clist );
		}
	}

	return 0;
}

int do_vpp_debug ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	unsigned int data_in, cmd_id, i, type, charIndex;
	char *cmd;
	printf ( "[vpp]%s cmd_id = %s.\n", __func__, argv[1] );

	if ( argc < 2 ) {
		return -1;
	}

	cmd = argv[1];

	vpp_debug = malloc ( 4 );
	cmd_id = strtoul ( argv[1], NULL, 10 );
	vpp_debug[0] = ( unsigned char ) cmd_id;
	charIndex = 1;

	for ( i = 0; i < cmd_def[cmd_id].num_param; i++ ) {
		data_in = strtoul ( argv[2 + i], NULL, 10 );
		type = ( cmd_def[cmd_id].type >> ( i * 2 ) ) & 0x03;

		switch ( type ) {
			case 1:
				if ( data_in > 0xff ) {
					LOGE ( TAG_VPP, "[vpp]error:parameter over flow limit one byte!!!\n" );
					return -1;
				}

				vpp_debug[charIndex] = ( unsigned char ) data_in;
				charIndex++;
				break;

			case 2:
				if ( data_in > 0xffff ) {
					LOGE ( TAG_VPP, "[vpp]error:parameter over flow limit two bytes!!!\n" );
					return -1;
				}

				vpp_debug[charIndex] = ( unsigned char ) ( data_in & 0xff );
				vpp_debug[charIndex + 1] = ( unsigned char ) ( ( data_in >> 8 ) & 0xff );
				charIndex += 2;
				break;

			case 3:
				if ( data_in > 0xffffffff ) {
					LOGE ( TAG_VPP, "[vpp]error:parameter over flow limit four bytes!!!\n" );
					return -1;
				}

				vpp_debug[charIndex] = ( unsigned char ) ( data_in & 0xFF );
				vpp_debug[charIndex + 1] = ( unsigned char ) ( ( data_in >> 8 ) & 0xFF );
				vpp_debug[charIndex + 2] = ( unsigned char ) ( ( data_in >> 16 ) & 0xFF );
				vpp_debug[charIndex + 3] = ( unsigned char ) ( ( data_in >> 24 ) & 0xFF );
				charIndex += 4;
				break;

			default:
				break;
		}
	}

	CmdChannelAddData ( INPUT_VPP_DEBUG, vpp_debug );
	return -1;
}

int vpp_check_cmd_is_supported ( int cmd )
{
	if ( cmd == ( VPU_CMD_ENABLE | VPU_CMD_READ ) ) {
		return 0;
	}

	switch ( cmd & 0x7f ) {
		/* top */
		case VPU_CMD_INIT:
		case VPU_CMD_ENABLE:
		case VPU_CMD_BYPASS:
		case VPU_CMD_OUTPUT_MUX:
		case VPU_CMD_TIMING:
		case VPU_CMD_SOURCE:
		case VPU_CMD_GAMMA_MOD:

		/* PQ */
		case VPU_CMD_NATURE_LIGHT_EN:
		case VPU_CMD_BACKLIGHT_EN:
		case VPU_CMD_BRIGHTNESS:
		case VPU_CMD_CONTRAST:
		case VPU_CMD_BACKLIGHT:
		case VPU_CMD_SWITCH_5060HZ:
		case VPU_CMD_AVMUTE:
		case VPU_CMD_SATURATION:
		case VPU_CMD_DYNAMIC_CONTRAST:
		case VPU_CMD_PICTURE_MODE:
		case VPU_CMD_PATTERN_EN:
		case VPU_CMD_PATTEN_SEL:
		case VPU_CMD_USER_GAMMA:
		case VPU_CMD_COLOR_TEMPERATURE_DEF:
		case VPU_CMD_BRIGHTNESS_DEF:
		case VPU_CMD_CONTRAST_DEF:
		case VPU_CMD_COLOR_DEF:
		case VPU_CMD_HUE_DEF:
		case VPU_CMD_BACKLIGHT_DEF:
		case VPU_CMD_AUTO_LUMA_EN:
		case VPU_CMD_GRAY_PATTERN:

		/* case VPU_CMD_AUTO_ELEC_MODE: */
		case VPU_CMD_BURN:
		case VPU_CMD_COLOR_SURGE:

		/* debug */
		case VPU_CMD_HIST:
		case VPU_CMD_BLEND:
		case VPU_CMD_DEMURA:
		case VPU_CMD_CSC:
		case VPU_CMD_CM2:
		case VPU_CMD_GAMMA:
		case VPU_CMD_SRCIF:
		case VPU_CMD_D2D3:

		/* wb */
		case VPU_CMD_RED_GAIN_DEF:
		case VPU_CMD_GREEN_GAIN_DEF:
		case VPU_CMD_BLUE_GAIN_DEF:
		case VPU_CMD_PRE_RED_OFFSET_DEF:
		case VPU_CMD_PRE_GREEN_OFFSET_DEF:
		case VPU_CMD_PRE_BLUE_OFFSET_DEF:
		case VPU_CMD_POST_RED_OFFSET_DEF:
		case VPU_CMD_POST_GREEN_OFFSET_DEF:
		case VPU_CMD_POST_BLUE_OFFSET_DEF:
		case VPU_CMD_WB:
		case CMD_HDMI_STAT:
		case CMD_HDMI_REG:
		case CMD_HDR_KNEE_FACTOR:
		case CMD_HDR_KNEE_INTERPOLATION_MODE:
		case CMD_HDR_KNEE_SETTING:
		case VPU_CMD_GAMMA_PATTERN:
		case CMD_DNLP_PRINTK:
			return 1;

		default:
			return 0;
	}
}

void register_vpp_task_mid()
{
	vpp_task_id = RegisterTask ( vpp_task_handle, NULL, 0, TASK_PRIORITY_VPP );
	RegisterCmd ( &vpp_cmd_list, vpp_task_id, INPUT_CEC | INPUT_UART_HOST | INPUT_UART_CONSOLE | INPUT_VPP_DEBUG, vpp_check_cmd_is_supported, vpp_handle_cmd );
}

int get_factory_mode(void){
	return 0;//factory_mode_enable;
}

unsigned int vpp_cmd_user_setting ( unsigned char *s )
{
	switch ( CmdID ( s ) ) {
		case VPU_CMD_NATURE_LIGHT_EN:
		case VPU_CMD_BACKLIGHT_EN:
		case VPU_CMD_BRIGHTNESS:
		case VPU_CMD_CONTRAST:
		case VPU_CMD_BACKLIGHT:
		case VPU_CMD_SATURATION:
		case VPU_CMD_DYNAMIC_CONTRAST:
		case VPU_CMD_PICTURE_MODE:
		case VPU_CMD_PATTERN_EN:
		case VPU_CMD_PATTEN_SEL:
		case VPU_CMD_USER_GAMMA:
		case VPU_CMD_COLOR_TEMPERATURE_DEF:
		case VPU_CMD_BRIGHTNESS_DEF:
		case VPU_CMD_CONTRAST_DEF:
		case VPU_CMD_COLOR_DEF:
		case VPU_CMD_HUE_DEF:
		case VPU_CMD_BACKLIGHT_DEF:
		case VPU_CMD_AUTO_LUMA_EN:

		/* case VPU_CMD_AUTO_ELEC_MODE: */
		/* wb */
		case VPU_CMD_RED_GAIN_DEF:
		case VPU_CMD_GREEN_GAIN_DEF:
		case VPU_CMD_BLUE_GAIN_DEF:
		case VPU_CMD_PRE_RED_OFFSET_DEF:
		case VPU_CMD_PRE_GREEN_OFFSET_DEF:
		case VPU_CMD_PRE_BLUE_OFFSET_DEF:
		case VPU_CMD_POST_RED_OFFSET_DEF:
		case VPU_CMD_POST_GREEN_OFFSET_DEF:
		case VPU_CMD_POST_BLUE_OFFSET_DEF:
		case VPU_CMD_WB:

		/* audio */
		case AUDIO_CMD_SET_SOURCE:
		case AUDIO_CMD_SET_MASTER_VOLUME:
		case AUDIO_CMD_SET_CHANNEL_VOLUME:
		case AUDIO_CMD_SET_SUBCHANNEL_VOLUME:
		case AUDIO_CMD_SET_MASTER_VOLUME_GAIN:
		case AUDIO_CMD_SET_CHANNEL_VOLUME_INDEX:
		case AUDIO_CMD_SET_VOLUME_BAR:
		case AUDIO_CMD_SET_MUTE:
		case AUDIO_CMD_SET_EQ_MODE:
		case AUDIO_CMD_SET_BALANCE:
			//Qy temp g_user_setting.change_flag = 1;
			break;

		case VPU_CMD_ENABLE:
			int *params = GetParams ( s );

			systems_t* psystem = &(g_nvm_user.system);

			if ( params != NULL ) {
				switch ( params[0] ) {
					case VPU_MODULE_GAMMA:
						psystem->vpp.gamma_enable = params[1];
						break;

					case VPU_MODULE_WB:
						psystem->vpp.wb_enable = params[1];
						break;

					case VPU_MODULE_CM2:
						psystem->vpp.cm2_enable = params[1];
						break;

					case VPU_MODULE_DNLP:
						psystem->vpp.dnlp_enable = params[1];
						break;

					default:
						break;
				}

				free ( params );
				params = NULL;
				//Qy temp g_user_setting.change_flag = 1;
			}

			break;

		default:
			break;
	}

	return 0;
}

unsigned char vpp_cmd_read_user_setting ( unsigned char *s, int *returns )
{
	int *params = GetParams ( s );

	if ( params == NULL ) {
		return 1;
	}

	systems_t* psystem = &(g_nvm_user.system);

	switch ( CmdID ( s ) ) {
		case ( VPU_CMD_ENABLE | VPU_CMD_READ ) :
			switch ( params[0] ) {
				case VPU_MODULE_GAMMA:
					returns[0] = psystem->vpp.gamma_enable;
					break;

				case VPU_MODULE_WB:
					returns[0] = psystem->vpp.wb_enable;
					break;

				case VPU_MODULE_CM2:
					returns[0] = psystem->vpp.cm2_enable;
					break;

				case VPU_MODULE_DNLP:
					returns[0] = psystem->vpp.dnlp_enable;
					break;

				default:
					break;
			}

			break;

		default:
			break;
	}

	free ( params );
	return 0;
}

