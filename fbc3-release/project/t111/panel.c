#include <string.h>
#include <panel.h>
#include <user_setting.h>
#include <gpio.h>
#include <project.h>
#include <vpp_api.h>
#include <reboot.h>

#include <ui.h>
#include <task.h>
#include <board_config.h>
#include <vpu_util.h>
#include <user_setting.h>

#include <crc.h>

#ifdef ENABLE_LOCAL_DIMMING
	#include <ldim_drv.h>
#endif
#include <spi_regional_division.h>

#define UI_BRIGHTNESS_LEVEL_MAX    255
#define UI_BRIGHTNESS_LEVEL_MIN    0

panel_config_t *panel_param;
int project_id = 0;
static int backlight_level;

unsigned char customer_ptn = 0;

#ifndef IN_FBC_MAIN_CONFIG
void sleep ( int us )
{
	int n = us * ( 80 / 3 );

	while ( n-- )
		;
}
#endif

void mdelay ( int ms )
{
	sleep ( 4000 * ms );
}

void panel_power_aml ( char flag )
{
	printf("%s: %d\n", __func__, flag);
	if (flag) {
		*P_PREG_PAD_GPIO3_EN_N &= ( ~ ( 1 << 5 ) );
		*P_PREG_PAD_GPIO3_O |= ( 1 << 5 );
	} else {
		*P_PREG_PAD_GPIO3_EN_N &= ( ~ ( 1 << 5 ) );
		*P_PREG_PAD_GPIO3_O &= ( ~ ( 1 << 5 ) );
	}
}

void backlight_power_aml ( char flag )
{
	if (flag) {
		if ( strcmp ( switch_p, "PA" ) == 0 ) {
			// 112
			*P_PREG_PAD_GPIO3_EN_N &= ( ~ ( 1 << 12 ) );
			*P_PREG_PAD_GPIO3_O &= ( ~ ( 1 << 12 ) );
		} else {
			// 111
			*P_PREG_PAD_GPIO0_EN_N &= ( ~ ( 1 << 2 ) );
			*P_PREG_PAD_GPIO0_O &= ( ~ ( 1 << 2 ) );
		}
	} else {
		if ( strcmp ( switch_p, "PA" ) == 0 ) {
			// 112
			*P_PREG_PAD_GPIO3_EN_N &= ( ~ ( 1 << 12 ) );
			*P_PREG_PAD_GPIO3_O |= ( 1 << 12 );
		} else {
			// 111
			*P_PREG_PAD_GPIO0_EN_N &= ( ~ ( 1 << 2 ) );
			*P_PREG_PAD_GPIO0_O |= ( 1 << 2 );
		}
	}
}

const panel_config_t panel_aml = {
	.model_name = "basic_lvds",
	.panel_id = PANEL_ID_1080P_NORMAL,
	.interface = PANEL_IF_LVDS, .output_mode = T_1080P50HZ,
	.timing = TIMING_1920x1080P60,
	//.format = PANEL_NORMAL,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = 0, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 7, .vx1_lvds_phy_preem = 0, /* lvds */
	.clk_ss_level = 0,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 20, .panel_power_off_delay = 500,
	.signal_enable_delay = 0, .signal_disable_delay = 10,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

	.ManufactureID = { 0x05, 0xAc }, .ProductID = { 0xff, 0xff },
	.SerialNumber = { 0xff, 0xff, 0xff, 0xff }, .ManufactureDate = { 0x00, 0x19 },
	.ChipID = { 0xfb, 0x0c }, .PanelInfo = 0x03,
	.ThreeDinfo = 0x04, .SpecicalInfo = 0x05
};

const panel_config_t panel_aml_vb1_3B = {
	.model_name = "basic_vx1_3byte",
	.panel_id = PANEL_ID_4K_NORMAL_3B,
	.interface = PANEL_IF_VBYONE, .output_mode = T_2160P50HZ420,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV420,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 3, .color_fmt = 4,
	.vx1_lockn_option = 0, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1, /* vbyone */
	.clk_ss_level = 0,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 20, .panel_power_off_delay = 500,
	.signal_enable_delay = 0, .signal_disable_delay = 10,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},
};

const panel_config_t panel_aml_vb1_4B = {
	.model_name = "basic_vx1_2region",
	.panel_id = PANEL_ID_4K_NORMAL_4B,
	.interface = PANEL_IF_VBYONE, .output_mode = T_1080P50HZ44410BIT,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV420,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = -1, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1, /* vbyone */
	.clk_ss_level = 0,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 200, .panel_power_off_delay = 1000,
	.signal_enable_delay = 0, .signal_disable_delay = 10,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

	.ManufactureID = { 0x05, 0xAc }, .ProductID = { 0xff, 0xff },
	.SerialNumber = { 0xff, 0xff, 0xff, 0xff }, .ManufactureDate = { 0x00, 0x19 },
	.ChipID = { 0xfb, 0x0c }, .PanelInfo = 0x01,
	.ThreeDinfo = 0x00, .SpecicalInfo = 0x00,
};

