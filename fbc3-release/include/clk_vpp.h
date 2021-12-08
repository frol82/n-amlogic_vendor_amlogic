#ifndef CLK_VPP_H
#define CLK_VPP_H

#define K_CLOCK_MODE_ENCL_MODE1			0
#define K_CLOCK_MODE_ENCL_MODE2			1
#define K_CLOCK_MODE_ENCL_HDMI			2


#define K_USE_CLOCK_MODE				K_CLOCK_MODE_ENCL_MODE2

#define K_CLK_MEASURE_SRC_SYS_OSC				0
#define K_CLK_MEASURE_SRC_HDMI_CABLECLK			29
#define K_CLK_MEASURE_SRC_VID_PLLCLK			30
#define K_CLK_MEASURE_SRC_VPU_CTSCLK			31
#define K_CLK_MEASURE_SRC_LVDS_FIFOCLK			32
#define K_CLK_MEASURE_SRC_HDMI_TMDSCLK			41
#define K_CLK_MEASURE_SRC_II2OUT_MCLK			62


typedef enum {
	TV_ENC_LCD480x234 = 0,
	TV_ENC_LCD480x234_dsi36b = 1,       // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD240x160 = 2,
	TV_ENC_LCD240x160_dsi36b = 3,       // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD720x480 = 4,
	TV_ENC_LCD720x480_dsi36b = 5,       // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD720x576 = 6,
	TV_ENC_LCD720x576_dsi36b = 7,       // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD1280x720 = 8,
	TV_ENC_LCD1280x720_dsi36b = 9,      // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD1920x1080 = 10,
	TV_ENC_LCD1920x1080_dsi36b = 11,    // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD1920x2205 = 12,
	TV_ENC_LCD1920x2205_dsi36b = 13,    // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD2560x1600 = 14,
	TV_ENC_LCD2560x1600_dsi36b = 15,    // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD3840x2440 = 16,
	TV_ENC_LCD3840x2440_dsi36b = 17,    // For MIPI_DSI 36-bit color: sample rate=2, 1 pixel per 2 cycle
	TV_ENC_LCD3840x2160p_vic03 = 18,
	TV_ENC_LCD4096x2160p_vic04 = 19,
	TV_ENC_LCD640x480 = 20,
	TV_ENC_LCD1920x1200p = 21,
	TV_ENC_LCD240x160_dsi   = 22,
	TV_ENC_LCD240x160_slow   = 24,
	TV_ENC_LCD3840x2160p_vic01 = 23,
	TV_ENC_LCD2048x1536 = 25,
	TV_ENC_LCD1366x768 = 26,
	TV_ENC_LCD_TYPE_MAX
} tv_enc_lcd_type_t;

#define CLK_UTIL_VID_PLL_DIV_1      0
#define CLK_UTIL_VID_PLL_DIV_2      1
#define CLK_UTIL_VID_PLL_DIV_3      2
#define CLK_UTIL_VID_PLL_DIV_3p5    3
#define CLK_UTIL_VID_PLL_DIV_3p75   4
#define CLK_UTIL_VID_PLL_DIV_4      5
#define CLK_UTIL_VID_PLL_DIV_5      6
#define CLK_UTIL_VID_PLL_DIV_6      7
#define CLK_UTIL_VID_PLL_DIV_6p25   8
#define CLK_UTIL_VID_PLL_DIV_7      9
#define CLK_UTIL_VID_PLL_DIV_7p5    10
#define CLK_UTIL_VID_PLL_DIV_12     11
#define CLK_UTIL_VID_PLL_DIV_14     12
#define CLK_UTIL_VID_PLL_DIV_15     13

#define K_MSR_TIME			64

void clocks_set_vid_clk_div ( int div_sel );
void vclk_set_encl_lvds ( int vformat, int lvds_ports );
void vclk_set_encl_vx1 ( int vfromat, int lane_num, int byte_num );
void vclk_set_encl_vx1_mode2 ( int vfromat, int lane_num, int byte_num );
int set_vx1_lvds_dpll ( int freq, int od1 );
void set_crt_video_enc ( int inSel, int DivN );
//void enable_crt_vpu_clki (int enable, int inSel);
//void enable_crt_vpu_clko (int enable, int inSel);
unsigned int clk_util_clk_msr(unsigned int clk_mux, unsigned int uS_gate_time ,unsigned int* realcnt);
unsigned int clk_util_clk_msr2 ( unsigned int clk_mux, unsigned int uS_gate_time,unsigned int* realcnt);
void clock_set_srcif_ref_clk();
void vclk_set_lvds_hdmi ( int tmds_bits, int tmds_repeat, int lvds_ports );
void vclk_set_vx1_hdmi ( int tmds_bits, int tmds_repeat, int lane_num, int byte_num );
void set_crt_hdmi_div ( int inSel, int DivN );
extern void vclk_monitor_debug(void);

#endif
