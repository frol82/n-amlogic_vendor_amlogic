#ifndef __DATA_STRUCT__
#define __DATA_STRUCT__


#include "XYmemoryMapping.h"

#define CC_FACTORY_SN_SIZE     (20)

#define CC_CRI_REVERSED_LEN    (112)
#define CC_CRI_DEVICE_ID_LEN   (124)
#define CC_CRI_FAC_SN_LEN      (128)

typedef enum src_pq{
	ePQSrc_HDMI = 0 ,
	ePQSrc_ATV,
	ePQSrc_AV,
	ePQSrc_MPEG,
	ePQSrc_MAX,
}eSrc_pq;

typedef enum vpu_wb {	//color temperature
	eWB_COLD = 0,
	eWB_STD,
	eWB_WARM,
	eWB_USER,
	eWB_MAX,
} vpu_wb_e;

typedef enum db_wb_idx {	//color temperature
	eDB_WB_STD = 0,
	eDB_WB_WARM,
	eDB_WB_COLD,
	eDB_WB_USER,
}edb_wb_idx;

typedef enum vpu_picmod_e {	//picture mode
	PICMOD_STD = 0,		//standard
	PICMOD_BRI,		//brightness
	PICMOD_SOFT,		//soft
	PICMOD_MOVIE,		//movie
	PICMOD_USER,		//user
	PICMOD_MAX,
} vpu_picmod_e;

typedef struct misc {
	unsigned char cur_source;
	unsigned char sleep_time;
	/* 0 direct suspend, 1 direct power on */
	unsigned char power_on_mode;
	unsigned char watch_dog;
	unsigned int panel_backlight_level;
	unsigned int pattern_mode;
	unsigned int gammaIdx;
	unsigned int pqDBIdx;
	unsigned int monitor_mode;
}misc_t;


struct vpp_switch {

	/*VPU_MODULE_VPU,       //vpu uint
	 VPU_MODULE_TIMGEN,
	 VPU_MODULE_PATGEN,
	 VPU_MODULE_GAMMA,
	 VPU_MODULE_WB,      //WhiteBalance
	 VPU_MODULE_BC,      //Brightness&Contrast
	 VPU_MODULE_BCRGB,   //RGB Brightness&Contrast
	 VPU_MODULE_CM2,
	 VPU_MODULE_CSC1,
	 VPU_MODULE_DNLP,
	 VPU_MODULE_CSC0,
	 VPU_MODULE_OSD,
	 VPU_MODULE_BLEND,
	 VPU_MODULE_DEMURE,  //15
	 VPU_MODULE_OUTPUT,  //LVDS/VX1 output
	 VPU_MODULE_OSDDEC,  //OSD decoder */
	/* unsigned char vpu_enable; */
	/* unsigned char timgen_enable; */
	/* unsigned char patgen_enable; */
	unsigned char gamma_enable;

	unsigned char wb_enable;

	/* unsigned char bc_enable; */
	/* unsigned char bcrgb_enable; */
	unsigned char cm2_enable;

	/* unsigned char csc1_enable; */
	unsigned char dnlp_enable;

	unsigned char eye_protect_enable;

	unsigned char nature_light_enable;


	/* unsigned char csc0_enable; */
	/* unsigned char osd_enable; */
	/* unsigned char blend_enable; */
	/* unsigned char demura_enable; */
	/* unsigned char output_enable; */
	/* unsigned char osddec_enable; */
};
#if 0
typedef struct white_balance_setting {
	unsigned char r_gain;
	unsigned char g_gain;
	unsigned char b_gain;
	unsigned char r_offset;
	unsigned char g_offset;
	unsigned char b_offset;
} white_balance_setting_t;

typedef struct white_balance_setting_ext {
	unsigned short r_gain;
	unsigned short g_gain;
	unsigned short b_gain;
	unsigned short r_offset;
	unsigned short g_offset;
	unsigned short b_offset;
} white_balance_setting_ext_t;
#endif

typedef struct vpu_picmod_table_s {
	vpu_picmod_e picmod;
	unsigned int bright;
	unsigned int contrast;
	unsigned int saturation;
	unsigned int hue;
} vpu_picmod_table_t;


typedef struct systems {
	unsigned project_id; /* must be 1st member */
	unsigned ver1;
	unsigned ver2;
	unsigned db_ver;
	char device_id[CC_CRI_DEVICE_ID_LEN];
	char factory_sn[CC_FACTORY_SN_SIZE];
	misc_t misc;
	struct vpp_switch vpp;
}systems_t;

typedef struct vpu_colortemp_table_s {
	vpu_wb_e color_temp;
	vpu_wb_t wb_param;
} vpu_colortemp_table_t;

#if 0
typedef struct vpu_fac_pq_s { //for factory menu
	unsigned short bri_ui;//0~255
	unsigned short con_ui;//0~255
	unsigned short satu_ui;//0~255
	unsigned short hue_ui;//0~255
	unsigned short backlight_ui;//0~255
	vpu_wb_t colortemp_user;//0~255
	vpu_wb_e colortemp_mod;
	vpu_picmod_e picmod;
	unsigned short test_pattern_mod;
} vpu_fac_pq_t;
#endif

typedef struct user_pq { //for user menu
	vpu_picmod_e pic_mode[ePQSrc_MAX];
	vpu_wb_e wb_mode[ePQSrc_MAX];
	vpu_picmod_table_t picmode[ePQSrc_MAX][PICMOD_MAX];
}user_pq_t;

typedef struct fac_pq { //for fac menu
	vpu_wb_t wb[eWB_MAX];
}fac_pq_t;
#if 0
typedef struct wb_setting {

	unsigned adjusted;

	int p_gamma_index;

	int wb_data_flag;

	vpu_colortemp_table_t cold;

	vpu_colortemp_table_t warm;

	vpu_colortemp_table_t standard;

	vpu_colortemp_table_t user;

}wb_setting_t;
#endif

struct cri_prj_id {

	unsigned int prj_id_ori;

	unsigned int prj_id_chksum;

	unsigned char reversed[8];
};

struct cri_rev {
	unsigned char reversed[CC_CRI_REVERSED_LEN];
};

struct cri_dev_id {

	unsigned int dev_id_len;

	unsigned int dev_id_l_chksum;

	unsigned int dev_id_b_chksum;

	unsigned char dev_id_buf[CC_CRI_DEVICE_ID_LEN];

	unsigned char reversed[8];
};

struct cri_fac_sn {
	unsigned int fac_sn_len;

	unsigned int fac_sn_l_chksum;

	unsigned int fac_sn_b_chksum;

	unsigned char fac_sn_buf[CC_CRI_FAC_SN_LEN];

	unsigned char reversed[4];
};

typedef struct cri_data {
	struct cri_prj_id prj_id;
	struct cri_rev rev;
	struct cri_dev_id dev_id;
	struct cri_fac_sn fac_sn;
}cri_data_t;

typedef struct nvm_user{
	systems_t 		system;
	//cri_data_t 		cri;
	audio_control_t audio;
	user_pq_t		pquser;
}nvm_user_t;

typedef struct factory_data{
	unsigned int db_ver_factory;
	unsigned int factory_data_flag;
	fac_pq_t	pqfac;
}factory_data_t;


#endif

