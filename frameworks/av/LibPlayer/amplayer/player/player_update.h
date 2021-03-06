/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef _PLAYER_UPDATE_H_
#define _PLAYER_UPDATE_H_

#include <player_type.h>
#include "player_priv.h"

unsigned int get_pts_pcrscr(play_para_t *p_para);
unsigned int get_pts_video(play_para_t *p_para);
unsigned int get_pts_audio(play_para_t *p_para);
int     update_playing_info(play_para_t *p_para);
int     update_player_cachetime(play_para_t *p_para, struct buf_status *vbuf, struct buf_status *abuf);
int     set_media_info(play_para_t *p_para);
int     check_time_interrupt(long *old_msecond, int interval_ms);
int set_ps_subtitle_info(play_para_t *p_para, subtitle_info_t *sub_info, int sub_num);
void set_drm_rental(play_para_t *p_para, unsigned int rental_value);
int check_audio_ready_time(int64_t *first_time);
long player_get_systemtime_ms(void);
void check_avdiff_status(play_para_t *p_para);
int check_to_retry(play_para_t *p_para);
int64_t getstreambuffer_buffedsize(play_para_t *p_para);
int64_t getlpbuffer_buffedsize(play_para_t *p_para);
int  audio_digital_mode_change(play_para_t *p_para);
int force_buffering_enter(play_para_t *p_para);

#endif

