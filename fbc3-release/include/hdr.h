#ifndef HDR_H
#define HDR_H
#include <cmd.h>
#include <panel.h>
#include <vpp_api.h>

#define u8	unsigned char
#define u16	unsigned short
#define u32	unsigned int
typedef unsigned long long u64;
typedef int	int32_t;
typedef	long long int64_t;

#define false 0
#define true 1

#if 0
#define XVYCC_LUT_R_ADDR_PORT	0x315e
#define XVYCC_LUT_R_DATA_PORT	0x315f
#define XVYCC_LUT_G_ADDR_PORT	0x3160
#define XVYCC_LUT_G_DATA_PORT	0x3161
#define XVYCC_LUT_B_ADDR_PORT	0x3162
#define XVYCC_LUT_B_DATA_PORT	0x3163
//#define XVYCC_INV_LUT_CTL		0x3164
#define XVYCC_LUT_CTL			0x3165
#endif


typedef enum {
	eHDR_MODE_HDR = 0,
	eHDR_MODE_SDR = 1,
	eHDR_MODE_AUTO = 2,
	eHDR_MODE_OFF = 3,
}eHDR_MODE;

typedef enum {
	eHDR_CHG_MD_HDR2HDR = 0,
	eHDR_CHG_MD_HDR2SDR = 1,
	eHDR_CHG_MD_SDR2HDR = 2,
}eHDR_PRO_MODE;


/* struct ve_dnlp_s          video_ve_dnlp; */

//#define FLAG_RSV31              (1 << 31)
//#define FLAG_RSV30              (1 << 30)
//#define FLAG_VE_DNLP            (1 << 29)
//#define FLAG_VE_NEW_DNLP        (1 << 28)
//#define FLAG_RSV27              (1 << 27)
//#define FLAG_RSV26              (1 << 26)
//#define FLAG_3D_BLACK_DIS       (1 << 25)
//#define FLAG_3D_BLACK_EN        (1 << 24)
//#define FLAG_3D_SYNC_DIS        (1 << 23)
//#define FLAG_3D_SYNC_EN         (1 << 22)
//#define FLAG_VLOCK_DIS          (1 << 21)
//#define FLAG_VLOCK_EN          (1 << 20)
//#define FLAG_VE_DNLP_EN         (1 << 19)
//#define FLAG_VE_DNLP_DIS        (1 << 18)
#define FLAG_VADJ1_CON			(1 << 17)
//#define FLAG_VADJ1_BRI			(1 << 16)
//#define FLAG_GAMMA_TABLE_EN     (1 << 15)
//#define FLAG_GAMMA_TABLE_DIS    (1 << 14)
//#define FLAG_GAMMA_TABLE_R      (1 << 13)
//#define FLAG_GAMMA_TABLE_G      (1 << 12)
//#define FLAG_GAMMA_TABLE_B      (1 << 11)
//#define FLAG_RGB_OGO            (1 << 10)
//#define FLAG_RSV9               (1 <<  9)
#define FLAG_MATRIX_UPDATE      (1 <<  8)
//#define FLAG_BRI_CON            (1 <<  7)
//#define FLAG_LVDS_FREQ_SW       (1 <<  6)
//#define FLAG_REG_MAP5           (1 <<  5)
//#define FLAG_REG_MAP4           (1 <<  4)
//#define FLAG_REG_MAP3           (1 <<  3)
//#define FLAG_REG_MAP2           (1 <<  2)
//#define FLAG_REG_MAP1           (1 <<  1)
//#define FLAG_REG_MAP0           (1 <<  0)

#define CSC_ON              1
#define CSC_OFF             0


struct matrix_s {
	u16 pre_offset[3];
	u16 matrix[3][3];
	u16 offset[3];
	u16 right_shift;
};

struct vframe_master_display_colour_s {
	u32 present_flag;
	u32 primaries[3][2];
	u32 white_point[2];
	u32 luminance[2];
}; /* master_display_colour_info_volume from SEI */

/* vframe properties */
struct vframe_prop_s {
	struct vframe_master_display_colour_s
		master_display_colour;
} /*vframe_prop_t */;

struct vframe_fbc {
	u32 signal_type;
	/* vframe properties */
	struct vframe_prop_s prop;

} ;


/* master_display_info for display device */
struct master_display_info_s {
	u32 present_flag;
	u32 features;			/* feature bits bt2020/2084 */
	u32 primaries[3][2];		/* normalized 50000 in G,B,R order */
	u32 white_point[2];		/* normalized 50000 */
	u32 luminance[2];		/* max/min lumin, normalized 10000 */
};


struct hdr_info {
	u32 hdr_support; /* RX EDID hdr support types */
	u32 lumi_max; /* RX EDID Lumi Max value */
	u32 lumi_avg; /* RX EDID Lumi Avg value */
	u32 lumi_min; /* RX EDID Lumi Min value */
};

struct vinfo_s {
	//enum tvin_color_fmt_e viu_color_fmt;
	struct hdr_info hdr_info;
	struct master_display_info_s
		master_display_info;
};

enum vpp_matrix_csc_e {
	VPP_MATRIX_NULL = 0,
	VPP_MATRIX_RGB_YUV601 = 0x1,
	VPP_MATRIX_RGB_YUV709 = 0x11,
	VPP_MATRIX_YUV709_RGB = 0x12,
	VPP_MATRIX_YUV709F_RGB = 0x13,
	VPP_MATRIX_YUV709_YUV709F = 0x14,
	VPP_MATRIX_RGB_YUV2020 = 0x20,
	VPP_MATRIX_YUV2020_RGB709_STM = 0X21,
	VPP_MATRIX_YUV2020_RGB709_STD = 0X22,
	VPP_MATRIX_YUV2020_RGB2020 = 0X23,
	VPP_MATRIX_BT2020RGB_CUSRGB = 0x50,
};

enum vpp_matrix_sel_e {
	VPP_MATRIX_CSC0,/*builtin_pattern*/
	VPP_MATRIX_CSC1,/*input_signal*/
};

void amvecm_matrix_process(void);
void hdr_get_info(void);
void vpp_set_matrix(enum vpp_matrix_sel_e csc0_or_csc1,
		unsigned int on,enum vpp_matrix_csc_e csc_mode,struct matrix_s *m);
void hdr_init(void);
void hdr_force_csc1(int csc1type);

#endif