#if 0
const panel_config_t panel_aml_vb1_4k1k_4B = {
	.model_name = "basic_vx1_4k1k",
	.panel_id = PANEL_ID_4K_SAMSUNG_LSC550FN04,
	.interface = PANEL_IF_VBYONE, .output_mode = T_2160P50HZ42010BIT,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV420,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = 0, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1, /* vbyone */
	.clk_ss_level = 0,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 20, .panel_power_off_delay = 500,
	.signal_enable_delay = 0, .signal_disable_delay = 10,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

   };
#endif

const panel_config_t panel_aml_vb1_4B_ext1 = {
	.model_name = "basic_vx1_2region",
	.panel_id = PANEL_ID_4K_SAMSUNG_LSC550FN04,
	.interface = PANEL_IF_VBYONE, .output_mode = T_2160P50HZ42210BIT,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV422,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = -1, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1, /* vbyone */
	.clk_ss_level = 1,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 200, .panel_power_off_delay = 1000, /* panel spec min is 1s */
	.signal_enable_delay = 0, .signal_disable_delay = 10,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

	.ManufactureID = { 0x05, 0xAc }, .ProductID = { 0xff, 0xff },
	.SerialNumber = { 0xff, 0xff, 0xff, 0xff }, .ManufactureDate = { 0x00, 0x19 },
	.ChipID = { 0xfb, 0x0c }, .PanelInfo = 0x01,
	.ThreeDinfo = 0x00, .SpecicalInfo = 0x00,
   };

#if 0
const panel_config_t panel_aml_vb1444_4B = {
	.model_name = "basic_vx1_444",
	.panel_id = PANEL_ID_4K_SAMSUNG_LSC550FN04,
	.interface = PANEL_IF_VBYONE, .output_mode = T_2160P50HZ444,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV444,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 2, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = -1, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1, /* vbyone */
	.clk_ss_level = 1,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 20, .panel_power_off_delay = 500,
	.signal_enable_delay = 0, .signal_disable_delay = 0,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 500, .backlight_power_off_delay = 30,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

	.ManufactureID = { 0x05, 0xAc }, .ProductID = { 0xff, 0xff },
	.SerialNumber = { 0xff, 0xff, 0xff, 0xff }, .ManufactureDate = { 0x00, 0x19 },
	.ChipID = { 0xfb, 0x0c }, .PanelInfo = 0x01,
	.ThreeDinfo = 0x00, .SpecicalInfo = 0x00,
};
#endif

const panel_config_t panel_aml_vb1_4k_Innolux = {
	.model_name = "basic_vx1_1region",
	.panel_id = PANEL_ID_4K_INNOLUX_V400DJ1,
	.interface = PANEL_IF_VBYONE, .output_mode = T_2160P50HZ42210BIT,
	.timing = TIMING_3840x2160P60,
	//.format = PANEL_YUV422,

	.reverse = 0, .scaler = 0,

	.clk = 0x63, .repack = 1, .ports = 1, .bit_size = 0,
	.odd_even = 0, .pn_swap = 0, .hv_invert = 0, .lvds_swap = 7, .clk_pin_swap = 0,

	.lane_num = 8, .region_num = 1, .byte_num = 4, .color_fmt = 4,
	.vx1_lockn_option = -1, .vx1_counter_option = 0, .vx1_hpd_wait = 1,
	.hsync_pol = 1, .vsync_pol = 1,

	.vx1_lvds_phy_vswing = 10, .vx1_lvds_phy_preem = 1,
	.clk_ss_level = 1,

	.bl_ctrl_method = 1, .bl_level_default = 128,
	.bl_pwm_port = PWM_BL_PWM0, .bl_pwm_pol = 0,
	.bl_pwm_hz = 120, .bl_pwm_duty_max = 255, .bl_pwm_duty_min = 0,
	/* local diming config */
	.bl_ldim_mode = 1,
	.bl_ldim_region_row = 1, .bl_ldim_region_col = 8,
	.bl_ldim_dev_index = 0,

	.panel_power_on_delay = 200, .panel_power_off_delay = 1000, /* panel spec min is 1s */
	.signal_enable_delay = 0, .signal_disable_delay = 150,
	.pwm_enable_delay = 0, .pwm_disable_delay = 0,
	.backlight_power_on_delay = 150, .backlight_power_off_delay = 150,
	.panel_power_ctrl = panel_power_aml,
	.backlight_power_ctrl = backlight_power_aml,

	.hdr_support = 0,
	.hdr_lumi_max = 255, .hdr_lumi_min = 0, .hdr_lumi_avg = 128,
	.hdr_primaries = {{0, 0}, {0, 0}, {0, 0}},
	.hdr_white_point = {330, 330},

	.ManufactureID = { 0x05, 0xAc }, .ProductID = { 0xff, 0xff }, .SerialNumber = { 0xff, 0xff, 0xff, 0xff }, .ManufactureDate = { 0x00, 0x19 }, .ChipID = { 0xfb, 0x0c }, .PanelInfo = 0x01,
	.ThreeDinfo = 0x00, .SpecicalInfo = 0x00,
};

