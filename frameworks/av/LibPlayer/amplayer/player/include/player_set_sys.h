/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef _PLAYER_SET_DISP_H_
#define _PLAYER_SET_DISP_H_

#define MID_800_400_FREESCALE (0x10001) //mid 800*400

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    DISP_MODE_480I  = 1,
    DISP_MODE_480P  = 2,
    DISP_MODE_576I  = 3,
    DISP_MODE_576P  = 4,
    DISP_MODE_720P  = 5,
    DISP_MODE_1080I = 6,
    DISP_MODE_1080P = 7
}
display_mode;

typedef struct {
    display_mode disp_mode;
    int osd_disble_coordinate[8];
    int osd_enable_coordinate[8];
    int video_enablecoordinate[4];
    int fb0_freescale_width;
    int fb0_freescale_height;
    int fb1_freescale_width;
    int fb1_freescale_height;
} freescale_setting_t;



int set_sysfs_str(const char *path, const char *val);
int  get_sysfs_str(const char *path, char *valstr, int size);
int set_sysfs_int(const char *path, int val);
int get_sysfs_int(const char *path);
int get_vf_next_pts();

int set_black_policy(int blackout);
int get_black_policy();
int set_tsync_enable(int enable);
int get_tsync_enable(void);
int set_tsync_discontinue(int enable);
int get_pts_discontinue();
int set_fb0_blank(int blank);
int set_fb1_blank(int blank);
int set_subtitle_num(int num);
int av_get_subtitle_curr();
int set_subtitle_startpts(int pts);
int set_subtitle_fps(int fps);
int set_subtitle_subtype(int subtype);
void get_display_mode(char *mode);
void get_vfm_map_info(char *vfm_map);
int set_fb0_freescale(int freescale);
int set_fb1_freescale(int freescale);
int set_display_axis(int *coordinate);
int set_video_axis(int *coordinate);
int set_fb0_scale_width(int width);
int set_fb0_scale_height(int height);
int set_fb1_scale_width(int width);
int set_fb1_scale_height(int height);
int check_audiodsp_fatal_err(void);
int set_stb_source_hiu(void);
int set_stb_demux_source_hiu(void);

int set_subtitle_enable(int num);
int set_subtitle_curr(int num);
int check_file_same(char *filename2);
int set_auto_refresh_rate(int enable);
int get_auto_refresh_rate();
int reset_auto_refresh_rate();
//player sysfs API channel
int set_amutils_enable(int isOn);
int set_amutils_cmd(const char* cmd);
int get_amutils_cmd(char* cmd);
int check_audio_output();
int set_poweron_clock_level(int level);

int set_subtitle_index(int num);
int get_audio_digital_output_mode();

#ifdef  __cplusplus
}
#endif

#endif
