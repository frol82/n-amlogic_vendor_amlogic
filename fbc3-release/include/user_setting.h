#ifndef USER_SETTING_H
#define USER_SETTING_H

#include <vpp_api.h>
#include <data_struct.h>
#include <cmd.h>


#define HDCP_KEY_PARA 0x5A

#define K_MAX_DB_SIZE			20
#define K_MAX_GAMMA_SIZE		9

extern struct cri_data gCriData;
extern nvm_user_t g_nvm_user;

extern int setting_task_handle ( int task_id, void *param );
extern int check_cmd_is_supported ( int cmd );


unsigned nvm_read_project_id(void);
int nvm_write_project_id ( unsigned val );
char *nvm_read_device_id();
int nvm_write_device_id ( char *device_id );
char *nvm_read_factory_sn();
int nvm_write_factory_sn ( char *pSn );
//unsigned char nvm_read_power_on_mode();
//int nvm_write_power_on_mode ( unsigned char mode );
char *nvm_read_hdcpkey();
int nvm_write_hdcpkey ( const char *key, int len );

//int nvm_write_wb_setting ( vpu_wb_e mode, vpu_wb_t *val );
//vpu_wb_t *nvm_read_wb_setting ( vpu_wb_e mode,vpu_wb_t *val );
//int nvm_write_picmode ( vpu_picmod_t mode, vpu_picmod_table_t *val );
//vpu_picmod_table_t *nvm_read_picmode ( vpu_picmod_t mode );

int nvm_switch_project_id ( unsigned int id );

int load_default_setting();

int nvm_read_light_enable();
void nvm_write_nature_lihgt_en ( unsigned int on_off );

extern void nvm_init_task ( void );

extern int clr_default_wb_setting ( void );

//int nvm_write_gamma_idx ( int temp_value );

extern int nvm_read_backlight_level(void);

extern void nvm_write_backlight_level(int level,int save);

extern int nvm_read_cursource(void);

extern int nvm_read_pic_mode(void);

extern int nvm_write_pic_mode(int cur_pic_mode,int save);

extern int nvm_read_pic_param(vpu_picmod_e cur_pic_mode,vpu_picmod_table_t* tab);

extern int nvm_write_pic_param(vpu_picmod_e cur_pic_mode,vpu_picmod_table_t* tab,int save);

extern int nvm_read_wb_mode(void);

extern int nvm_write_wb_mode(int cur_wb_mode, int save);

extern int nvm_read_wb_param(int cur_wb_mode,vpu_wb_t *ptabWb);

extern int nvm_write_wb_param(int cur_wb_mode,vpu_wb_t *ptabWb, int save);

extern int nvm_read_pattern_mode(void);

extern void nvm_write_pattern_mode(int ptm_md,int save);

extern void nvm_read_gammatab(int idx,int dbIdx);

extern int nvm_read_gamma_idx(void);

extern int nvm_write_gamma_idx(int idx,int save);

extern int nvm_read_pqDB_idx(void);

extern int nvm_write_pqDB_idx(int idx,int save);

extern int nvm_read_eye_protect(void);

extern int nvm_write_eye_protect(int cur_eye_flag,int save);

//return 0:on 1:off
extern int nvm_read_monitor_mode(void);

extern int nvm_write_monitor_mode(int OnOff,int save);

#endif
