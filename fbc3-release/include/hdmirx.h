#ifndef HDMIRX_H
#define HDMIRX_H

// Device ID to differentiate HDMIRX register access to TOP, DWC or PHY
#define HDMIRX_DEV_ID_TOP       0
#define HDMIRX_DEV_ID_DWC       1
#define HDMIRX_DEV_ID_PHY       2

#define HDMIRX_ADDR_PORT        0x80034400  // TOP ADDR_PORT: 0xc800e000; DWC ADDR_PORT: 0xc800e010
#define HDMIRX_DATA_PORT        0x80034404  // TOP DATA_PORT: 0xc800e004; DWC DATA_PORT: 0xc800e014
#define HDMIRX_CTRL_PORT        0x80034408  // TOP CTRL_PORT: 0xc800e008; DWC CTRL_PORT: 0xc800e018

#define INT_HDMIRX_BASE_OFFSET  0x00000000  // INT TOP ADDR_PORT: 0xc800e000; INT DWC ADDR_PORT: 0xc800e010
#define EXT_HDMIRX_BASE_OFFSET  0x00000020  // EXT TOP ADDR_PORT: 0xc800e020; EXT DWC ADDR_PORT: 0xc800e030

//#define ENABLE_CEC_FUNCTION


#define HDMI_COLOR_DEPTH_24B            4
#define HDMI_COLOR_DEPTH_30B            5
#define HDMI_COLOR_DEPTH_36B            6
#define HDMI_COLOR_DEPTH_48B            7

#define HDMI_COLOR_FORMAT_RGB           0
#define HDMI_COLOR_FORMAT_422           1
#define HDMI_COLOR_FORMAT_444           2
#define HDMI_COLOR_FORMAT_420           3

#define HDMI_COLOR_RANGE_LIM            0
#define HDMI_COLOR_RANGE_FUL            1

#define HDMI_AUDIO_PACKET_SMP           0x02
#define HDMI_AUDIO_PACKET_1BT           0x07
#define HDMI_AUDIO_PACKET_DST           0x08
#define HDMI_AUDIO_PACKET_HBR           0x09

#define HDMI_HPD_LOW		0
#define HDMI_HPD_HIGH		1
#define HDMI_HPD_INPUT		2

#define PANEL_OUTMD_LVDS	0
#define PANEL_OUTMD_VBO	1

enum tvin_hdr_eotf_e {
	EOTF_SDR,
	EOTF_HDR,
	EOTF_SMPTE_ST_2048,
	EOTF_MAX,
};

enum tvin_hdr_state_e {
	HDR_STATE_OLD,
	HDR_STATE_READ,
	HDR_STATE_NEW,
};

struct tvin_hdr_property_s {
	unsigned int x;/* max */
	unsigned int y;/* min */
};

struct tvin_hdr_data_s {
	enum tvin_hdr_eotf_e eotf:8;
	unsigned char metadata_id;
	unsigned char lenght;
	enum tvin_hdr_state_e data_status:8;
	struct tvin_hdr_property_s primaries[3];
	struct tvin_hdr_property_s white_points;
	struct tvin_hdr_property_s master_lum;/* max min lum */
	unsigned int mcll;
	unsigned int mfall;
};

extern unsigned int    dpll_ctr2;
extern unsigned int    dpll_ctr[6];

//extern int backporch_unstable;
extern int frontporch_unstable;
extern int hsync_pixel_unstable;
extern int active_pixel_unstable;
extern int sof_line_unstable;
extern int eof_line_unstable;
extern int vsync_line_unstable;
extern int active_ine_unstable;
extern int front_end_alignment_stability;
extern int hsAct;
extern int vsAct;
extern int deActivity;
extern int ilace;
extern int htot32Clk;
extern int hsClk;
extern int hactPix;
extern int vtotClk;
extern int vsClk;
extern int vactLin;
extern int vtotLin;
extern int vofsLin;
extern int enable_10bit;
extern int clk_divider;
extern int enable_10bit_input;
extern int pll_rst_counter;

extern int hdmirx_fsm_pause;
extern int hdmirx_int_pro_pause;

// Use the following functions to access the HDMIRX modules (TOP, DWC or PHY) by default
//extern void             hdmirx_wr_only_reg  (unsigned char dev_id, unsigned long addr, unsigned long data);
//extern unsigned long    hdmirx_rd_reg       (unsigned char dev_id, unsigned long addr);
//extern void             hdmirx_rd_check_reg (unsigned char dev_id, unsigned long addr, unsigned long exp_data, unsigned long mask);
//extern void             hdmirx_wr_reg       (unsigned char dev_id, unsigned long addr, unsigned long data);
//extern void             hdmirx_poll_reg     (unsigned char dev_id, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void hdmirx_fsm_init(void);
extern void hdmirx_fsm_to_unstable(void);
extern void 			hdmirx_init ( void );
extern void 			hdmirx_sig_monitor ( void );
extern void 			hdmirx_cec_monitor ( void );
extern void 			hdmi_rx_get_cec_cmd ( void );
extern void				hdmirx_register_channel ( void );
// HDMIRX repeater configuration
//extern void             hdmirx_repeater_config (unsigned long bstatus, unsigned short *ksv);