#define CC_PANEL_MAX_COUNT	(35)

int get_panel_max_count ( void )
{
	return CC_PANEL_MAX_COUNT;
}

int get_panel_def_id ( void )
{
#ifdef PROJECT_ID
	printf ( "id1-%d\n", PROJECT_ID );
	return PROJECT_ID;
#else /*
	   */
	printf ( "id2%d\n" );
	return 2;
#endif /*
	*/
}

int get_panel_type(void)
{
	int output_val = get_panel_def_id();

	return output_val;
}

const panel_config_t *gPanelParams[CC_PANEL_MAX_COUNT + 1] = {

	&panel_aml,
	&panel_aml_vb1_3B,
	&panel_aml_vb1_4B,
	//&panel_aml_vb1_4k1k_4B,
	&panel_aml_vb1_4B_ext1,
	//&panel_aml_vb1444_4B,
	&panel_aml_vb1_4k_Innolux,
	NULL,
};

static char *panel_outputmode_table[] = {
	"1080p50hz", "2160p50hz420", "1080p50hz44410bit", "2160p50hz42010bit", "2160p50hz42210bit", "T_2160P50HZ42210BIT"
};

static void get_panel_by_id ( int id )
{
	int tmp_params_cnt = 0;
	int ret = 0;
#if 0
	while ( gPanelParams[tmp_params_cnt] != NULL ) {
		tmp_params_cnt += 1;
	}

	printf ( "id (%d), panel params define count = %d, max panel count = %d.\n", id, tmp_params_cnt, CC_PANEL_MAX_COUNT );

	if ( id >= 0 && id < tmp_params_cnt && id < get_panel_max_count() ) {
		panel_param = (panel_config_t*)gPanelParams[id];
		printf ( "use panel id (%d) panel param.\n", id );

	} else {
		printf ( "project id (%d) is undefined!\n", id );
		id = get_panel_def_id();
		panel_param = (panel_config_t*)gPanelParams[id];
		printf ( "use default panel id (%d) panel param.\n", id );
	}
#else
	while (gPanelParams[tmp_params_cnt] != NULL) {
		panel_param = (panel_config_t*)gPanelParams[tmp_params_cnt];
		if (panel_param->panel_id == (char)id) {
			ret = 1;
			printf("fined panel id %d , in idx %d\n",id , tmp_params_cnt);
			break;
		}
		tmp_params_cnt++;
	}

	if (ret == 0) {
		panel_param = (panel_config_t*)gPanelParams[0];
		printf("err: not fined defined panel ID %d ,max cnt =%d\n",id,tmp_params_cnt);
	}
#endif

	/* bit_mode & yuv_mode */
	switch (panel_param->output_mode) {
	case T_1080P50HZ44410BIT:
	case T_2160P50HZ42210BIT:
	case T_2160P50HZ42010BIT:
		bit10_mode = 1;
		break;
	default:
		break;
	}

	#if 0
	if (panel_param->format == PANEL_YUV420)
		hdmi_420Mode = 1;
	else
		hdmi_420Mode = 0;
	#endif

	printf("panel: %s, outputmode: %s, bit10_mode: %d, hdmi_420Mode: %d\n",
		panel_param->model_name,
		panel_outputmode_table[panel_param->output_mode],
		bit10_mode, hdmi_420Mode);
	switch (panel_param->interface) {
	case PANEL_IF_LVDS:
		printf ( "panel interface: lvds, current timing: %d\n", panel_param->timing );
		break;
	case PANEL_IF_VBYONE:
		printf ( "panel interface: vbyone, current timing: %d\n", panel_param->timing );
		printf ( "lane_num: %d, byte_num: %d, region_num: %d\n", panel_param->lane_num, panel_param->byte_num, panel_param->region_num );
		break;
	default:
		printf("panel interface: invalid\n");
		break;
	}
}

enum panel_interface_e get_panel_interface ( void )
{
	return panel_param->interface;
}