//extern void             aocec_poll_reg_busy (unsigned char reg_busy);
extern void             aocec_wr_only_reg ( unsigned long addr, unsigned long data );
extern unsigned long    aocec_rd_reg ( unsigned long addr, unsigned int check_busy_high );
extern void             aocec_rd_check_reg ( unsigned long addr, unsigned long exp_data, unsigned long mask, unsigned int check_busy_high );
extern void             aocec_wr_reg ( unsigned long addr, unsigned long data );
extern void             aocec_wr_reg_fast ( unsigned long addr, unsigned long data );
extern void				hdmirx_set_DPLL ( int lvl );
extern void 			hdmirx_DPLL_reset ( void );
extern void 			hdmirx_audio_fifo_rst ( void );
extern void 			hdmirx_CDR_init ( void );
extern void hdmirx_cdr_reset ( void );
extern void 			HDMIRX_IRQ_Handle ( void *arg );
extern void 			hdmirx_config ( void );
extern void 			hdmirx_phy_init ( void );
#ifdef ENABLE_AVMUTE_CONTROL
//extern void 			hdmi_avmute ( int en );
extern int avmute_count;
#endif
// extern void hdmirx_avmute ( int en );
// Internal functions:
void            hdmirx_wr_only_top  ( unsigned long addr, unsigned long data );
void            hdmirx_wr_only_dwc  ( unsigned long addr, unsigned long data );
//void            hdmirx_wr_only_phy  (unsigned long addr, unsigned long data);
unsigned long   hdmirx_rd_top       ( unsigned long addr );
unsigned long   hdmirx_rd_dwc       ( unsigned long addr );
//unsigned long   hdmirx_rd_phy       (unsigned long addr);
//void            hdmirx_rd_check_top (unsigned long addr, unsigned long exp_data, unsigned long mask);
void            hdmirx_rd_check_dwc ( unsigned long addr, unsigned long exp_data, unsigned long mask );
//void            hdmirx_rd_check_phy (unsigned long addr, unsigned long exp_data, unsigned long mask);
void            hdmirx_wr_top       ( unsigned long addr, unsigned long data );
void            hdmirx_wr_dwc       ( unsigned long addr, unsigned long data );
//void            hdmirx_wr_phy       (unsigned long addr, unsigned long data);
//void            hdmirx_poll_top     (unsigned long addr, unsigned long exp_data, unsigned long mask);
//void            hdmirx_poll_dwc     (unsigned long addr, unsigned long exp_data, unsigned long mask, unsigned long max_try);
//void            hdmirx_poll_phy     (unsigned long addr, unsigned long exp_data, unsigned long mask);
void            hdmirx_key_setting  (   unsigned char           encrypt_en );

int 			hdmi_rx_phy_pll_lock ( void );
int 			hdmi_rx_audio_pll_lock ( void );
extern unsigned int hdmirx_get_clock ( int index );
void 			hdmirx_phy_init ( void );
void 			hdmirx_audio_control ( char enable );
extern void hdmirx_phy_init ( void );
int drm_handler(void);
void hdmirx_get_hdr_property(struct tvin_hdr_data_s *hdr_data);
int hdmirx_get_hdr_attach_cnt(void);
extern void hdmi_rx_hpd(int HighLow);
extern char io_hdmi_5v_pw ( void );
extern void hdmirx_int_get_status(void);
extern void hdmirx_int_clr_status(void);

//------------------------------------------------------------------------------
// Defines for simulation
//------------------------------------------------------------------------------

typedef enum TimingID{
	K_ID_NOTSUPPORT = 0,
	K_ID_NOTSIGNAL,
	K_ID_1080P50,
	K_ID_1080P60,
	K_ID_3840x2160P50,
	K_ID_3840x2160P60,
	K_ID_3840x2160P50420,
	K_ID_3840x2160P60420,
	K_ID_3840x2160P50420_1,
	K_ID_3840x2160P60420_1,
	K_ID_END,
}eTimingID;


typedef struct timingWind{
	int HValid;
	int VValid;
	int HTotal;
	int VTotal;
}stTimingWind;

typedef struct timing{
	eTimingID ID;
	stTimingWind window;
}stTiming;

#define diffRange(a,b)	(( a > b ) ? ( a - b ) : ( b - a ))

#endif /* HDMIRX_H */