vpu_timing_t get_timing_mode ( void )
{
	return panel_param->timing;
}

int get_panel_pwm_inverter_flag ( void )
{
	return (panel_param->bl_pwm_pol ? 0 : 1);
}

/* ******************************************************* */
// panel api
/* ******************************************************* */
void backlight_power_ctrl(char flag)
{
	printf("%s: %d\n", __func__, flag);
	if (flag) { /* power on */
		switch (panel_param->bl_ctrl_method) {
		case 1:
			panel_param->backlight_power_ctrl(1);
			break;
#ifdef ENABLE_LOCAL_DIMMING
		case 2:
			ldim_power_on();
			break;
#endif
		default:
			printf("%s: invalid bl_ctrl_method: %d\n", panel_param->bl_ctrl_method);
			break;
		}

	} else { /* power off */
		switch (panel_param->bl_ctrl_method) {
		case 1:
			panel_param->backlight_power_ctrl(0);
			break;
#ifdef ENABLE_LOCAL_DIMMING
		case 2:
			ldim_power_off();
			break;
#endif
		default:
			printf("%s: invalid bl_ctrl_method: %d\n", panel_param->bl_ctrl_method);
			break;
		}
	}
}

static int backlight_level_convert(int ui_level)
{
	int max = UI_BRIGHTNESS_LEVEL_MAX;
	int min = UI_BRIGHTNESS_LEVEL_MIN;
	int level;

	level = 255 * (ui_level - min) / (max-min);
	return level;
}

void backlight_set_level(int level)
{
	vpu_timing_t timing_cur;
	timing_cur = get_timing_mode();

	printf("%s: %d\n", __func__, level);
	backlight_level = level;
	level = backlight_level_convert(level);

	switch (panel_param->bl_ctrl_method) {
	case 1:
		vpu_backlight_adj ( level, timing_cur );
		break;
#ifdef ENABLE_LOCAL_DIMMING
	case 2:
		ldim_set_level(level);
		break;
#endif
	default:
		printf("%s: invalid bl_ctrl_method: %d\n", panel_param->bl_ctrl_method);
		break;
	}
}

#if 0
int backight_get_level(void)
{
	printf("%s: %d\n", __func__, backlight_level);
	return backlight_level;
}
#endif

int panel_disable ( void )
{
	printf("%s\n", __func__);
	backlight_power_ctrl(0);
	mdelay ( panel_param->backlight_power_off_delay );
	mdelay ( panel_param->pwm_disable_delay );
	lcd_signal_ctrl(0); /* turn off panel signal */
	mdelay ( panel_param->signal_disable_delay );
	panel_param->panel_power_ctrl(0);
	mdelay ( panel_param->panel_power_off_delay );
	return 0;
}

int panel_enable ( void )
{
	printf("%s\n", __func__);
	panel_param->panel_power_ctrl(1);
	mdelay ( panel_param->panel_power_on_delay );
	lcd_signal_ctrl(1); /* turn on panel signal */
	mdelay ( panel_param->signal_enable_delay );
	mdelay ( panel_param->pwm_enable_delay );
	mdelay ( panel_param->backlight_power_on_delay );
	backlight_power_ctrl(1);
	return 0;
}
/* ******************************************************* */

unsigned int reboot_func ( void )
{
	panel_disable();

	return 0;
}

void panel_pre_load ( void )
{
	int ret = 0;
#ifdef IN_FBC_MAIN_CONFIG
	project_id = nvm_read_project_id();
#else /*
	   */
	unsigned int tmp_prj_id_addr = SPI_BASE + USER_CRI_DATA_START;
	unsigned int tmp_crc = 0, prj_id_chksum = 0;
	int panel_max_cnt = get_panel_max_count();
	project_id = * ( int * ) ( tmp_prj_id_addr );
	prj_id_chksum = * ( int * ) ( tmp_prj_id_addr + 4 );
	tmp_crc = crc32 ( 0, ( unsigned char * ) &project_id, sizeof ( unsigned int ) );

	if ( tmp_crc != prj_id_chksum ) {
		printf ( "cri data project id checksum error" );
		printf ( "(%d, 0x%08X, 0x%08X)!\n", project_id, tmp_crc, prj_id_chksum );
		project_id = get_panel_def_id();
	}

	printf ( "project id is %d, address is 0x%08x\n", project_id, tmp_prj_id_addr );
#endif /*
	*/
	/* printf("current project_id is %d\n",project_id); */
	get_panel_by_id ( project_id );
#ifdef IN_FBC_MAIN_CONFIG

	if ( UiGetHaveLogoFlag() == 1 ) {
		register_backlight_func ( backlight_power_ctrl );
	}

	registerRebootTimming ( reboot_func );
#endif /*
	*/
}
