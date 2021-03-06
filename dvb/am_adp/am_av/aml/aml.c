#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief AMLogic 音视频解码驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-09: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
#define _LARGEFILE64_SOURCE

#include <am_debug.h>
#include <am_misc.h>
#include <am_mem.h>
#include <am_evt.h>
#include <am_time.h>
#include "am_dmx.h"
#include <am_thread.h>
#include "../am_av_internal.h"
#include "../../am_aout/am_aout_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <amports/amstream.h>
#include <cutils/properties.h>
#include "am_ad.h"

#ifdef ANDROID
#include <cutils/properties.h>
#include <sys/system_properties.h>
#endif


#ifdef CHIP_8226H
#include <linux/jpegdec.h>
#endif
#if defined(CHIP_8226M) || defined(CHIP_8626X)
#include <linux/amports/jpegdec.h>
#endif

//#define PLAYER_API_NEW
#define ADEC_API_NEW

#include "player.h"
#define PLAYER_INFO_POP_INTERVAL 500
#define FILENAME_LENGTH_MAX 2048

#include <codec_type.h>
#include <adec-external-ctrl.h>

void *adec_handle = NULL;

#ifndef TRICKMODE_NONE
#define TRICKMODE_NONE      0x00
#define TRICKMODE_I         0x01
#define TRICKMODE_FFFB      0x02
#define TRICK_STAT_DONE     0x01
#define TRICK_STAT_WAIT     0x00
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AOUT_DEV_NO 0

#define ENABLE_AUDIO_RESAMPLE
#if 0
#define ENABLE_DROP_BFRAME
#endif
//#define ENABLE_BYPASS_DI
#define ENABLE_PCR

#define ADEC_START_AUDIO_LEVEL       1024
#define ADEC_START_VIDEO_LEVEL       2048
#define ADEC_FORCE_START_AUDIO_LEVEL 2048
#define DEC_STOP_AUDIO_LEVEL         16
#define DEC_STOP_VIDEO_LEVEL         512
#define UP_RESAMPLE_AUDIO_LEVEL      128
#define UP_RESAMPLE_VIDEO_LEVEL      1024
#define DOWN_RESAMPLE_CACHE_TIME     90000*2
#define NO_DATA_CHECK_TIME           2000*2
#define VMASTER_REPLAY_TIME          4000
#define SCRAMBLE_CHECK_TIME          1000
#define TIMESHIFT_INJECT_DIFF_TIME	 4
#define TIMESHIFT_FFFB_ERROR_CNT	 5
#define VIDEO_AVAILABLE_MIN_CNT     2

#ifdef ENABLE_PCR
#ifndef AMSTREAM_IOC_PCRID
#define AMSTREAM_IOC_PCRID        _IOW(AMSTREAM_IOC_MAGIC, 0x4f, int)
#endif
#endif

#define STREAM_VBUF_FILE    "/dev/amstream_vbuf"
#define STREAM_ABUF_FILE    "/dev/amstream_abuf"
#define STREAM_TS_FILE      "/dev/amstream_mpts"
#define STREAM_PS_FILE      "/dev/amstream_mpps"
#define STREAM_RM_FILE      "/dev/amstream_rm"
#define JPEG_DEC_FILE       "/dev/amjpegdec"
#define AMVIDEO_FILE        "/dev/amvideo"

#define PPMGR_FILE			"/dev/ppmgr"

#define VID_AXIS_FILE       "/sys/class/video/axis"
#define VID_CONTRAST_FILE   "/sys/class/video/contrast"
#define VID_SATURATION_FILE "/sys/class/video/saturation"
#define VID_BRIGHTNESS_FILE "/sys/class/video/brightness"
#define VID_DISABLE_FILE    "/sys/class/video/disable_video"
#define VID_BLACKOUT_FILE   "/sys/class/video/blackout_policy"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"
#define VID_ASPECT_RATIO_FILE "/sys/class/video/aspect_ratio"
#define VID_ASPECT_MATCH_FILE "/sys/class/video/matchmethod"

#if !defined (AMLOGIC_LIBPLAYER)
#define  AUDIO_CTRL_DEVICE    "/dev/amaudio_ctl"
#define AMAUDIO_IOC_MAGIC  'A'
#define AMAUDIO_IOC_SET_LEFT_MONO               _IOW(AMAUDIO_IOC_MAGIC, 0x0e, int)
#define AMAUDIO_IOC_SET_RIGHT_MONO              _IOW(AMAUDIO_IOC_MAGIC, 0x0f, int)
#define AMAUDIO_IOC_SET_STEREO                  _IOW(AMAUDIO_IOC_MAGIC, 0x10, int)
#define AMAUDIO_IOC_SET_CHANNEL_SWAP            _IOW(AMAUDIO_IOC_MAGIC, 0x11, int)
#endif

#define VDEC_H264_ERROR_RECOVERY_MODE_FILE "/sys/module/amvdec_h264/parameters/error_recovery_mode"
#define VDEC_H264_FATAL_ERROR_RESET_FILE "/sys/module/amvdec_h264/parameters/fatal_error_reset"
#define DISP_MODE_FILE      "/sys/class/display/mode"
#define ASTREAM_FORMAT_FILE "/sys/class/astream/format"
#define VIDEO_DROP_BFRAME_FILE  "/sys/module/amvdec_h264/parameters/enable_toggle_drop_B_frame"
#define DI_BYPASS_FILE    "/sys/module/di/parameters/bypass_post"
#define TSYNC_MODE_FILE   "/sys/class/tsync/mode"
#define ENABLE_RESAMPLE_FILE    "/sys/class/amaudio/enable_resample"
#define RESAMPLE_TYPE_FILE      "/sys/class/amaudio/resample_type"
#define AUDIO_DMX_PTS_FILE	"/sys/class/stb/audio_pts"
#define VIDEO_DMX_PTS_FILE	"/sys/class/stb/video_pts"
#define AUDIO_PTS_FILE	"/sys/class/tsync/pts_audio"
#define VIDEO_PTS_FILE	"/sys/class/tsync/pts_video"
#define TSYNC_MODE_FILE "/sys/class/tsync/mode"
#define AV_THRESHOLD_MIN_FILE "/sys/class/tsync/av_threshold_min"
#define AV_THRESHOLD_MAX_FILE "/sys/class/tsync/av_threshold_max"
#define AVS_PLUS_DECT_FILE "/sys/module/amvdec_avs/parameters/profile"
#define DEC_CONTROL_H264 "/sys/module/amvdec_h264/parameters/dec_control"
#define DEC_CONTROL_MPEG12 "/sys/module/amvdec_mpeg12/parameters/dec_control"
#define VIDEO_NEW_FRAME_COUNT_FILE "/sys/module/amvideo/parameters/new_frame_count"

#define AUDIO_DSP_DIGITAL_RAW_FILE "/sys/class/audiodsp/digital_raw"

#define DEC_CONTROL_PROP "media.dec_control"
#define AC3_AMASTER_PROP "media.ac3_amaster"

#define CANVAS_ALIGN(x)    (((x)+7)&~7)
#define JPEG_WRTIE_UNIT    (32*1024)
#define AUDIO_START_LEN (0*1024)
#define AUDIO_LOW_LEN (1*1024)
#define AV_SYNC_THRESHOLD	60
#define AV_SMOOTH_SYNC_VAL "100"

#ifdef ANDROID
#define open(a...)\
	({\
	 int ret, times=300;\
	 do{\
	 	ret = open(a);\
	 	if(ret==-1)\
	 		usleep(10000);\
	 }while(ret==-1 && times--);\
	 ret;\
	 })
#endif

#define PPMGR_IOC_MAGIC         'P'
#define PPMGR_IOC_2OSD0         _IOW(PPMGR_IOC_MAGIC, 0x00, unsigned int)
#define PPMGR_IOC_ENABLE_PP     _IOW(PPMGR_IOC_MAGIC, 0X01, unsigned int)
#define PPMGR_IOC_CONFIG_FRAME  _IOW(PPMGR_IOC_MAGIC, 0X02, unsigned int)
#define PPMGR_IOC_VIEW_MODE     _IOW(PPMGR_IOC_MAGIC, 0X03, unsigned int)

#define MODE_3D_DISABLE         0x00000000
#define MODE_3D_ENABLE          0x00000001
#define MODE_AUTO               0x00000002
#define MODE_2D_TO_3D           0x00000004
#define MODE_LR                 0x00000008
#define MODE_BT                 0x00000010
#define MODE_LR_SWITCH          0x00000020
#define MODE_FIELD_DEPTH        0x00000040
#define MODE_3D_TO_2D_L         0x00000080
#define MODE_3D_TO_2D_R         0x00000100
#define LR_FORMAT_INDICATOR     0x00000200
#define BT_FORMAT_INDICATOR     0x00000400

#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)

static int _get_vid_disabled();
static int _get_aud_disabled();

#define VALID_VIDEO(_pid_, _fmt_) (!_get_vid_disabled() && VALID_PID(_pid_))
#define VALID_AUDIO(_pid_, _fmt_) (!_get_aud_disabled() && VALID_PID(_pid_) && audio_get_format_supported(_fmt_))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 音视频数据注入*/
typedef struct
{
	int             fd;
	int             video_fd;
	pthread_t       thread;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	const uint8_t  *data;
	int             len;
	int             times;
	int             pos;
	int             dev_no;
	AM_Bool_t       is_audio;
	AM_Bool_t       running;
	void           *adec;
} AV_DataSource_t;

/**\brief JPEG解码相关数据*/
typedef struct
{
	int             vbuf_fd;
	int             dec_fd;
} AV_JPEGData_t;

/**\brief JPEG解码器状态*/
typedef enum {
	AV_JPEG_DEC_STAT_DECDEV,
	AV_JPEG_DEC_STAT_INFOCONFIG,
	AV_JPEG_DEC_STAT_INFO,
	AV_JPEG_DEC_STAT_DECCONFIG,
	AV_JPEG_DEC_STAT_RUN
} AV_JPEGDecState_t;

#ifdef PLAYER_API_NEW
/**\brief 文件播放器数据*/
typedef struct
{
	int                media_id;
	play_control_t pctl;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	AM_AV_Device_t    *av;
} AV_FilePlayerData_t;
#endif

/**\brief 数据注入播放模式相关数据*/
typedef struct
{
	AM_AV_PFormat_t    pkg_fmt;
	int                aud_fd;
	int                vid_fd;
	int                aud_id;
	int                vid_id;
	int                cntl_fd;
	void              *adec;
	AM_AD_Handle_t ad;
} AV_InjectData_t;

/**\brief Timeshift播放状态*/
typedef enum
{
	AV_TIMESHIFT_STAT_STOP,
	AV_TIMESHIFT_STAT_PLAY,
	AV_TIMESHIFT_STAT_PAUSE,
	AV_TIMESHIFT_STAT_FFFB,
	AV_TIMESHIFT_STAT_EXIT,
	AV_TIMESHIFT_STAT_INITOK,
	AV_TIMESHIFT_STAT_SEARCHOK,
} AV_TimeshiftState_t;

typedef struct AV_TimeshiftSubfile
{
	int rfd;
	int wfd;
	int findex;
	loff_t size;

	struct AV_TimeshiftSubfile *next;
}AV_TimeshiftSubfile_t;

/**\brief Timeshift文件*/
typedef struct
{
	int		opened;	/**< 是否已打开*/
	loff_t	size;	/**< 文件长度*/
	loff_t	avail;	/**< 当前可读数据长度*/
	loff_t	total;	/**< 当前已写入的数据长度*/
	loff_t	start;	/**< 文件起始位置*/
	loff_t	read;	/**< 当前读位置*/
	loff_t	write;	/**< 当前写位置*/
	pthread_mutex_t lock;	/*读写锁*/
	pthread_cond_t		cond;
	AM_Bool_t	loop;	/*是否使用循环文件*/
	AM_Bool_t	is_timeshift;
	char	*name;	/*文件名称*/

	/* sub files control */
	int last_sub_index;
	loff_t sub_file_size;
	AV_TimeshiftSubfile_t *sub_files;
	AV_TimeshiftSubfile_t *cur_rsub_file;
	AV_TimeshiftSubfile_t *cur_wsub_file;
} AV_TimeshiftFile_t;

/**\brief Timeshift播放模式相关数据*/
typedef struct
{
	int					av_fd;
	int					speed;
	int					running;
	int					rate;	/**< 码率 bytes/s*/
	int					duration;	/**< 时移时长，固定值*/
	int					dmxfd;
	int					cntl_fd;
	int					fffb_time;
	int					fffb_base;
	int					left;
	int					inject_size;
	int					timeout;
	int					seek_pos;
	loff_t				rtotal;
	int					rtime;
	int					aud_pid;
	int					aud_fmt;
	int					aud_idx;
	AM_Bool_t				aud_valid;
	AV_PlayCmd_t		cmd;
	AV_PlayCmd_t		last_cmd;
	pthread_t			thread;
	pthread_mutex_t		lock;
	pthread_cond_t		cond;
	AV_TimeshiftState_t	state;
	AV_TimeshiftFile_t	file;
	AV_TimeShiftPlayPara_t para;
	AM_AV_Device_t       *dev;
	AM_AV_TimeshiftInfo_t	info;
	char 				last_stb_src[16];
	char 				last_dmx_src[16];
	void               *adec;
	AM_AD_Handle_t      ad;
} AV_TimeshiftData_t;

/**\brief TS player 相关数据*/
typedef struct
{
	int fd;
	int vid_fd;
	void *adec;
	AM_AD_Handle_t ad;
} AV_TSData_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

/*音视频设备操作*/
static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para);
static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para);
static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src);
static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
static AM_ErrorCode_t aml_inject_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status);
static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info);
static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val);
static AM_ErrorCode_t aml_inject(AM_AV_Device_t *dev, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout);
static AM_ErrorCode_t aml_video_frame(AM_AV_Device_t *dev, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s);
static AM_ErrorCode_t aml_get_astatus(AM_AV_Device_t *dev, AM_AV_AudioStatus_t *para);
static AM_ErrorCode_t aml_get_vstatus(AM_AV_Device_t *dev, AM_AV_VideoStatus_t *para);
static AM_ErrorCode_t aml_timeshift_fill_data(AM_AV_Device_t *dev, uint8_t *data, int size);
static AM_ErrorCode_t aml_timeshift_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para);
static AM_ErrorCode_t aml_timeshift_get_info(AM_AV_Device_t *dev, AM_AV_TimeshiftInfo_t *info);
static AM_ErrorCode_t aml_set_vpath(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_switch_ts_audio_fmt(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_switch_ts_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt);
static AM_ErrorCode_t aml_reset_audio_decoder(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_set_drm_mode(AM_AV_Device_t *dev, int enable);
static AM_ErrorCode_t aml_set_audio_ad(AM_AV_Device_t *dev, int enable, uint16_t apid, AM_AV_AFormat_t afmt);

const AM_AV_Driver_t aml_av_drv =
{
.open        = aml_open,
.close       = aml_close,
.open_mode   = aml_open_mode,
.start_mode  = aml_start_mode,
.close_mode  = aml_close_mode,
.ts_source   = aml_ts_source,
.file_cmd    = aml_file_cmd,
.inject_cmd  = aml_inject_cmd,
.file_status = aml_file_status,
.file_info   = aml_file_info,
.set_video_para = aml_set_video_para,
.inject      = aml_inject,
.video_frame = aml_video_frame,
.get_audio_status = aml_get_astatus,
.get_video_status = aml_get_vstatus,
.timeshift_fill = aml_timeshift_fill_data,
.timeshift_cmd = aml_timeshift_cmd,
.get_timeshift_info = aml_timeshift_get_info,
.set_vpath   = aml_set_vpath,
.switch_ts_audio = aml_switch_ts_audio,
.reset_audio_decoder = aml_reset_audio_decoder,
.set_drm_mode = aml_set_drm_mode,
.set_audio_ad = aml_set_audio_ad,
};

/*音频控制（通过解码器）操作*/
static AM_ErrorCode_t adec_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_close(AM_AOUT_Device_t *dev);
static AM_ErrorCode_t adec_set_pre_gain(AM_AOUT_Device_t *dev, float gain);
static AM_ErrorCode_t adec_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);

const AM_AOUT_Driver_t adec_aout_drv =
{
.open         = adec_open,
.set_volume   = adec_set_volume,
.set_mute     = adec_set_mute,
.set_output_mode = adec_set_output_mode,
.close        = adec_close,
.set_pre_gain = adec_set_pre_gain,
.set_pre_mute = adec_set_pre_mute,
};

/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev);

const AM_AOUT_Driver_t amplayer_aout_drv =
{
.open         = amp_open,
.set_volume   = amp_set_volume,
.set_mute     = amp_set_mute,
.set_output_mode = amp_set_output_mode,
.close        = amp_close
};

/*监控AV buffer, PTS 操作*/
static pthread_mutex_t gAVMonLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gAVMonCond = PTHREAD_COND_INITIALIZER;
static AM_Bool_t	gAVPcrEnable = AM_FALSE;

static void* aml_av_monitor_thread(void *arg);
static AM_ErrorCode_t aml_get_pts(const char *class_file,  uint32_t *pts);

/*Timeshift 操作*/
static void *aml_timeshift_thread(void *arg);

static AV_TimeshiftData_t *m_tshift = NULL;


static AM_ErrorCode_t aml_set_ad_source(AM_AD_Handle_t *ad, int enable, int pid, int fmt, void *user);

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_Bool_t show_first_frame_nosync(void)
{
	char buf[32];

	if (AM_FileRead("/sys/class/video/show_first_frame_nosync", buf, sizeof(buf)) >= 0) {
		int v = atoi(buf);

		return (v == 1) ? AM_TRUE : AM_FALSE;
	}

	return AM_FALSE;
}

static int get_amstream(AM_AV_Device_t *dev)
{
	if (dev->mode & AV_PLAY_TS)
	{
		if(dev->ts_player.drv_data)
			return ((AV_TSData_t *)dev->ts_player.drv_data)->fd;
		else
			return -1;
	}
	else if (dev->mode & AV_INJECT)
	{
		AV_InjectData_t *inj = (AV_InjectData_t *)dev->inject_player.drv_data;
		if (inj->vid_fd != -1)
			return inj->vid_fd;
		else
			return inj->aud_fd;
	}
	else if (dev->mode & AV_PLAY_VIDEO_ES)
	{
		AV_DataSource_t *src = (AV_DataSource_t *)dev->vid_player.drv_data;
		return src->fd;
	}
	else if (dev->mode & AV_PLAY_AUDIO_ES)
	{
		AV_DataSource_t *src = (AV_DataSource_t *)dev->aud_player.drv_data;
		return src->fd;
	}
	else if (dev->mode & (AV_GET_JPEG_INFO | AV_DECODE_JPEG))
	{
		AV_JPEGData_t *jpeg = (AV_JPEGData_t *)dev->vid_player.drv_data;
		return jpeg->vbuf_fd;
	}

	return -1;
}


#define AUD_ASSO_PROP "media.audio.enable_asso"
#define AUD_ASSO_MIX_PROP "media.audio.mix_asso"
#define VID_DISABLED_PROP "media.dvb.video.disabled"
#define AUD_DISABLED_PROP "media.dvb.audio.disabled"
static int _get_prop_int(char *prop, int def) {
	char v[32];
	int val = 0;
	property_get(prop, v, "0");
	if (sscanf(v, "%d", &val) != 1)
		val = def;
	return val;
}
static int _get_asso_enable() {
	return _get_prop_int(AUD_ASSO_PROP, 0);
}
static int _get_asso_mix() {
	return _get_prop_int(AUD_ASSO_MIX_PROP, 50);
}
static int _get_vid_disabled() {
	return _get_prop_int(VID_DISABLED_PROP, 0);
}
static int _get_aud_disabled() {
	return _get_prop_int(AUD_DISABLED_PROP, 0);
}

static void adec_start_decode(int fd, int fmt, int has_video, void **padec)
{
#if !defined(ADEC_API_NEW)
		adec_cmd("start");
#else
		if (padec) {
			arm_audio_info param;
			memset(&param, 0, sizeof(param));
			param.handle = fd;
			param.format = fmt;
			param.has_video = has_video;
			param.associate_dec_supported = _get_asso_enable();
			param.mixing_level = _get_asso_mix();
			audio_decode_init(padec, &param);
			audio_set_av_sync_threshold(*padec, AV_SYNC_THRESHOLD);
			audio_decode_set_volume(*padec, 1.);
		}
#endif
		AM_AOUT_SetDriver(AOUT_DEV_NO, &adec_aout_drv, *padec);
}

static void adec_stop_decode(void **padec)
{
#if !defined(ADEC_API_NEW)
		adec_cmd("stop");
#else
		if (padec && *padec) {
			AM_AOUT_SetDriver(AOUT_DEV_NO, NULL, NULL);
			audio_decode_stop(*padec);
			audio_decode_release(padec);
			*padec = NULL;
		}
#endif
}

static void adec_set_decode_ad(int enable, int pid, int fmt, void *adec)
{
#if !defined(ADEC_API_NEW)
#else
	UNUSED(pid);
	UNUSED(fmt);

	if (adec)
		audio_set_associate_enable(adec, enable);
#endif
}

/*音频控制（通过解码器）操作*/
static AM_ErrorCode_t adec_cmd(const char *cmd)
{
#if !defined(ADEC_API_NEW)
	AM_ErrorCode_t ret;
	char buf[32];
	int fd;

	ret = AM_LocalConnect("/tmp/amadec_socket", &fd);
	if (ret != AM_SUCCESS)
		return ret;

	ret = AM_LocalSendCmd(fd, cmd);

	if (ret == AM_SUCCESS)
	{
		ret = AM_LocalGetResp(fd, buf, sizeof(buf));
	}

	close(fd);

	return ret;
#else
	UNUSED(cmd);
	return 0;
#endif
}

static AM_ErrorCode_t adec_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	UNUSED(dev);
	UNUSED(para);
	return AM_SUCCESS;
}

static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol)
{
#ifndef ADEC_API_NEW
	char buf[32];

	snprintf(buf, sizeof(buf), "volset:%d", vol);

	return adec_cmd(buf);
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = audio_decode_set_volume(vol);
#else
	ret = audio_decode_set_volume(dev->drv_data, ((float)vol)/100);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifndef ADEC_API_NEW
	const char *cmd = mute?"mute":"unmute";

	return adec_cmd(cmd);
#else
	int ret=0;

	UNUSED(dev);
	AM_DEBUG(1, "set_mute %d\n", mute?1:0);
	ret = audio_decode_set_mute(dev->drv_data, mute?1:0);

	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
#ifndef ADEC_API_NEW


	switch (mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			cmd = "stereo";
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			cmd = "leftmono";
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			cmd = "rightmono";
		break;
		case AM_AOUT_OUTPUT_SWAP:
			cmd = "swap";
		break;
	}

	return adec_cmd(cmd);
#else
	int ret=0;

	UNUSED(dev);

	switch(mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			ret=audio_channel_stereo(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			ret=audio_channel_left_mono(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			ret=audio_channel_right_mono(dev->drv_data);
		break;
		case AM_AOUT_OUTPUT_SWAP:
			ret=audio_channels_swap(dev->drv_data);
		break;
	}

	if(ret==-1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t adec_close(AM_AOUT_Device_t *dev)
{
	UNUSED(dev);
	return AM_SUCCESS;
}

static AM_ErrorCode_t adec_set_pre_gain(AM_AOUT_Device_t *dev, float gain)
{
#ifndef ADEC_API_NEW
	return AM_FAILURE;
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = -1;
#else
	AM_DEBUG(1, "set_pre_gain %f\n", gain);
	ret = audio_decode_set_pre_gain(dev->drv_data, gain);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t adec_set_pre_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifndef ADEC_API_NEW
	return AM_FAILURE;
#else
	int ret=0;

	UNUSED(dev);

#ifdef CHIP_8626X
	ret = -1;
#else
	AM_DEBUG(1, "set_pre_mute %d\n", mute?1:0);
	ret = audio_decode_set_pre_mute(dev->drv_data, mute?1:0);
#endif
	if (ret == -1)
		return AM_FAILURE;
	else
		return AM_SUCCESS;
#endif
}



/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	UNUSED(dev);
	UNUSED(para);
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;

	if (audio_set_volume(media_id, vol) == -1)
	{
		AM_DEBUG(1, "audio_set_volume failed");
		return AM_AV_ERR_SYS;
	}
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;

	AM_DEBUG(1, "audio_set_mute %d\n", mute);
	if (audio_set_mute(media_id, mute?0:1) == -1)
	{
		AM_DEBUG(1, "audio_set_mute failed");
		return AM_AV_ERR_SYS;
	}
#endif
	return AM_SUCCESS;
}

#if !defined(AMLOGIC_LIBPLAYER)
static int audio_hardware_ctrl( AM_AOUT_OutputMode_t mode)
{
    int fd;

    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        AM_DEBUG(1,"Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (mode) {
    case AM_AOUT_OUTPUT_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case AM_AOUT_OUTPUT_DUAL_LEFT:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case AM_AOUT_OUTPUT_DUAL_RIGHT:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case AM_AOUT_OUTPUT_STEREO:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        AM_DEBUG(1,"Unknow mode %d!", mode);
        break;

    };

    close(fd);

    return 0;
}
#endif

static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
#ifdef PLAYER_API_NEW
	int media_id = (long)dev->drv_data;
	int ret=0;

	switch (mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
#ifdef AMLOGIC_LIBPLAYER
                	ret = audio_stereo(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_left_mono(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_right_mono(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
		case AM_AOUT_OUTPUT_SWAP:
#ifdef AMLOGIC_LIBPLAYER
			ret = audio_swap_left_right(media_id);
#else
                        ret = audio_hardware_ctrl(mode);
#endif
		break;
	}
	if (ret == -1)
	{
		AM_DEBUG(1, "audio_set_mode failed");
		return AM_AV_ERR_SYS;
	}
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev)
{
	UNUSED(dev);
	return AM_SUCCESS;
}

/**\brief 音视频数据注入线程*/
static void* aml_data_source_thread(void *arg)
{
	AV_DataSource_t *src = (AV_DataSource_t*)arg;
	struct pollfd pfd;
	struct timespec ts;
	int cnt;

	while (src->running)
	{
		pthread_mutex_lock(&src->lock);

		if (src->pos  >= 0)
		{
			pfd.fd     = src->fd;
			pfd.events = POLLOUT;

			if (poll(&pfd, 1, 20) == 1)
			{
				cnt = src->len-src->pos;
				cnt = write(src->fd, src->data+src->pos, cnt);
				if (cnt <= 0)
				{
					AM_DEBUG(1, "write data failed");
				}
				else
				{
					src->pos += cnt;
				}
			}

			if (src->pos == src->len)
			{
				if (src->times > 0)
				{
					src->times--;
					if (src->times)
					{
						src->pos = 0;
						AM_EVT_Signal(src->dev_no, src->is_audio?AM_AV_EVT_AUDIO_ES_END:AM_AV_EVT_VIDEO_ES_END, NULL);
					}
					else
					{
						src->pos = -1;
					}
				}
			}

			AM_TIME_GetTimeSpecTimeout(20, &ts);
			pthread_cond_timedwait(&src->cond, &src->lock, &ts);
		}
		else
		{
			pthread_cond_wait(&src->cond, &src->lock);
		}

		pthread_mutex_unlock(&src->lock);

	}

	return NULL;
}

/**\brief 创建一个音视频数据注入数据*/
static AV_DataSource_t* aml_create_data_source(const char *fname, int dev_no, AM_Bool_t is_audio)
{
	AV_DataSource_t *src;

	src = (AV_DataSource_t*)malloc(sizeof(AV_DataSource_t));
	if (!src)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}

	memset(src, 0, sizeof(AV_DataSource_t));

	src->fd = open(fname, O_RDWR);
	if (src->fd == -1)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", fname);
		free(src);
		return NULL;
	}

	src->dev_no   = dev_no;
	src->is_audio = is_audio;

	pthread_mutex_init(&src->lock, NULL);
	pthread_cond_init(&src->cond, NULL);

	return src;
}

/**\brief 运行数据注入线程*/
static AM_ErrorCode_t aml_start_data_source(AV_DataSource_t *src, const uint8_t *data, int len, int times)
{
	int ret;

	if (!src->running)
	{
		src->running = AM_TRUE;
		src->data    = data;
		src->len     = len;
		src->times   = times;
		src->pos     = 0;

		ret = pthread_create(&src->thread, NULL, aml_data_source_thread, src);
		if (ret)
		{
			AM_DEBUG(1, "create the thread failed");
			src->running = AM_FALSE;
			return AM_AV_ERR_SYS;
		}
	}
	else
	{
		pthread_mutex_lock(&src->lock);

		src->data  = data;
		src->len   = len;
		src->times = times;
		src->pos   = 0;

		pthread_mutex_unlock(&src->lock);
		pthread_cond_signal(&src->cond);
	}

	return AM_SUCCESS;
}

/**\brief 释放数据注入数据*/
static void aml_destroy_data_source(AV_DataSource_t *src)
{
	if (src->running)
	{
		src->running = AM_FALSE;
		pthread_cond_signal(&src->cond);
		pthread_join(src->thread, NULL);
	}

	close(src->fd);
	pthread_mutex_destroy(&src->lock);
	pthread_cond_destroy(&src->cond);
	free(src);
}

#ifdef PLAYER_API_NEW
/**\brief 释放文件播放数据*/
static void aml_destroy_fp(AV_FilePlayerData_t *fp)
{
	/*等待播放器停止*/
	if (fp->media_id >= 0)
	{
		player_exit(fp->media_id);
	}

	pthread_mutex_destroy(&fp->lock);
	pthread_cond_destroy(&fp->cond);

	free(fp);
}

/**\brief 创建文件播放器数据*/
static AV_FilePlayerData_t* aml_create_fp(AM_AV_Device_t *dev)
{
	AV_FilePlayerData_t *fp;

	fp = (AV_FilePlayerData_t*)malloc(sizeof(AV_FilePlayerData_t));
	if (!fp)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}

	if (player_init() < 0)
	{
		AM_DEBUG(1, "player_init failed");
		free(fp);
		return NULL;
	}

	memset(fp, 0, sizeof(AV_FilePlayerData_t));
	pthread_mutex_init(&fp->lock, NULL);
	pthread_cond_init(&fp->cond, NULL);

	fp->av       = dev;
	fp->media_id = -1;

	return fp;
}

static int aml_update_player_info_callback(int pid,player_info_t * info)
{
	UNUSED(pid);
	if (info)
		AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)info);

	return 0;
}
#endif

int aml_set_tsync_enable(int enable)
{
	int fd;
	char *path = "/sys/class/tsync/enable";
	char  bcmd[16];

	snprintf(bcmd,sizeof(bcmd),"%d",enable);
	
	return AM_FileEcho(path, bcmd);

	// fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	// if(fd>=0)
	// {
	// 	snprintf(bcmd,sizeof(bcmd),"%d",enable);
	// 	write(fd,bcmd,strlen(bcmd));
	// 	close(fd);
	// 	return 0;
	// }

	// return -1;
}

/**\brief 初始化JPEG解码器*/
static AM_ErrorCode_t aml_init_jpeg(AV_JPEGData_t *jpeg)
{
	if (jpeg->dec_fd != -1)
	{
		close(jpeg->dec_fd);
		jpeg->dec_fd = -1;
	}

	if (jpeg->vbuf_fd != -1)
	{
		close(jpeg->vbuf_fd);
		jpeg->vbuf_fd = -1;
	}

	jpeg->vbuf_fd = open(STREAM_VBUF_FILE, O_RDWR|O_NONBLOCK);
	if (jpeg->vbuf_fd == -1)
	{
		AM_DEBUG(1, "cannot open amstream_vbuf");
		goto error;
	}

	if (ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_VFORMAT, VFORMAT_JPEG) == -1)
	{
		AM_DEBUG(1, "set jpeg video format failed (\"%s\")", strerror(errno));
		goto error;
	}

	if (ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_PORT_INIT) == -1)
	{
		AM_DEBUG(1, "amstream init failed (\"%s\")", strerror(errno));
		goto error;
	}

	return AM_SUCCESS;
error:
	if (jpeg->dec_fd != -1)
	{
		close(jpeg->dec_fd);
		jpeg->dec_fd = -1;
	}
	if (jpeg->vbuf_fd != -1)
	{
		close(jpeg->vbuf_fd);
		jpeg->vbuf_fd = -1;
	}
	return AM_AV_ERR_SYS;
}

/**\brief 创建JPEG解码相关数据*/
static AV_JPEGData_t* aml_create_jpeg_data(void)
{
	AV_JPEGData_t *jpeg;

	jpeg = malloc(sizeof(AV_JPEGData_t));
	if (!jpeg)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}

	jpeg->vbuf_fd = -1;
	jpeg->dec_fd  = -1;

	if (aml_init_jpeg(jpeg) != AM_SUCCESS)
	{
		free(jpeg);
		return NULL;
	}

	return jpeg;
}

/**\brief 释放JPEG解码相关数据*/
static void aml_destroy_jpeg_data(AV_JPEGData_t *jpeg)
{
	if (jpeg->dec_fd != -1)
		close(jpeg->dec_fd);
	if (jpeg->vbuf_fd != -1)
		close(jpeg->vbuf_fd);

	free(jpeg);
}

/**\brief 创建数据注入相关数据*/
static AV_InjectData_t* aml_create_inject_data(void)
{
	AV_InjectData_t *inj;

	inj = (AV_InjectData_t *)malloc(sizeof(AV_InjectData_t));
	if (!inj)
		return NULL;

	inj->aud_fd = -1;
	inj->vid_fd = -1;
	inj->aud_id = -1;
	inj->vid_id = -1;
	inj->cntl_fd = -1;
	inj->adec = NULL;

	return inj;
}

/**\brief 设置数据注入参赛*/
static AM_ErrorCode_t aml_start_inject(AV_InjectData_t *inj, AV_InjectPlayPara_t *inj_para)
{
	int vfd=-1, afd=-1;
	AM_AV_InjectPara_t *para = &inj_para->para;
	AM_Bool_t has_video = VALID_VIDEO(para->vid_id, para->vid_fmt);
	AM_Bool_t has_audio = VALID_AUDIO(para->aud_id, para->aud_fmt);

	inj->pkg_fmt = para->pkg_fmt;

	switch (para->pkg_fmt)
	{
		case PFORMAT_ES:
			if (has_video)
			{
				vfd = open(STREAM_VBUF_FILE, O_RDWR);
				if (vfd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_VBUF_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
				inj->vid_fd = vfd;
			}
			if (has_audio)
			{
				afd = open(STREAM_ABUF_FILE, O_RDWR);
				if (afd == -1)
				{
					AM_DEBUG(1, "cannot open device \"%s\"", STREAM_ABUF_FILE);
					return AM_AV_ERR_CANNOT_OPEN_FILE;
				}
				inj->aud_fd = afd;
			}
		break;
		case PFORMAT_PS:
			vfd = open(STREAM_PS_FILE, O_RDWR);
			if (vfd == -1)
			{
				AM_DEBUG(1, "cannot open device \"%s\"", STREAM_PS_FILE);
				return AM_AV_ERR_CANNOT_OPEN_FILE;
			}
			inj->vid_fd = afd = vfd;
		break;
		case PFORMAT_TS:
			vfd = open(STREAM_TS_FILE, O_RDWR);
			if (vfd == -1)
			{
				AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_FILE);
				return AM_AV_ERR_CANNOT_OPEN_FILE;
			}
			inj->vid_fd = afd = vfd;
		break;
		case PFORMAT_REAL:
			vfd = open(STREAM_RM_FILE, O_RDWR);
			if (vfd == -1)
			{
				AM_DEBUG(1, "cannot open device \"%s\"", STREAM_RM_FILE);
				return AM_AV_ERR_CANNOT_OPEN_FILE;
			}
			inj->vid_fd = afd = vfd;
		break;
		default:
			AM_DEBUG(1, "unknown package format %d", para->pkg_fmt);
		return AM_AV_ERR_NOT_SUPPORTED;
	}

	if (has_video)
	{
		dec_sysinfo_t am_sysinfo;

		if (ioctl(vfd, AMSTREAM_IOC_VFORMAT, para->vid_fmt) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(vfd, AMSTREAM_IOC_VID, para->vid_id) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}

		memset(&am_sysinfo,0,sizeof(dec_sysinfo_t));
		if (para->vid_fmt == VFORMAT_VC1)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (para->vid_fmt == VFORMAT_H264)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (para->vid_fmt == VFORMAT_AVS)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_AVS;
			/*am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;*/
		}
		else if (para->vid_fmt == VFORMAT_HEVC)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			am_sysinfo.width  = 3840;
			am_sysinfo.height = 2160;
		}

		if (ioctl(vfd, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo) == -1)
		{
			AM_DEBUG(1, "set AMSTREAM_IOC_SYSINFO");
			return AM_AV_ERR_SYS;
		}

	}

	if (has_audio)
	{
		if (ioctl(afd, AMSTREAM_IOC_AFORMAT, para->aud_fmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(afd, AMSTREAM_IOC_AID, para->aud_id) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
		if ((para->aud_fmt == AFORMAT_PCM_S16LE) || (para->aud_fmt == AFORMAT_PCM_S16BE) ||
				(para->aud_fmt == AFORMAT_PCM_U8)) {
			ioctl(afd, AMSTREAM_IOC_ACHANNEL, para->channel);
			ioctl(afd, AMSTREAM_IOC_SAMPLERATE, para->sample_rate);
			ioctl(afd, AMSTREAM_IOC_DATAWIDTH, para->data_width);
		}
	}

	if (vfd != -1)
	{
		if (ioctl(vfd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
		{
			AM_DEBUG(1, "amport init failed");
			return AM_AV_ERR_SYS;
		}

		inj->cntl_fd = open(AMVIDEO_FILE, O_RDWR);
		if (inj->cntl_fd == -1)
		{
			AM_DEBUG(1, "cannot open \"%s\"", AMVIDEO_FILE);
			return AM_AV_ERR_CANNOT_OPEN_FILE;
		}
		ioctl(inj->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
	}

	if ((afd != -1) && (afd != vfd))
	{
		if (ioctl(afd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
		{
			AM_DEBUG(1, "amport init failed");
			return AM_AV_ERR_SYS;
		}
	}

	if (has_audio)
	{
		adec_start_decode(afd, para->aud_fmt, has_video, &inj->adec);
	}

	inj->aud_id = para->aud_id;
	inj->vid_id = para->vid_id;

	return AM_SUCCESS;
}

/**\brief 释放数据注入相关数据*/
static void aml_destroy_inject_data(AV_InjectData_t *inj)
{
	if (inj->aud_id != -1) {
		aml_set_ad_source(&inj->ad, 0, 0, 0, inj->adec);
		adec_set_decode_ad(0, 0, 0, inj->adec);
		adec_stop_decode(&inj->adec);
	}
	if (inj->aud_fd != -1)
		close(inj->aud_fd);
	if (inj->vid_fd != -1)
		close(inj->vid_fd);
	if (inj->cntl_fd != -1) {
		ioctl(inj->cntl_fd, AMSTREAM_IOC_VPAUSE, 0);
		close(inj->cntl_fd);
	}
	free(inj);
}
void aml_term_signal_handler(int signo)
{
	AM_DEBUG(1,"PVR_DEBUG recive a signal:%d", signo);
	if (signo == SIGTERM)
	{
		AM_DEBUG(1,"PVR_DEBUG recive SIGTERM");
		AV_TimeshiftFile_t *tfile = &m_tshift->file;
		struct stat tfile_stat;

		if (tfile) {
			if (tfile->opened == 1)
			{
				AM_DEBUG(1, "PVR_DEBUG in timeshift mode, fd:%d ,fname:%s", tfile->sub_files->rfd, tfile->name);
			} else {
				AM_DEBUG(1, "PVR_DEBUG timeshift file was not open");
			}
		}
		else
		{
			AM_DEBUG(1, "PVR_DEBUG tfile is NULL");
		}
	}
}
static int aml_timeshift_data_write(int fd, uint8_t *buf, int size)
{
	int ret;
	int left = size;
	uint8_t *p = buf;

	while (left > 0)
	{
		ret = write(fd, p, left);
		if (ret == -1)
		{
			if (errno != EINTR)
			{
				AM_DEBUG(0, "Timeshift inject data failed: %s", strerror(errno));
				break;
			}
			ret = 0;
		}

		left -= ret;
		p += ret;
	}

	return (size - left);
}

static AV_TimeshiftSubfile_t *aml_timeshift_new_subfile(AV_TimeshiftFile_t *tfile)
{
	int flags;
	struct stat st;
	char fname[1024];
	AM_Bool_t is_timeshift = tfile->is_timeshift;
	AV_TimeshiftSubfile_t *sub_file, *ret = NULL;

	if (is_timeshift)
	{
		/* create new timeshifting sub files */
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	}
	else
	{
		/* gathering all pvr playback sub files */
		flags = O_WRONLY;
	}

	sub_file = (AV_TimeshiftSubfile_t *)malloc(sizeof(AV_TimeshiftSubfile_t));
	if (sub_file == NULL)
	{
		AM_DEBUG(1, "Cannot write sub file , no memory!\n");
		return ret;
	}

	sub_file->next = NULL;


	if (tfile->last_sub_index == 0 && !is_timeshift)
	{
		/* To be compatible with the old version */
		sub_file->findex = tfile->last_sub_index++;
		snprintf(fname, sizeof(fname), "%s", tfile->name);
	}
	else
	{
		sub_file->findex = tfile->last_sub_index++;
		snprintf(fname, sizeof(fname), "%s.%d", tfile->name, sub_file->findex);
	}

	if (!is_timeshift && stat(fname, &st) < 0)
	{
		AM_DEBUG(1, "PVR sub file '%s': %s", fname, strerror(errno));
		sub_file->wfd = -1;
		sub_file->rfd = -1;
	}
	else
	{
		AM_DEBUG(1, "openning %s\n", fname);
		sub_file->wfd = open(fname, flags, 0666);
		sub_file->rfd = open(fname, O_RDONLY, 0666);
	}

	if (sub_file->wfd < 0 || sub_file->rfd < 0)
	{
		AM_DEBUG(1, "Open file failed: %s", strerror(errno));
		if (is_timeshift)
		{
			AM_DEBUG(1, "Cannot open new sub file: %s\n", strerror(errno));
		}
		else
		{
			/* To be compatible with the old version */
			if (sub_file->findex == 0)
			{
				ret = aml_timeshift_new_subfile(tfile);
			}
			else
			{
				AM_DEBUG(1, "PVR playback sub files end at index %d", sub_file->findex-1);
			}
		}

		if (sub_file->wfd >= 0)
			close(sub_file->wfd);
		if (sub_file->rfd >= 0)
			close(sub_file->rfd);

		free(sub_file);
	}
	else
	{
		ret = sub_file;
	}

	return ret;
}

static ssize_t aml_timeshift_subfile_read(AV_TimeshiftFile_t *tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = 0;

	if (tfile->sub_files == NULL)
	{
		AM_DEBUG(1, "No sub files, cannot read\n");
		return -1;
	}

	if (tfile->cur_rsub_file == NULL)
	{
		tfile->cur_rsub_file = tfile->sub_files;
		AM_DEBUG(1, "Reading from the start, file index %d", tfile->cur_rsub_file->findex);
	}

	ret = read(tfile->cur_rsub_file->rfd, buf, size);
	if (ret == 0)
	{
		/* reach the end, automatically turn to next sub file */
		if (tfile->cur_rsub_file->next != NULL)
		{
			tfile->cur_rsub_file = tfile->cur_rsub_file->next;
			AM_DEBUG(1, "Reading from file index %d ...", tfile->cur_rsub_file->findex);
			lseek64(tfile->cur_rsub_file->rfd, 0, SEEK_SET);
			ret = aml_timeshift_subfile_read(tfile, buf, size);
		}
	}

	return ret;
}

static ssize_t aml_timeshift_subfile_write(AV_TimeshiftFile_t *tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = 0;
	loff_t fsize;

	if (tfile->sub_files == NULL)
	{
		AM_DEBUG(1, "No sub files, cannot write\n");
		return -1;
	}

	if (tfile->cur_wsub_file == NULL)
	{
		tfile->cur_wsub_file = tfile->sub_files;
		AM_DEBUG(1, "Switching to file index %d for writing...\n",
			tfile->cur_wsub_file->findex);
	}

	fsize = lseek64(tfile->cur_wsub_file->wfd, 0, SEEK_CUR);
	if (fsize >= tfile->sub_file_size)
	{
		AM_Bool_t start_new = AM_FALSE;

		if (tfile->cur_wsub_file->next != NULL)
		{
			tfile->cur_wsub_file = tfile->cur_wsub_file->next;
			start_new = AM_TRUE;
		}
		else
		{
			AV_TimeshiftSubfile_t *sub_file = aml_timeshift_new_subfile(tfile);
			if (sub_file != NULL)
			{
				tfile->cur_wsub_file->next = sub_file;
				tfile->cur_wsub_file = sub_file;
				start_new = AM_TRUE;
			}
		}

		if (start_new)
		{
			AM_DEBUG(1, "Switching to file index %d for writing...\n",
				tfile->cur_wsub_file->findex);
			lseek64(tfile->cur_wsub_file->wfd, 0, SEEK_SET);
			ret = aml_timeshift_subfile_write(tfile, buf, size);
		}
	}
	else
	{
		ret = aml_timeshift_data_write(tfile->cur_wsub_file->wfd, buf, size);
	}

	return ret;
}

static int aml_timeshift_subfile_close(AV_TimeshiftFile_t *tfile)
{
	AV_TimeshiftSubfile_t *sub_file, *next;
	char fname[1024];

	sub_file = tfile->sub_files;
	while (sub_file != NULL)
	{
		next = sub_file->next;
		if (sub_file->rfd >= 0)
		{
			close(sub_file->rfd);
		}
		if (sub_file->wfd >= 0)
		{
			close(sub_file->wfd);
		}

		if (tfile->is_timeshift)
		{
			snprintf(fname, sizeof(fname), "%s.%d", tfile->name, sub_file->findex);
			AM_DEBUG(1, "unlinking file: %s", fname);
			unlink(fname);
		}

		free(sub_file);
		sub_file = next;
	}

	return 0;
}

static int aml_timeshift_subfile_open(AV_TimeshiftFile_t *tfile)
{
	int index = 1, flags;
	char fname[1024];
	AV_TimeshiftSubfile_t *sub_file, *prev_sub_file, *exist_sub_file;
	AM_Bool_t is_timeshift = tfile->is_timeshift;
	loff_t left = is_timeshift ? tfile->size : 1/*just keep the while going*/;


	if (is_timeshift)
	{
		tfile->sub_files = aml_timeshift_new_subfile(tfile);
	}
	else
	{
		prev_sub_file = NULL;
		do
		{
			sub_file = aml_timeshift_new_subfile(tfile);
			if (sub_file != NULL)
			{
				off64_t size;

				if (prev_sub_file == NULL)
					tfile->sub_files = sub_file;
				else
					prev_sub_file->next = sub_file;
				prev_sub_file = sub_file;

				size = lseek64(sub_file->rfd, 0, SEEK_END);
				tfile->sub_file_size = AM_MAX(tfile->sub_file_size, size);
				tfile->size += size;
				lseek64(sub_file->rfd, 0, SEEK_SET);
			}
		} while (sub_file != NULL);
	}

	return 0;
}

static loff_t aml_timeshift_subfile_seek(AV_TimeshiftFile_t *tfile, loff_t offset, AM_Bool_t read)
{
	int sub_index = offset/tfile->sub_file_size; /* start from 0 */
	loff_t sub_offset = offset%tfile->sub_file_size;
	AV_TimeshiftSubfile_t *sub_file = tfile->sub_files;

	while (sub_file != NULL)
	{
		if (sub_file->findex == sub_index)
			break;
		sub_file = sub_file->next;
	}

	if (sub_file != NULL)
	{
		AM_DEBUG(1, "Seek to sub file %d at %lld\n", sub_index, sub_offset);
		if (read)
		{
			tfile->cur_rsub_file = sub_file;
			lseek64(sub_file->rfd, sub_offset, SEEK_SET);
		}
		else
		{
			tfile->cur_wsub_file = sub_file;
			lseek64(sub_file->wfd, sub_offset, SEEK_SET);
		}
		return offset;
	}

	return (loff_t)-1;
}


static AM_ErrorCode_t aml_timeshift_file_open(AV_TimeshiftFile_t *tfile, const char *file_name)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	const loff_t SUB_FILE_SIZE = 1024*1024*1024ll;

	if (tfile->opened)
	{
		AM_DEBUG(1, "Timeshift file has already opened");
		return ret;
	}

	tfile->name = strdup(file_name);
	tfile->sub_file_size = SUB_FILE_SIZE;
	tfile->is_timeshift = (strstr(tfile->name, "TimeShifting") != NULL);

	if (tfile->loop)
	{
		tfile->size = SUB_FILE_SIZE;

		if (aml_timeshift_subfile_open(tfile) < 0)
			return AM_AV_ERR_CANNOT_OPEN_FILE;

		tfile->avail = 0;
		tfile->total = 0;
	}
	else
	{
		if (aml_timeshift_subfile_open(tfile) < 0)
			return AM_AV_ERR_CANNOT_OPEN_FILE;

		tfile->avail = tfile->size;
		tfile->total = tfile->size;
		AM_DEBUG(1, "Playback total subfiles size %lld", tfile->size);
	}

	tfile->start = 0;
	tfile->read = tfile->write = 0;

	pthread_mutex_init(&tfile->lock, NULL);
	pthread_cond_init(&tfile->cond, NULL);

	tfile->opened = 1;

	return ret;
}

static AM_ErrorCode_t aml_timeshift_file_close(AV_TimeshiftFile_t *tfile)
{
	if (! tfile->opened)
	{
		AM_DEBUG(1, "Timeshift file has not opened");
		return AM_AV_ERR_INVAL_ARG;
	}
	tfile->opened = 0;
	pthread_mutex_lock(&tfile->lock);
	aml_timeshift_subfile_close(tfile);

	/*remove timeshift file*/
	if (tfile->name != NULL)
	{
		free(tfile->name);
		tfile->name = NULL;
	}
	pthread_mutex_unlock(&tfile->lock);
	pthread_mutex_destroy(&tfile->lock);

	return AM_SUCCESS;
}

static ssize_t aml_timeshift_file_read(AV_TimeshiftFile_t *tfile, uint8_t *buf, size_t size, int timeout)
{
	ssize_t ret = -1;
	size_t todo, split;
	struct timespec rt;

	if (! tfile->opened)
	{
		AM_DEBUG(1, "Timeshift file has not opened");
		return AM_AV_ERR_INVAL_ARG;
	}

	if (! tfile->loop)
	{
		ret = aml_timeshift_subfile_read(tfile, buf, size);
		if (ret > 0)
		{
			tfile->avail -= ret;
			if (tfile->avail < 0)
				tfile->avail = 0;
		}
		return ret;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->avail <= 0)
	{
		AM_TIME_GetTimeSpecTimeout(timeout, &rt);
		pthread_cond_timedwait(&tfile->cond, &tfile->lock, &rt);
	}
	if (tfile->avail <= 0)
	{
		tfile->avail = 0;
		goto read_done;
	}
	todo = (size < tfile->avail) ? size : tfile->avail;
	size = todo;
	split = ((tfile->read+size) > tfile->size) ? (tfile->size - tfile->read) : 0;
	if (split > 0)
	{
		/*read -> end*/
		ret = aml_timeshift_subfile_read(tfile, buf, split);
		if (ret < 0)
			goto read_done;
		if (ret != (ssize_t)split)
		{
			tfile->read += ret;
			goto read_done;
		}

		tfile->read = 0;
		todo -= ret;
		buf += ret;
	}
	/*rewind the file*/
	if (split > 0)
		aml_timeshift_subfile_seek(tfile, 0, AM_TRUE);
	ret = aml_timeshift_subfile_read(tfile, buf, todo);
	if (ret > 0)
	{
		todo -= ret;
		tfile->read += ret;
		tfile->read %= tfile->size;
		ret = size - todo;
	}

read_done:
	if (ret > 0)
	{
		tfile->avail -= ret;
		if (tfile->avail < 0)
			tfile->avail = 0;
	}
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

static ssize_t aml_timeshift_file_write(AV_TimeshiftFile_t *tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = -1;
	size_t  split;
	loff_t fsize, wpos;

	if (! tfile->opened)
	{
		AM_DEBUG(1, "Timeshift file has not opened");
		return AM_AV_ERR_INVAL_ARG;
	}

	if (! tfile->loop)
	{
		/* Normal write */
		ret = aml_timeshift_subfile_write(tfile, buf, size);
		if (ret > 0)
		{
			pthread_mutex_lock(&tfile->lock);
			tfile->size += ret;
			tfile->write += ret;
			tfile->total += ret;
			tfile->avail += ret;
			pthread_mutex_unlock(&tfile->lock);
		}
		goto write_done;
	}

	pthread_mutex_lock(&tfile->lock);
	fsize = tfile->size;
	wpos = tfile->write;
	pthread_mutex_unlock(&tfile->lock);
	/*is the size exceed the file size?*/
	if (size > fsize)
	{
		size = fsize;
		buf += fsize - size;
	}

	split = ((wpos+size) > fsize) ? (fsize - wpos) : 0;
	if (split > 0)
	{
		/*write -> end*/
		ret = aml_timeshift_subfile_write(tfile, buf, split);
		if (ret < 0)
			goto write_done;
		if (ret != (ssize_t)split)
			goto adjust_pos;

		size -= ret;
		buf += ret;
	}
	/*rewind the file*/
	if (split > 0)
		aml_timeshift_subfile_seek(tfile, 0, AM_FALSE);
	ret = aml_timeshift_subfile_write(tfile, buf, size);
	if (ret > 0)
		ret += split;
	else if (ret == 0)
		ret = split;

adjust_pos:
	if (ret > 0)
	{
		pthread_mutex_lock(&tfile->lock);
		/*now, ret bytes actually writen*/
		off_t rleft = tfile->size - tfile->avail;
		off_t sleft = tfile->size - tfile->total;

		tfile->write = (tfile->write + ret) % tfile->size;
		if (ret > rleft)
			tfile->read = tfile->write;
		if (ret > sleft)
			tfile->start = tfile->write;

		if (tfile->avail < tfile->size)
		{
			tfile->avail += ret;
			if (tfile->avail > tfile->size)
				tfile->avail = tfile->size;
			pthread_cond_signal(&tfile->cond);
		}
		if (tfile->total < tfile->size)
		{
			tfile->total += ret;
			if (tfile->total > tfile->size)
				tfile->total = tfile->size;
		}
		pthread_mutex_unlock(&tfile->lock);
	}
write_done:

	return ret;
}

/**\brief seek到指定的偏移，如越界则返回1*/
static int aml_timeshift_file_seek(AV_TimeshiftFile_t *tfile, loff_t offset)
{
	int ret = 1;

	pthread_mutex_lock(&tfile->lock);
	if (tfile->size <= 0)
	{
		pthread_mutex_unlock(&tfile->lock);
		return ret;
	}
	if (offset > tfile->total)
		offset = tfile->total - 1;
	else if (offset < 0)
		offset = 0;
	else
		ret = 0;

	tfile->read = (tfile->start + offset)%tfile->size;
	tfile->avail = tfile->total - offset;
	if (tfile->avail > 0)
		pthread_cond_signal(&tfile->cond);

	aml_timeshift_subfile_seek(tfile, tfile->read, AM_TRUE);

	AM_DEBUG(0, "Timeshift file Seek: start %lld, read %lld, write %lld, avail %lld, total %lld",
				tfile->start, tfile->read, tfile->write, tfile->avail, tfile->total);
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}
/**\brief 创建Timeshift相关数据*/
static AV_TimeshiftData_t* aml_create_timeshift_data(void)
{
	AV_TimeshiftData_t *tshift;

	tshift = (AV_TimeshiftData_t *)malloc(sizeof(AV_TimeshiftData_t));
	if (!tshift)
		return NULL;

	memset(tshift, 0, sizeof(AV_TimeshiftData_t));
	tshift->av_fd = -1;
	tshift->aud_idx = -1;
	tshift->aud_valid = AM_FALSE;

	return tshift;
}

static int aml_timeshift_inject(AV_TimeshiftData_t *tshift, uint8_t *data, int size, int timeout)
{
	int ret;
	int real_written = 0;
	int fd = tshift->av_fd;

	if (timeout >= 0)
	{
		struct pollfd pfd;

		pfd.fd = fd;
		pfd.events = POLLOUT;

		ret = poll(&pfd, 1, timeout);
		if (ret != 1)
		{
			AM_DEBUG(1, "timeshift poll timeout");
			goto inject_end;
		}
	}

	if (size)
	{
		ret = write(fd, data, size);
		if ((ret == -1) && (errno != EAGAIN))
		{
			AM_DEBUG(1, "inject data failed errno:%d msg:%s", errno, strerror(errno));
			goto inject_end;
		}
		else if ((ret == -1) && (errno == EAGAIN))
		{
			AM_DEBUG(1, "ret=%d,inject data failed errno:%d msg:%s",ret, errno, strerror(errno));
			real_written = 0;
		}
		else if (ret >= 0)
		{
			real_written = ret;
		}
	}

inject_end:
	return real_written;
}

static int aml_timeshift_get_trick_stat(AV_TimeshiftData_t *tshift)
{
	int state;

	if (tshift->cntl_fd == -1)
		return -1;

	ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICK_STAT, (unsigned long)&state);

	return state;
}

/**\brief 设置Timeshift参数*/
static AM_ErrorCode_t aml_start_timeshift(AV_TimeshiftData_t *tshift, AV_TimeShiftPlayPara_t *tshift_para, AM_Bool_t create_thread, AM_Bool_t start_audio)
{
	char buf[64];
	AM_AV_TimeshiftPara_t *para = &tshift_para->para;
	AM_Bool_t has_video = VALID_PID(para->media_info.vid_pid);
	AM_Bool_t has_audio = tshift->aud_valid;

	AM_DEBUG(1, "Openning demux%d",para->dmx_id);
	snprintf(buf, sizeof(buf), "/dev/dvb0.demux%d", para->dmx_id);
	tshift->dmxfd = open(buf, O_RDWR);
	if (tshift->dmxfd == -1)
	{
		AM_DEBUG(1, "cannot open \"/dev/dvb0.demux%d\" error:%d \"%s\"", para->dmx_id, errno, strerror(errno));
		return AM_AV_ERR_CANNOT_OPEN_DEV;
	}

	AM_FileRead("/sys/class/stb/source", tshift->last_stb_src, 16);
	snprintf(buf, sizeof(buf), "dmx%d", para->dmx_id);
	AM_FileEcho("/sys/class/stb/source", buf);

	snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", para->dmx_id);
	AM_FileRead(buf, tshift->last_dmx_src, 16);
	AM_FileEcho(buf, "hiu");

	snprintf(buf, sizeof(buf), "%d", 32*1024);
	AM_FileEcho("/sys/class/stb/asyncfifo0_flush_size", buf);

	AM_DEBUG(1, "Openning mpts");
	tshift->av_fd = open(STREAM_TS_FILE, O_RDWR);
	if (tshift->av_fd == -1)
	{
		AM_DEBUG(1, "cannot open device \"%s\"", STREAM_TS_FILE);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}
	AM_DEBUG(1, "Openning video");
	tshift->cntl_fd = open(AMVIDEO_FILE, O_RDWR);
	if (tshift->cntl_fd == -1)
	{
		AM_DEBUG(1, "cannot create data source \"/dev/amvideo\"");
		return AM_AV_ERR_CANNOT_OPEN_DEV;
	}
	AM_DEBUG(1, "Setting play param");
#if defined(ANDROID) || defined(CHIP_8626X)
	/*Set tsync enable/disable*/
	if (has_video && has_audio)
	{
		AM_DEBUG(1, "Set tsync enable to 1");
		aml_set_tsync_enable(1);
	}
	else
	{
		AM_DEBUG(1, "Set tsync enable to 0");
		aml_set_tsync_enable(0);
	}
#endif

	if (has_video)
	{
		if (ioctl(tshift->av_fd, AMSTREAM_IOC_VFORMAT, para->media_info.vid_fmt) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(tshift->av_fd, AMSTREAM_IOC_VID, para->media_info.vid_pid) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	if (has_audio)
	{
		if (ioctl(tshift->av_fd, AMSTREAM_IOC_AFORMAT, tshift->aud_fmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(tshift->av_fd, AMSTREAM_IOC_AID, tshift->aud_pid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
		/*if ((para->aud_fmt == AFORMAT_PCM_S16LE)||(para->aud_fmt == AFORMAT_PCM_S16BE)||
				(para->aud_fmt == AFORMAT_PCM_U8)) {
			ioctl(afd, AMSTREAM_IOC_ACHANNEL, para->channel);
			ioctl(afd, AMSTREAM_IOC_SAMPLERATE, para->sample_rate);
			ioctl(afd, AMSTREAM_IOC_DATAWIDTH, para->data_width);
		}*/
	}

	if (ioctl(tshift->av_fd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
	{
		AM_DEBUG(1, "amport init failed");
		return AM_AV_ERR_SYS;
	}

	if (has_audio && start_audio)
	{
		audio_info_t info;

		/*Set audio info*/
		memset(&info, 0, sizeof(info));
		info.valid  = 1;
		if (ioctl(tshift->av_fd, AMSTREAM_IOC_AUDIO_INFO, (unsigned long)&info) == -1)
		{
			AM_DEBUG(1, "set audio info failed");
		}

		adec_start_decode(tshift->av_fd, tshift->aud_fmt, has_video, &tshift->adec);
	}

	if (create_thread)
	{
		tshift->state = AV_TIMESHIFT_STAT_STOP;
		tshift->speed = 0;
		tshift->duration = para->media_info.duration;

		AM_DEBUG(1, "timeshifting mode %d, duration %d", para->mode, tshift->duration);
		if (para->mode == AM_AV_TIMESHIFT_MODE_PLAYBACK || tshift->duration <= 0)
			tshift->file.loop = AM_FALSE;
		else
			tshift->file.loop = AM_TRUE;

		AM_TRY(aml_timeshift_file_open(&tshift->file, para->file_path));

		if (! tshift->file.loop && tshift->duration)
		{
			tshift->rate = tshift->file.size/tshift->duration;
			AM_DEBUG(1, "@@@Playback file size %lld, duration %d s, the bitrate is %d bps", tshift->file.size, tshift->duration, tshift->rate);
		}
		tshift->running = 1;
		if (pthread_create(&tshift->thread, NULL, aml_timeshift_thread, (void*)tshift))
		{
			AM_DEBUG(1, "create the timeshift thread failed");
			tshift->running = 0;
		}
		else
		{
			tshift->para = *tshift_para;
			pthread_mutex_init(&tshift->lock, NULL);
			pthread_cond_init(&tshift->cond, NULL);
		}
		AM_DEBUG(1, "PVR_DEBUG create aml_term_signal_handler");
		signal(SIGTERM, &aml_term_signal_handler);
	}

	return AM_SUCCESS;
}

/**\brief 释放Timeshift相关数据*/
static void aml_destroy_timeshift_data(AV_TimeshiftData_t *tshift, AM_Bool_t destroy_thread)
{
	char buf[64];

	if (tshift->running && destroy_thread)
	{
		tshift->running = 0;
		pthread_cond_broadcast(&tshift->cond);
		pthread_join(tshift->thread, NULL);
		aml_timeshift_file_close(&tshift->file);
	}

	if (tshift->av_fd != -1)
	{

		AM_DEBUG(1, "Stopping Audio decode");
		aml_set_ad_source(&tshift->ad, 0, 0, 0, tshift->adec);
		adec_set_decode_ad(0, 0, 0, tshift->adec);
		adec_stop_decode(&tshift->adec);
		AM_DEBUG(1, "Closing mpts");
		close(tshift->av_fd);
	}
	AM_DEBUG(1, "Closing demux 1");
	if (tshift->dmxfd != -1)
		close(tshift->dmxfd);
	AM_DEBUG(1, "Closing video");
	if (tshift->cntl_fd != -1)
		close(tshift->cntl_fd);

	AM_FileEcho("/sys/class/stb/source", tshift->last_stb_src);
	snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", tshift->para.para.dmx_id);
	AM_FileEcho(buf, tshift->last_dmx_src);

	if (AM_FileRead("/sys/module/di/parameters/bypass_all", buf, sizeof(buf)) == AM_SUCCESS) {
		if (!strncmp(buf, "1", 1)) {
			AM_FileEcho("/sys/module/di/parameters/bypass_all","0");
		}
	}

	if (destroy_thread)
		free(tshift);
}

static int am_timeshift_reset(AV_TimeshiftData_t *tshift, int deinterlace_val, AM_Bool_t start_audio)
{
	UNUSED(deinterlace_val);

	aml_destroy_timeshift_data(tshift, AM_FALSE);

	aml_start_timeshift(tshift, &tshift->para, AM_FALSE, start_audio);

	/*Set the left to 0, we will read from the new point*/
	tshift->left = 0;

	return 0;
}

static int am_timeshift_reset_continue(AV_TimeshiftData_t *tshift, int deinterlace_val, AM_Bool_t start_audio)
{
	UNUSED(deinterlace_val);

	aml_destroy_timeshift_data(tshift, AM_FALSE);

	aml_start_timeshift(tshift, &tshift->para, AM_FALSE, start_audio);

	return 0;
}

static int am_timeshift_fffb(AV_TimeshiftData_t *tshift)
{
	int now, ret, next_time;
	loff_t offset;
	int speed = tshift->speed/*(tshift->speed/AM_ABS(tshift->speed))*(2<<(AM_ABS(tshift->speed) - 1))*/;

	if (speed)
	{
		AM_TIME_GetClock(&now);
		if (speed < 0)
			speed += -1;
		next_time = tshift->fffb_base + speed *(now - tshift->fffb_time);

		tshift->fffb_base = next_time;
		tshift->fffb_time = now;

		next_time /= 1000;
		offset = (loff_t)next_time * (loff_t)tshift->rate;
		AM_DEBUG(1, "fffb_base is %d, distance is %d, speed is %d", tshift->fffb_base, speed *(now - tshift->fffb_time)/1000, speed);
		ret = aml_timeshift_file_seek(&tshift->file, offset);
		AM_DEBUG(1,"FFFB next offset is %lld, next time is %d, reach end %d, speed %d, distance %d", offset, next_time, ret, tshift->speed, now - tshift->fffb_time);

		am_timeshift_reset(tshift, 0, AM_FALSE);
	}
	else
	{
		/*speed is 0, turn to play*/
		ret = 1;
	}

	if (ret)
	{
		/*FF FB reach end or speed is 0, so after this loop, we will turn to PLAY*/
		pthread_mutex_lock(&tshift->lock);
		tshift->cmd = AV_PLAY_START;
		pthread_mutex_unlock(&tshift->lock);
	}

	return 0;
}

static int aml_timeshift_pause_av(AV_TimeshiftData_t *tshift)
{
	if (tshift->aud_valid)
	{
#if defined(ADEC_API_NEW)
		audio_decode_pause(tshift->adec);
#else
		//TODO
#endif
	}

	if (tshift->para.para.media_info.vid_pid < 0x1fff)
	{
		ioctl(tshift->cntl_fd, AMSTREAM_IOC_VPAUSE, 1);
	}

	return 0;
}

static int aml_timeshift_resume_av(AV_TimeshiftData_t *tshift)
{
	if (tshift->aud_valid)
	{
#if defined(ADEC_API_NEW)
		audio_decode_resume(tshift->adec);
#else
		//TODO
#endif
	}
	if (tshift->para.para.media_info.vid_pid < 0x1fff)
	{
		ioctl(tshift->cntl_fd, AMSTREAM_IOC_VPAUSE, 0);
	}


	return 0;
}

static int aml_timeshift_do_play_cmd(AV_TimeshiftData_t *tshift, AV_PlayCmd_t cmd, AM_AV_TimeshiftInfo_t *info)
{
	AV_TimeshiftState_t	last_state = tshift->state;
	loff_t offset;

	if (! tshift->rate && (cmd == AV_PLAY_FF || cmd == AV_PLAY_FB || cmd == AV_PLAY_SEEK))
	{
		AM_DEBUG(1, "Have not got the rate, skip this command");
		return -1;
	}

	if (tshift->last_cmd != cmd)
	{
		switch (cmd)
		{
		case AV_PLAY_START:
			if (tshift->state != AV_TIMESHIFT_STAT_PLAY)
			{
				tshift->inject_size = 64*1024;
				tshift->timeout = 0;
				tshift->state = AV_TIMESHIFT_STAT_PLAY;
				AM_DEBUG(1, "@@@Timeshift start normal play, seek to time %d s...", info->current_time);
				offset = (loff_t)info->current_time * (loff_t)tshift->rate ;
				aml_timeshift_file_seek(&tshift->file, offset);
				ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				am_timeshift_reset(tshift, 2, AM_TRUE);

				//if (tshift->last_cmd == AV_PLAY_FF || tshift->last_cmd == AV_PLAY_FB)
				{
					//usleep(200*1000);
					AM_DEBUG(1, "set di bypass_all to 0");
					AM_FileEcho("/sys/module/di/parameters/bypass_all","0");
				}
			}
			break;
		case AV_PLAY_PAUSE:
			if (tshift->state != AV_TIMESHIFT_STAT_PAUSE)
			{
				tshift->inject_size = 0;
				tshift->timeout = 1000;
				tshift->state = AV_TIMESHIFT_STAT_PAUSE;
				aml_timeshift_pause_av(tshift);
				AM_DEBUG(1, "@@@Timeshift Paused");
			}
			break;
		case AV_PLAY_RESUME:
			if (tshift->state == AV_TIMESHIFT_STAT_PAUSE)
			{
				tshift->inject_size = 64*1024;
				tshift->timeout = 0;
				tshift->state = AV_TIMESHIFT_STAT_PLAY;
				aml_timeshift_resume_av(tshift);
				AM_DEBUG(1, "@@@Timeshift Resumed");
			}
			break;
		case AV_PLAY_FF:
		case AV_PLAY_FB:
			if (tshift->state != AV_TIMESHIFT_STAT_FFFB)
			{
				if (tshift->speed == 0 && tshift->state == AV_TIMESHIFT_STAT_PLAY)
					return 0;
				tshift->inject_size = 64*1024;
				tshift->timeout = 0;
				tshift->state = AV_TIMESHIFT_STAT_FFFB;

				if (tshift->last_cmd == AV_PLAY_START)
				{
					AM_DEBUG(1, "set di bypass_all to 1");
					AM_FileEcho("/sys/module/di/parameters/bypass_all","1");
					usleep(200*1000);
				}

				ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_FFFB);

				AM_TIME_GetClock(&tshift->fffb_time);
				tshift->fffb_base = info->current_time*1000;
				am_timeshift_fffb(tshift);
			}
			break;
		case AV_PLAY_SEEK:
			offset = (loff_t)tshift->seek_pos * (loff_t)tshift->rate;
			AM_DEBUG(1, "Timeshift seek to offset %lld, time %d s", offset, tshift->seek_pos);
			aml_timeshift_file_seek(&tshift->file, offset);
			ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			am_timeshift_reset(tshift, 0, AM_TRUE);
			tshift->inject_size = 0;
			tshift->timeout = 0;
			tshift->state = AV_TIMESHIFT_STAT_SEARCHOK;
			info->current_time = (tshift->file.total - tshift->file.avail)*tshift->duration/tshift->file.size;
			break;
		case AV_PLAY_RESET_VPATH:
			ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			/*stop play first*/
			aml_destroy_timeshift_data(tshift, AM_FALSE);
			/*reset the vpath*/
			aml_set_vpath(tshift->dev);
			/*restart play now*/
			aml_start_timeshift(tshift, &tshift->para, AM_FALSE, AM_TRUE);

			/*Set the left to 0, we will read from the new point*/
			tshift->left = 0;
			tshift->inject_size = 0;
			tshift->timeout = 0;
			/* will turn to play state from current_time */
			tshift->state = AV_TIMESHIFT_STAT_SEARCHOK;
			break;
		case AV_PLAY_SWITCH_AUDIO:
			/* just restart play using the new audio para */
			AM_DEBUG(1, "Switch audio to pid=%d, fmt=%d, valid=%d",tshift->aud_pid, tshift->aud_fmt, tshift->aud_valid);
			/*Set the left to 0, we will read from the new point*/
			tshift->left = 0;
			tshift->inject_size = 0;
			tshift->timeout = 0;
			if (tshift->state == AV_TIMESHIFT_STAT_PAUSE)
			{
				/* keep pause with the new audio */
				ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				am_timeshift_reset(tshift, 2, AM_TRUE);
				tshift->timeout = 1000;
				aml_timeshift_pause_av(tshift);
			}
			else
			{
				/* will turn to play state from current_time */
				tshift->state = AV_TIMESHIFT_STAT_SEARCHOK;
			}
			break;
		default:
			AM_DEBUG(1, "Unsupported timeshift play command %d", cmd);
			return -1;
			break;
		}
	}

	if (tshift->last_cmd != cmd)
		tshift->last_cmd = cmd;

	if (tshift->state != last_state)
	{
		/*Notify status changed*/
		AM_DEBUG(1, "Notify status changed: 0x%x->0x%x", last_state, tshift->state);
		info->status = tshift->state;
		AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)info);

		/*if (tshift->state != AV_TIMESHIFT_STAT_FFFB && tshift->state != AV_TIMESHIFT_STAT_SEARCHOK)
		{
			ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
		}
		if (last_state == AV_TIMESHIFT_STAT_FFFB || last_state == AV_TIMESHIFT_STAT_SEARCHOK)
		{
			am_timeshift_reset(tshift, 2);
		}*/
		if (tshift->state == AV_TIMESHIFT_STAT_SEARCHOK)
		{
			pthread_mutex_lock(&tshift->lock);
			tshift->cmd = AV_PLAY_START;
			pthread_mutex_unlock(&tshift->lock);
		}
	}
	return 0;
}

static void aml_timeshift_update_info(AV_TimeshiftData_t *tshift, AM_AV_TimeshiftInfo_t *info)
{
	pthread_mutex_lock(&tshift->lock);
	tshift->info = *info;
	pthread_mutex_unlock(&tshift->lock);
}

/**\brief Timeshift 线程*/
static void *aml_timeshift_thread(void *arg)
{
	AV_TimeshiftData_t *tshift = (AV_TimeshiftData_t *)arg;
	int speed, update_time, now/*, fffb_time*/;
	int len, ret;
	int /*dmxfd,*/ trick_stat;
	uint8_t buf[64*1024];
	const int FFFB_STEP = 150;
	struct timespec rt;
	AM_AV_TimeshiftInfo_t info;
	struct am_io_param astatus;
	struct am_io_param vstatus;
	AV_PlayCmd_t cmd;
	int playback_alen=0, playback_vlen=0;
	AM_Bool_t is_playback_mode = (tshift->para.para.mode == AM_AV_TIMESHIFT_MODE_PLAYBACK);
	int vbuf_len = 0;
	int abuf_len = 0;
	int skip_flag_count = 0;
	int dmx_vpts = 0,  vpts = 0;
	char pts_buf[32];
	int diff = 0, last_diff = 0;
	int error_cnt = 0;

	memset(pts_buf, 0, sizeof(pts_buf));
	memset(&info, 0, sizeof(info));
	tshift->last_cmd = -1;
	AM_DEBUG(1, "Starting timeshift player thread ...");
	info.status = AV_TIMESHIFT_STAT_INITOK;
	AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)&info);
	AM_TIME_GetClock(&update_time);

	AM_FileEcho(VID_BLACKOUT_FILE, "0");
	while (tshift->running || tshift->cmd != tshift->last_cmd )
	{
		pthread_mutex_lock(&tshift->lock);
		cmd = tshift->cmd;
		speed = tshift->speed;
		pthread_mutex_unlock(&tshift->lock);

		if ((tshift->last_cmd == AV_PLAY_FF || tshift->last_cmd == AV_PLAY_FB) && cmd == AV_PLAY_PAUSE)
		{
			/*
			 * $ID: 101093,Fixed, timeshift resume decoder is stop when status from FastForward(or FastBack) to Pause directly
			 */
			AM_DEBUG(1, "@@@last cmd is %s, cur_cmd is AV_PLAY_PAUSE, do AV_PLAY_START first",
					(tshift->last_cmd == AV_PLAY_FF)? "AV_PLAY_FF" : "AV_PLAY_FB");
			aml_timeshift_do_play_cmd(tshift, AV_PLAY_START, &info);
		}
		aml_timeshift_do_play_cmd(tshift, cmd, &info);

		/*read some bytes*/
		len = sizeof(buf) - tshift->left;
		if (len > 0)
		{
			/*
			 *	$ID: 100351, fixed timeshift switch subtitle too slow.
			 */
			if (AM_FileRead(VIDEO_DMX_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
				sscanf(pts_buf, "%d", &dmx_vpts);
			} else {
				AM_DEBUG(1, "cannot read \"%s\"", VIDEO_DMX_PTS_FILE);
				dmx_vpts = 0;
			}
			if (AM_FileRead(VIDEO_PTS_FILE, pts_buf, sizeof(pts_buf)) >= 0) {
				sscanf(pts_buf+2, "%x", &vpts);
			} else {
				AM_DEBUG(1, "cannot read \"%s\"", VIDEO_PTS_FILE);
				vpts = 0;
			}
			if (tshift->state == AV_TIMESHIFT_STAT_PLAY &&
				ioctl(tshift->av_fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1)
			{
				AM_DEBUG(1, "vstat_len:%d , file_avail:%lld", vstatus.status.data_len , tshift->file.avail);
			}
			else
			{
				AM_DEBUG(1,"get v_stat_len failed");
			}

			diff = (dmx_vpts - vpts)/90000;
			//AM_DEBUG(1, "#### vpts:%#x, dmx_vpts:%#x, diff:%d, %s, %s, play_stat:%d\n",
			// vpts, dmx_vpts, diff, diff>TIMESHIFT_INJECT_DIFF_TIME?"large":"small", (diff==last_diff)?"equal":"not equal", tshift->state);
			if (vpts != 0 && dmx_vpts != 0 && diff > TIMESHIFT_INJECT_DIFF_TIME &&
				vstatus.status.data_len > DEC_STOP_VIDEO_LEVEL && tshift->state == AV_TIMESHIFT_STAT_PLAY)
			{
				last_diff = diff;
				tshift->timeout = 10;
				goto wait_for_next_loop;
			}
			/*
			 *	## end of 100351
			 */

			ret = aml_timeshift_file_read(&tshift->file, buf+tshift->left, len, 100);
			if (ret > 0)
			{
				tshift->left += ret;
			}
			else
			{
				AM_DEBUG(1, "read playback file failed: %s", strerror(errno));
				if (errno == EIO && is_playback_mode)
				{
					AM_DEBUG(1, "Disk may be plugged out, exit playback.");
					break;
				}
			}
		}

		/*Inject*/
		if (tshift->inject_size > 0)
		{
			if (tshift->state == AV_TIMESHIFT_STAT_PLAY &&
				ioctl(tshift->av_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1 &&
				ioctl(tshift->av_fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1)
			{
				if (vstatus.status.data_len == 0 && astatus.status.data_len > 100*1024)
				{
					tshift->timeout = 10;
					goto wait_for_next_loop;
				}
				else
				{
					tshift->timeout = 0;
				}
			}
			ret = AM_MIN(tshift->left , tshift->inject_size);
			if (ret > 0)
				ret = aml_timeshift_inject(tshift, buf, ret, -1);

			if (ret > 0)
			{
				/*ret bytes written*/
				tshift->left -= ret;
				if (tshift->left > 0)
					memmove(buf, buf+ret, tshift->left);
			}
		}

		AM_TIME_GetClock(&now);

		/*Update the playing info*/
		if (tshift->state == AV_TIMESHIFT_STAT_FFFB)
		{
			if(vpts == 0)
			{
				AM_DEBUG(1, "@@@ FF_OR_FB get vpts==0");
				error_cnt++;
			}
			else
			{
				error_cnt = 0;
			}
			if (error_cnt > TIMESHIFT_FFFB_ERROR_CNT)
			{
				AM_DEBUG(1, "@@@ FF_OR_FB error_cnt is overflow, reset trick_mode");
				ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
				ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_FFFB);
				error_cnt = 0;
			}
			tshift->timeout = 0;
			if (tshift->para.para.media_info.vid_pid < 0x1fff)
				trick_stat = aml_timeshift_get_trick_stat(tshift);
			else
				trick_stat = 1;
			AM_DEBUG(7, "trick_stat is %d", trick_stat);
			if (trick_stat > 0)
			{
				tshift->timeout = FFFB_STEP;
				info.current_time = tshift->fffb_base/1000;

				if (is_playback_mode || tshift->file.loop)
					info.full_time = tshift->duration;
				else if (tshift->rate)
					info.full_time = tshift->file.total/tshift->rate;
				else
					info.full_time = 0;
				info.status = tshift->state;

				AM_DEBUG(1, "Notify time update");
				AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)&info);
				aml_timeshift_update_info(tshift, &info);
				am_timeshift_fffb(tshift);
			}
			else if ((now - tshift->fffb_time) > 2000)
			{
				AM_DEBUG(1, "FFFB seek frame timeout, maybe no data available");
				am_timeshift_fffb(tshift);
			}

		}
		else if ((now - update_time) >= 1000)
		{
			if (ioctl(tshift->av_fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1 &&
			ioctl(tshift->av_fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1)
			{
				update_time = now;
				AM_DEBUG(1, "is_playback_mode = %d,tshift->file.loop = %d",is_playback_mode,tshift->file.loop);

				if (is_playback_mode || tshift->file.loop)
					info.full_time = tshift->duration;
				else if (tshift->rate)
					info.full_time = tshift->file.total/tshift->rate;
				else
					info.full_time = 0;

				if (!is_playback_mode && !tshift->file.loop)
					tshift->duration = info.full_time;

				AM_DEBUG(1, "total %lld, avail %lld, alen %d, vlen %d, duration %d, size %lld , tshift rate = %d",
					tshift->file.total , tshift->file.avail , astatus.status.data_len ,
					vstatus.status.data_len, tshift->duration, tshift->file.size,tshift->rate);

				if (tshift->rate)
				{
					/* approximate, not accurate */
					loff_t buffered_es_len = astatus.status.data_len+vstatus.status.data_len;
					loff_t buffered_ts_en = 188*buffered_es_len/184;

					info.current_time = (tshift->file.total - tshift->file.avail - buffered_ts_en)
										* (loff_t)tshift->duration / tshift->file.size;
				}
				else
				{
					info.current_time = 0;
				}

				if (info.current_time < 0)
				{
					/*Exceed, play from the start*/
					aml_timeshift_file_seek(&tshift->file, 0);
					am_timeshift_reset(tshift, -1, AM_TRUE);
					info.current_time = 0;
				}

				if (tshift->state == AV_TIMESHIFT_STAT_PLAY)
				{
					/********Skip inject error*********/
					if (abuf_len != astatus.status.data_len) {
						abuf_len = astatus.status.data_len;
						skip_flag_count=0;
					}
					if (vbuf_len != vstatus.status.data_len) {
						vbuf_len =vstatus.status.data_len;
						skip_flag_count=0;
					}

					if (info.current_time > 0 && astatus.status.data_len == abuf_len && vstatus.status.data_len == vbuf_len) {
						skip_flag_count++;
					}

					if (skip_flag_count >= 4) {
						aml_timeshift_file_seek(&tshift->file,tshift->file.total - tshift->file.avail);
						am_timeshift_reset_continue(tshift, -1, AM_TRUE);
						vbuf_len = 0;
						abuf_len = 0;
						skip_flag_count=0;
					}
				}

				info.status = tshift->state;

				AM_DEBUG(1, "Notify time update, current %d, total %d", info.current_time, info.full_time);
				AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)&info);
				aml_timeshift_update_info(tshift, &info);

				/*If there is no data available in playback only mode, we send exit event*/
				if (is_playback_mode && !tshift->file.avail&&tshift->state != AV_TIMESHIFT_STAT_PAUSE)
				{
					if (playback_alen == astatus.status.data_len &&
						playback_vlen == vstatus.status.data_len)
					{
						AM_DEBUG(1, "Playback End");
						info.current_time = info.full_time;
						break;
					}
					else
					{
						playback_alen = astatus.status.data_len;
						playback_vlen = vstatus.status.data_len;
					}
				}
			}
		}

wait_for_next_loop:
		if (tshift->timeout == -1)
		{
			pthread_mutex_lock(&tshift->lock);
			pthread_cond_wait(&tshift->cond, &tshift->lock);
			pthread_mutex_unlock(&tshift->lock);
		}
		else if (tshift->timeout > 0)
		{
			pthread_mutex_lock(&tshift->lock);
			AM_TIME_GetTimeSpecTimeout(tshift->timeout, &rt);
			pthread_cond_timedwait(&tshift->cond, &tshift->lock, &rt);
			pthread_mutex_unlock(&tshift->lock);
		}
	}

	AM_DEBUG(1, "Timeshift player thread exit now");
	info.status = AV_TIMESHIFT_STAT_EXIT;
	AM_EVT_Signal(0, AM_AV_EVT_PLAYER_UPDATE_INFO, (void*)&info);
	aml_timeshift_update_info(tshift, &info);

	ioctl(tshift->cntl_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
	AM_FileEcho(VID_BLACKOUT_FILE, tshift->dev->video_blackout ? "1" : "0");

	return NULL;
}

static AM_ErrorCode_t aml_timeshift_fill_data(AM_AV_Device_t *dev, uint8_t *data, int size)
{
	AV_TimeshiftData_t *tshift = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;
	int now;

	if (tshift)
	{
		ssize_t ret;

		if (size > 0 && !tshift->rate)
		{
			AM_TIME_GetClock(&now);
			if (! tshift->rtime)
			{
				tshift->rtotal = 0;
				tshift->rtime = now;
			}
			else
			{
				if ((now - tshift->rtime) >= 3000)
				{
					tshift->rtotal += size;
					/*Calcaulate the rate*/
					tshift->rate = (tshift->rtotal*1000)/(now - tshift->rtime);
					if (tshift->rate && tshift->file.loop)
					{
						/*Calculate the file size*/
						tshift->file.size = (loff_t)tshift->rate * (loff_t)tshift->duration;
						pthread_cond_signal(&tshift->cond);
						AM_DEBUG(1, "@@@wirte record data %lld bytes in %d ms,so the rate is assumed to %d bps, ring file size %lld",
							tshift->rtotal, now - tshift->rtime, tshift->rate*8, tshift->file.size);
					}
					else
					{
						tshift->rtime = 0;
					}
				}
				else
				{
					tshift->rtotal += size;
				}
			}
		}

		ret = aml_timeshift_file_write(&tshift->file, data, size);
		if (ret != (ssize_t)size)
		{
			AM_DEBUG(1, "Write timeshift data to file failed");
			/*A write error*/
			return AM_AV_ERR_SYS;
		}
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_timeshift_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
	AV_TimeshiftData_t *data;

	data = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;

	pthread_mutex_lock(&data->lock);
	data->cmd = cmd;
	if (cmd == AV_PLAY_FF || cmd == AV_PLAY_FB)
	{
		data->speed = (long)para;
	}
	else if (cmd == AV_PLAY_SEEK)
	{
		data->seek_pos = ((AV_FileSeekPara_t *)para)->pos;;
	}
	else if (cmd == AV_PLAY_SWITCH_AUDIO)
	{
		AV_TSPlayPara_t *audio_para = (AV_TSPlayPara_t *)para;
		if (audio_para)
		{
			int i;

			for (i=0; i<data->para.para.media_info.aud_cnt; i++)
			{
				if (data->para.para.media_info.audios[i].pid == audio_para->apid &&
					data->para.para.media_info.audios[i].fmt == audio_para->afmt)
				{
					data->aud_pid = audio_para->apid;
					data->aud_fmt = audio_para->afmt;
					data->aud_idx = i;
					data->aud_valid = VALID_AUDIO(data->aud_pid, data->aud_fmt);
					break;
				}
			}
		}
	}
	pthread_cond_signal(&data->cond);
	pthread_mutex_unlock(&data->lock);

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_timeshift_get_info(AM_AV_Device_t *dev, AM_AV_TimeshiftInfo_t *info)
{
	AV_TimeshiftData_t *data;

	data = (AV_TimeshiftData_t *)dev->timeshift_player.drv_data;

	pthread_mutex_lock(&data->lock);
	*info = data->info;
	pthread_mutex_unlock(&data->lock);

	return AM_SUCCESS;
}

extern unsigned long CMEM_getPhys(unsigned long virts);

/**\brief 解码JPEG数据*/
static AM_ErrorCode_t aml_decode_jpeg(AV_JPEGData_t *jpeg, const uint8_t *data, int len, int mode, void *para)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
#if !defined(ANDROID)
	AV_JPEGDecodePara_t *dec_para = (AV_JPEGDecodePara_t *)para;
	AV_JPEGDecState_t s;
	AM_Bool_t decLoop = AM_TRUE;
	int decState = 0;
	int try_count = 0;
	int decode_wait = 0;
	const uint8_t *src = data;
	int left = len;
	AM_OSD_Surface_t *surf = NULL;
	jpegdec_info_t info;

	char tmp_buf[64];

	s = AV_JPEG_DEC_STAT_INFOCONFIG;
	while (decLoop)
	{
		if (jpeg->dec_fd == -1)
		{
			jpeg->dec_fd = open(JPEG_DEC_FILE, O_RDWR);
			if (jpeg->dec_fd != -1)
			{
				s = AV_JPEG_DEC_STAT_INFOCONFIG;
			}
			else
			{
				try_count++;
				if (try_count > 40)
				{
					AM_DEBUG(1, "jpegdec init timeout");
					try_count=0;
					ret = aml_init_jpeg(jpeg);
					if (ret != AM_SUCCESS)
						break;
				}
				usleep(10000);
				continue;
			}
		}
		else
		{
			decState = ioctl(jpeg->dec_fd, JPEGDEC_IOC_STAT);

			if (decState & JPEGDEC_STAT_ERROR)
			{
				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_ERROR");
				ret = AM_AV_ERR_DECODE;
				break;
			}

			if (decState & JPEGDEC_STAT_UNSUPPORT)
			{
				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_UNSUPPORT");
				ret = AM_AV_ERR_DECODE;
				break;
			}

			if (decState & JPEGDEC_STAT_DONE)
				break;

			if (decState & JPEGDEC_STAT_WAIT_DATA)
			{
				if (left > 0)
				{
					int send = AM_MIN(left, JPEG_WRTIE_UNIT);
					int rc;
					rc = write(jpeg->vbuf_fd, src, send);
					if (rc == -1)
					{
						AM_DEBUG(1, "write data to the jpeg decoder failed");
						ret = AM_AV_ERR_DECODE;
						break;
					}
					left -= rc;
					src  += rc;
				}
				else if (decode_wait == 0)
				{
					int i, times = JPEG_WRTIE_UNIT/sizeof(tmp_buf);

					memset(tmp_buf, 0, sizeof(tmp_buf));

					for (i=0; i<times; i++)
						write(jpeg->vbuf_fd, tmp_buf, sizeof(tmp_buf));
					decode_wait++;
				}
				else
				{
					if (decode_wait > 300)
					{
						AM_DEBUG(1, "jpegdec wait data error");
						ret = AM_AV_ERR_DECODE;
						break;
					}
					decode_wait++;
					usleep(10);
				}
			}

			switch (s)
			{
				case AV_JPEG_DEC_STAT_INFOCONFIG:
					if (decState & JPEGDEC_STAT_WAIT_INFOCONFIG)
					{
						if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFOCONFIG, 0) == -1)
						{
							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFOCONFIG error");
							ret = AM_AV_ERR_DECODE;
							decLoop = AM_FALSE;
						}
						s = AV_JPEG_DEC_STAT_INFO;
					}
					break;
				case AV_JPEG_DEC_STAT_INFO:
					if (decState & JPEGDEC_STAT_INFO_READY)
					{
						if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFO, &info) == -1)
						{
							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFO error");
							ret = AM_AV_ERR_DECODE;
							decLoop = AM_FALSE;
						}
						if (mode & AV_GET_JPEG_INFO)
						{
							AM_AV_JPEGInfo_t *jinfo = (AM_AV_JPEGInfo_t *)para;
							jinfo->width    = info.width;
							jinfo->height   = info.height;
							jinfo->comp_num = info.comp_num;
							decLoop = AM_FALSE;
						}
						AM_DEBUG(2, "jpegdec width:%d height:%d", info.width, info.height);
						s = AV_JPEG_DEC_STAT_DECCONFIG;
					}
					break;
				case AV_JPEG_DEC_STAT_DECCONFIG:
					if (decState & JPEGDEC_STAT_WAIT_DECCONFIG)
					{
						jpegdec_config_t config;
						int dst_w, dst_h;

						switch (dec_para->para.angle)
						{
							case AM_AV_JPEG_CLKWISE_0:
							default:
								dst_w = info.width;
								dst_h = info.height;
							break;
							case AM_AV_JPEG_CLKWISE_90:
								dst_w = info.height;
								dst_h = info.width;
							break;
							case AM_AV_JPEG_CLKWISE_180:
								dst_w = info.width;
								dst_h = info.height;
							break;
							case AM_AV_JPEG_CLKWISE_270:
								dst_w = info.height;
								dst_h = info.width;
							break;
						}

						if (dec_para->para.width > 0)
							dst_w = AM_MIN(dst_w, dec_para->para.width);
						if (dec_para->para.height > 0)
							dst_h = AM_MIN(dst_h, dec_para->para.height);

						ret = AM_OSD_CreateSurface(AM_OSD_FMT_YUV_420, dst_w, dst_h, AM_OSD_SURFACE_FL_HW, &surf);
						if (ret != AM_SUCCESS)
						{
							AM_DEBUG(1, "cannot create the YUV420 surface");
							decLoop = AM_FALSE;
						}
						else
						{
							config.addr_y = CMEM_getPhys((unsigned long)surf->buffer);
							config.addr_u = config.addr_y+CANVAS_ALIGN(surf->width)*surf->height;
							config.addr_v = config.addr_u+CANVAS_ALIGN(surf->width/2)*(surf->height/2);
							config.opt    = dec_para->para.option;
							config.dec_x  = 0;
							config.dec_y  = 0;
							config.dec_w  = surf->width;
							config.dec_h  = surf->height;
							config.angle  = dec_para->para.angle;
							config.canvas_width = CANVAS_ALIGN(surf->width);

							if (ioctl(jpeg->dec_fd, JPEGDEC_IOC_DECCONFIG, &config) == -1)
							{
								AM_DEBUG(1, "jpegdec JPEGDEC_IOC_DECCONFIG error");
								ret = AM_AV_ERR_DECODE;
								decLoop = AM_FALSE;
							}
							s = AV_JPEG_DEC_STAT_RUN;
						}
					}
				break;

				default:
					break;
			}
		}
	}

	if (surf)
	{
		if (ret == AM_SUCCESS)
		{
			dec_para->surface = surf;
		}
		else
		{
			AM_OSD_DestroySurface(surf);
		}
	}
#else
	UNUSED(jpeg);
	UNUSED(data);
	UNUSED(len);
	UNUSED(mode);
	UNUSED(para);
#endif

	return ret;
}

static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para)
{
//#ifndef ANDROID
	char buf[32];
	int v;

	UNUSED(para);

	if (AM_FileRead(VID_AXIS_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		int left, top, right, bottom;

		if (sscanf(buf, "%d %d %d %d", &left, &top, &right, &bottom) == 4)
		{
			dev->video_x = left;
			dev->video_y = top;
			dev->video_w = right-left;
			dev->video_h = bottom-top;
		}
	}
	if (AM_FileRead(VID_CONTRAST_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_contrast = v;
		}
	}
	if (AM_FileRead(VID_SATURATION_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_saturation = v;
		}
	}
	if (AM_FileRead(VID_BRIGHTNESS_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_brightness = v;
		}
	}
	if (AM_FileRead(VID_DISABLE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_enable = v?AM_FALSE:AM_TRUE;
		}
	}
	if (AM_FileRead(VID_BLACKOUT_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_blackout = v?AM_TRUE:AM_FALSE;
		}
	}
	if (AM_FileRead(VID_SCREEN_MODE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			dev->video_mode = v;
#ifndef 	CHIP_8226H
			switch(v) {
				case 0:
				case 1:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_AUTO;
					dev->video_mode = v;
					break;
				case 2:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_4_3;
					dev->video_mode = AM_AV_VIDEO_DISPLAY_FULL_SCREEN;
					break;
				case 3:
					dev->video_ratio = AM_AV_VIDEO_ASPECT_16_9;
					dev->video_mode = AM_AV_VIDEO_DISPLAY_FULL_SCREEN;
					break;
			}
 #endif
		}
	}
	if (AM_FileRead(DISP_MODE_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "576cvbs", 7) || !strncmp(buf, "576i", 4) || !strncmp(buf, "576i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 576;
		}
		else if (!strncmp(buf, "480cvbs", 7) || !strncmp(buf, "480i", 4) || !strncmp(buf, "480i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 480;
		}
		else if (!strncmp(buf, "720p", 4))
		{
			dev->vout_w = 1280;
			dev->vout_h = 720;
		}
		else if (!strncmp(buf, "1080i", 5) || !strncmp(buf, "1080p", 5))
		{
			dev->vout_w = 1920;
			dev->vout_h = 1080;
		}
	}
#ifdef CHIP_8226H
	if (AM_FileRead(VID_ASPECT_RATIO_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "auto", 4))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_AUTO;
		}
		else if (!strncmp(buf, "16x9", 4))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_16_9;
		}
		else if (!strncmp(buf, "4x3", 3))
		{
			dev->video_ratio = AM_AV_VIDEO_ASPECT_4_3;
		}
	}
	if (AM_FileRead(VID_ASPECT_MATCH_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "ignore", 4))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
		}
		else if (!strncmp(buf, "letter box", 10))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX;
		}
		else if (!strncmp(buf, "pan scan", 8))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN;
		}
		else if (!strncmp(buf, "combined", 8))
		{
			dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_COMBINED;
		}
	}
#else
	if (AM_FileRead(VID_ASPECT_MATCH_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (sscanf(buf, "%d", &v) == 1)
		{
			switch (v)
			{
				case 0:
				case 1:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
					break;
				case 2:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN;
					break;
				case 3:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX;
					break;
				case 4:
					dev->video_match = AM_AV_VIDEO_ASPECT_MATCH_COMBINED;
					break;
			}
		}
	}
#endif
//#endif

#if !defined(ADEC_API_NEW)
	adec_cmd("stop");
	return AM_SUCCESS;
#else
	audio_decode_basic_init();
	return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev)
{
	UNUSED(dev);
	return AM_SUCCESS;
}

#if 0
typedef struct {
    unsigned int    format;  ///< video format, such as H264, MPEG2...
    unsigned int    width;   ///< video source width
    unsigned int    height;  ///< video source height
    unsigned int    rate;    ///< video source frame duration
    unsigned int    extra;   ///< extra data information of video stream
    unsigned int    status;  ///< status of video stream
    float		    ratio;   ///< aspect ratio of video source
    void *          param;   ///< other parameters for video decoder
} dec_sysinfo_t;
#endif
dec_sysinfo_t am_sysinfo;

#define ARC_FREQ_FILE "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"

static void set_arc_freq(int hi)
{
	static int old_freq = 200000;
	int freq;
	char buf[32];

	if (AM_FileRead(ARC_FREQ_FILE, buf, sizeof(buf)) == AM_SUCCESS) {
		sscanf(buf, "%d", &freq);
		if (hi) {
			old_freq = freq;
			snprintf(buf, sizeof(buf), "%d", 300000);
			AM_FileEcho(ARC_FREQ_FILE, buf);
		} else {
			snprintf(buf, sizeof(buf), "%d", old_freq);
			AM_FileEcho(ARC_FREQ_FILE, buf);
		}
	}
}

static AM_ErrorCode_t set_dec_control(AM_Bool_t enable)
{
	char v[32], vv[32];
	int dc=0;
	char *pch = v;

	static char *dec_control[] = {
		DEC_CONTROL_H264,
		DEC_CONTROL_MPEG12
	};
	int cnt = sizeof(dec_control)/sizeof(dec_control[0]);
	int i;
	if (enable) {//format: "0xaa|0xbb"
		property_get(DEC_CONTROL_PROP, v, "");
		for (i=0; i<cnt; i++) {
			dc = 0;
			if (!pch || !pch[0])
				break;
			int j = sscanf(pch, "%i", &dc);
			if (j) {
				snprintf(vv, 31, "%#x", dc);
				AM_FileEcho(dec_control[i], vv);
				AM_DEBUG(1, "dec control: %d==%s\n", i, vv);
			}
			pch = strchr(pch, '|');
			if (pch)
				pch++;
		}
	} else {
		for (i=0; i<cnt; i++)
			AM_FileEcho(dec_control[i], "0");
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_open_ts_mode(AM_AV_Device_t *dev)
{
	AV_TSData_t *ts;
	ts = (AV_TSData_t *)malloc(sizeof(AV_TSData_t));
	if (!ts)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_AV_ERR_NO_MEM;
	}

	memset(ts, 0, sizeof(*ts));
	ts->fd     = -1;
	ts->vid_fd = open(AMVIDEO_FILE, O_RDWR);

#ifndef MPTSONLYAUDIOVIDEO
	ts->fd = open(STREAM_TS_FILE, O_RDWR);
	if (ts->fd == -1)
	{
		AM_DEBUG(1, "cannot open \"/dev/amstream_mpts\" error:%d \"%s\"", errno, strerror(errno));
		free(ts);
		return AM_AV_ERR_CANNOT_OPEN_DEV;
	}
	AM_DEBUG(1, "try to init subtitle ring buf");
	int sid = 0xffff;
	if (ioctl(ts->fd, AMSTREAM_IOC_SID, sid) == -1)
	{
		AM_DEBUG(1, "set sub PID failed");
	}
#endif

	dev->ts_player.drv_data = (void*)ts;

	return AM_SUCCESS;
}

static int aml_get_audio_digital_raw(void)
{
	int mode;
	char buf[32];
	/*0:pcm 1:spdif 2:hdmi*/
	if (AM_FileRead(AUDIO_DSP_DIGITAL_RAW_FILE, buf, sizeof(buf)) >= 0) {
		sscanf(buf, "%d", &mode);
	} else {
		mode = 0;
	}
	return mode;
}

static int aml_set_sync_mode(int has_audio, int has_video, int afmt)
{
	int is_dts_dolby = 0;

	if (afmt == AFORMAT_AC3 ||
		afmt == AFORMAT_DTS ||
		afmt == AFORMAT_EAC3) {
		is_dts_dolby = 1;
	}
	if (aml_get_audio_digital_raw() == 0) {
		AM_FileEcho(TSYNC_MODE_FILE, "2");
	} else if (has_audio && is_dts_dolby) {
		AM_FileEcho(TSYNC_MODE_FILE, "1");
	} else if (has_video && !has_audio) {
		AM_FileEcho(TSYNC_MODE_FILE, "0");
	} else {
		AM_FileEcho(TSYNC_MODE_FILE, "2");
	}
	/*AM_FileEcho(TSYNC_MODE_FILE, "2");*/
	return 0;
}

static AM_ErrorCode_t aml_start_ts_mode(AM_AV_Device_t *dev, AV_TSPlayPara_t *tp, AM_Bool_t create_thread)
{
	AV_TSData_t *ts;
	int val;
	AM_Bool_t has_video = VALID_VIDEO(tp->vpid, tp->vfmt);
	AM_Bool_t has_audio = VALID_AUDIO(tp->apid, tp->afmt);
	AM_Bool_t ac3_amaster = AM_FALSE;

	AM_DEBUG(1, "aml start ts: V[%d:%d] A[%d:%d] P[%d]", tp->vpid, tp->vfmt, tp->apid, tp->afmt, tp->pcrpid);

	ts = (AV_TSData_t *)dev->ts_player.drv_data;

	/*patch dec control*/
	set_dec_control(has_video);

	if ((tp->afmt == AFORMAT_AC3) || (tp->afmt == AFORMAT_EAC3))
	{
		char buf[32];
		property_get(AC3_AMASTER_PROP, buf, "0");

		if (!strcmp(buf, "1"))
		{
			ac3_amaster = AM_TRUE;
		}
	}

	if (has_video && has_audio && !ac3_amaster)
	{
		AM_DEBUG(1, "%s, enable pcr_master", __FUNCTION__);
		gAVPcrEnable = AM_TRUE;
	}
	else
	{
		AM_DEBUG(1, "%s, disable pcr_master", __FUNCTION__);
		gAVPcrEnable = AM_FALSE;
	}

#ifndef ENABLE_PCR
	if (ts->vid_fd != -1){
		ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 1);
	}
#endif

#ifdef MPTSONLYAUDIOVIDEO
	if (ts->fd == -1)
	{

		AM_DEBUG(1, "aml_start_ts_mode used MPTSONLYAUDIOVIDEO %d %d %d %d", tp->vpid, has_video, tp->apid, has_audio);

		if (has_video && has_audio)
		{
			ts->fd = open(STREAM_TS_FILE, O_RDWR);
		}
		else if ((!has_video) && has_audio)
		{
			ts->fd = open(STREAM_TSONLYAUDIO_FILE, O_RDWR);
		}
		else if((!has_audio) && has_video)
		{
			ts->fd = open(STREAM_TSONLYVIDEO_FILE, O_RDWR);
		}
		else
		{
			ts->fd = open(STREAM_TS_FILE, O_RDWR);
		}

		if (ts->fd == -1)
		{
			AM_DEBUG(1, "cannot open \"/dev/amstream_mpts\" error:%d \"%s\"", errno, strerror(errno));
			return AM_AV_ERR_CANNOT_OPEN_DEV;
		}
	}
#endif


#if defined(ANDROID) || defined(CHIP_8626X)
	/*Set tsync enable/disable*/
	if (has_video && has_audio)
	{
		AM_DEBUG(1, "Set tsync enable to 1");
		aml_set_tsync_enable(1);
	}
	else
	{
		AM_DEBUG(1, "Set tsync enable to 0");
		aml_set_tsync_enable(0);
	}
#endif



	if (has_video) {
		val = tp->vfmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_VFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set video format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->vpid;
		if (ioctl(ts->fd, AMSTREAM_IOC_VID, val) == -1)
		{
			AM_DEBUG(1, "set video PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	/*if ((tp->vfmt == VFORMAT_H264) || (tp->vfmt == VFORMAT_VC1))*/
	if (has_video) {

		memset(&am_sysinfo,0,sizeof(dec_sysinfo_t));
		if (tp->vfmt == VFORMAT_VC1)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_H264)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
			am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;
		}
		else if (tp->vfmt == VFORMAT_AVS)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_AVS;
			/*am_sysinfo.width  = 1920;
			am_sysinfo.height = 1080;*/
		}
		else if (tp->vfmt == VFORMAT_HEVC)
		{
			am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
			am_sysinfo.width  = 3840;
			am_sysinfo.height = 2160;
		}

		if (ioctl(ts->fd, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo) == -1)
		{
			AM_DEBUG(1, "set AMSTREAM_IOC_SYSINFO");
			return AM_AV_ERR_SYS;
		}
	}

	if (has_audio) {
		if ((tp->afmt == AFORMAT_AC3) || (tp->afmt == AFORMAT_DTS)) {
			//set_arc_freq(1);
		}

		val = tp->afmt;
		if (ioctl(ts->fd, AMSTREAM_IOC_AFORMAT, val) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		val = tp->apid;
		if (ioctl(ts->fd, AMSTREAM_IOC_AID, val) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

//#ifdef ENABLE_PCR
	if (gAVPcrEnable == AM_TRUE) {
		if (tp->pcrpid && (tp->pcrpid < 0x1fff)) {
			val = tp->pcrpid;
			if (ioctl(ts->fd, AMSTREAM_IOC_PCRID, val) == -1)
			{
				AM_DEBUG(1, "set PCR PID failed");
				return AM_AV_ERR_SYS;
			}
		}
		aml_set_sync_mode(has_audio, has_video, tp->afmt);
		/*AM_FileEcho(TSYNC_MODE_FILE, "2");*/
	}

	if (has_audio && !ac3_amaster) {
		if (!show_first_frame_nosync()) {
			property_set("sys.amplayer.drop_pcm", "1");
		}
		AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");

		adec_start_decode(ts->fd, tp->afmt, has_video, &ts->adec);

		if (VALID_PID(tp->sub_apid))
			aml_set_audio_ad(dev, 1, tp->sub_apid, tp->sub_afmt);
	}
//#endif /*ENABLE_PCR*/

	if (ioctl(ts->fd, AMSTREAM_IOC_PORT_INIT, 0) == -1)
	{
		AM_DEBUG(1, "amport init failed");
		return AM_AV_ERR_SYS;
	}
	AM_DEBUG(1, "set ts skipbyte to 0");
	if (ioctl(ts->fd, AMSTREAM_IOC_TS_SKIPBYTE, 0) == -1)
	{
		AM_DEBUG(1, "set ts skipbyte failed");
		return AM_AV_ERR_SYS;
	}

	AM_TIME_GetClock(&dev->ts_player.av_start_time);

	/*创建AV监控线程*/
	if (create_thread)
	{
		dev->ts_player.av_thread_running = AM_TRUE;
		if (pthread_create(&dev->ts_player.av_mon_thread, NULL, aml_av_monitor_thread, (void*)dev))
		{
			AM_DEBUG(1, "create the av buf monitor thread failed");
			dev->ts_player.av_thread_running = AM_FALSE;
		}
	}
	dev->ts_player.play_para = *tp;

	return AM_SUCCESS;
}

static int aml_close_ts_mode(AM_AV_Device_t *dev, AM_Bool_t destroy_thread)
{
	AV_TSData_t *ts;
	int fd;

	if (destroy_thread && dev->ts_player.av_thread_running)
	{
		dev->ts_player.av_thread_running = AM_FALSE;
		pthread_cond_broadcast(&gAVMonCond);
		AM_DEBUG(1, "TV aml_close_ts_mode---broardcast end join start\r\n");
		pthread_join(dev->ts_player.av_mon_thread, NULL);
		AM_DEBUG(1, "TV aml_close_ts_mode---join end\r\n");
	}


	ts = (AV_TSData_t*)dev->ts_player.drv_data;
	if (ts->fd != -1)
		close(ts->fd);
	if (ts->vid_fd != -1)
		close(ts->vid_fd);

	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	adec_set_decode_ad(0, 0, 0, ts->adec);
	adec_stop_decode(&ts->adec);

	free(ts);

	dev->ts_player.drv_data = NULL;

//#ifdef ENABLE_PCR
	property_set("sys.amplayer.drop_pcm", "0");
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
	if (gAVPcrEnable == AM_TRUE)
	{
		AM_FileEcho(TSYNC_MODE_FILE, "0");
	}
//#endif /*ENABLE_PCR*/

	//set_arc_freq(0);

	/*unpatch dec control*/
	set_dec_control(AM_FALSE);

	return 0;
}

/**\brief 读取PTS值*/
static AM_ErrorCode_t aml_get_pts(const char *class_file,  uint32_t *pts)
{
	char buf[32];

	if (AM_FileRead(class_file, buf, sizeof(buf)) == AM_SUCCESS)
	{
		*pts = (uint32_t)atol(buf);
		return AM_SUCCESS;
	}

	return AM_FAILURE;
}

int am_av_restart_pts_repeat_count = 2;

/**\brief AV buffer 监控线程*/
static void* aml_av_monitor_thread(void *arg)
{
	AM_AV_Device_t *dev = (AM_AV_Device_t *)arg;
	AM_Bool_t adec_start = AM_FALSE;
	AM_Bool_t av_paused = AM_TRUE;
	AM_Bool_t has_audio = VALID_AUDIO(dev->ts_player.play_para.apid, dev->ts_player.play_para.afmt);
	AM_Bool_t has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);
	AM_Bool_t bypass_di = AM_FALSE;
	AM_Bool_t drop_b_frame = AM_FALSE;
	AM_Bool_t is_hd_video = AM_FALSE;
	AM_Bool_t audio_scrambled = AM_FALSE;
	AM_Bool_t video_scrambled = AM_FALSE;
	AM_Bool_t no_audio_data = AM_TRUE, no_video_data = AM_TRUE;
	AM_Bool_t no_video = AM_TRUE;
	AM_Bool_t has_amaster = AM_FALSE;
	AM_Bool_t need_replay;
	int resample_type = 0;
	int next_resample_type = resample_type;
	int now, next_resample_start_time = 0;
	int abuf_level, vbuf_level;
	int abuf_size, vbuf_size;
	unsigned int abuf_read_ptr, vbuf_read_ptr;
	unsigned int last_abuf_read_ptr = 0, last_vbuf_read_ptr = 0;
	int arp_stop_time = 0, vrp_stop_time = 0;
	int arp_stop_dur = 0, vrp_stop_dur = 0;
	int apts, vpts, last_apts = 0, last_vpts = 0;
	int dmx_apts, dmx_vpts, last_dmx_apts = 0, last_dmx_vpts = 0;
	int apts_stop_time = 0, vpts_stop_time = 0, apts_stop_dur = 0, vpts_stop_dur = 0;
	int dmx_apts_stop_time = 0, dmx_vpts_stop_time = 0, dmx_apts_stop_dur = 0, dmx_vpts_stop_dur = 0;
	int tsync_mode, vmaster_time = 0, vmaster_dur = 0;
	int abuf_level_empty_time = 0, abuf_level_empty_dur = 0, vbuf_level_empty_time = 0, vbuf_level_empty_dur = 0;
	int down_audio_cache_time = 0, down_video_cache_time = 0;
	struct am_io_param astatus;
	struct am_io_param vstatus;
	int vdec_status, frame_width, frame_height;
	struct timespec rt;
	char buf[32];
	AV_TSData_t *ts;
	AM_Bool_t is_avs_plus = AM_FALSE;
	int avs_fmt = 0;

	unsigned int vframes_now = 0, vframes_last = 0;

#ifndef ENABLE_PCR
	if (!show_first_frame_nosync()) {
		property_set("sys.amplayer.drop_pcm", "1");
	}
#else
	av_paused  = AM_FALSE;
	adec_start = (adec_handle != NULL);
#endif

	AM_FileEcho(VID_BLACKOUT_FILE, "0");
	AM_FileEcho(VDEC_H264_FATAL_ERROR_RESET_FILE, "1");

	pthread_mutex_lock(&gAVMonLock);

	while (dev->ts_player.av_thread_running) {
		ts = (AV_TSData_t *)dev->ts_player.drv_data;

		if (!adec_start || (has_video && no_video))
			AM_TIME_GetTimeSpecTimeout(20, &rt);
		else
			AM_TIME_GetTimeSpecTimeout(200, &rt);

		pthread_cond_timedwait(&gAVMonCond, &gAVMonLock, &rt);
		if (! dev->ts_player.av_thread_running)
			break;
		//switch audio pid or fmt
		if (dev->audio_switch == AM_TRUE)
		{
			aml_switch_ts_audio_fmt(dev);
			dev->audio_switch = AM_FALSE;
		}

		AM_TIME_GetClock(&now);

		if (ioctl(ts->fd, AMSTREAM_IOC_AB_STATUS, (unsigned long)&astatus) != -1) {
			abuf_size  = astatus.status.size;
			abuf_level = astatus.status.data_len;
			abuf_read_ptr = astatus.status.read_pointer;
		} else {
			//AM_DEBUG(1, "cannot get audio buffer status");
			abuf_size  = 0;
			abuf_level = 0;
			abuf_read_ptr = 0;
		}

		if (ioctl(ts->fd, AMSTREAM_IOC_VB_STATUS, (unsigned long)&vstatus) != -1) {
			vbuf_size  = vstatus.status.size;
			vbuf_level = vstatus.status.data_len;
			vbuf_read_ptr = vstatus.status.read_pointer;
			//is_hd_video = vstatus.vstatus.width > 720;
		} else {
			//AM_DEBUG(1, "cannot get video buffer status");
			vbuf_size  = 0;
			vbuf_level = 0;
			vbuf_read_ptr = 0;
		}

		if (vbuf_level == 0) {
			if(!vbuf_level_empty_time)
				vbuf_level_empty_time = now;
			vbuf_level_empty_dur = now - vbuf_level_empty_time;
		} else {
			vbuf_level_empty_time = 0;
			vbuf_level_empty_dur = 0;
		}
		if (abuf_level == 0) {
			if (!abuf_level_empty_time)
				abuf_level_empty_time = now;
			abuf_level_empty_dur = now - abuf_level_empty_time;
		} else {
			abuf_level_empty_time = 0;
			abuf_level_empty_dur = 0;
		}
		if (abuf_read_ptr == last_abuf_read_ptr) {
			if (!arp_stop_time)
				arp_stop_time = now;
			arp_stop_dur = now - arp_stop_time;
		} else {
			arp_stop_time = 0;
			arp_stop_dur  = 0;
		}
		last_abuf_read_ptr = abuf_read_ptr;

		if (vbuf_read_ptr == last_vbuf_read_ptr) {
			if(!vrp_stop_time)
				vrp_stop_time = now;
			vrp_stop_dur = now - vrp_stop_time;
		} else {
			vrp_stop_time = 0;
			vrp_stop_dur  = 0;
		}
		last_vbuf_read_ptr = vbuf_read_ptr;

		memset(&vstatus, 0, sizeof(vstatus));
		if (ioctl(ts->fd, AMSTREAM_IOC_VDECSTAT, (unsigned long)&vstatus) != -1) {
			is_hd_video = (vstatus.vstatus.width > 720)? 1 : 0;
			vdec_status = vstatus.vstatus.status;
			frame_width = vstatus.vstatus.width;
			frame_height= vstatus.vstatus.height;
			//AM_DEBUG(1, "vdec width %d height %d status 0x%08x", frame_width, frame_height, vdec_status);
		} else {
			vdec_status = 0;
			frame_width = 0;
			frame_height= 0;
		}
		if (AM_FileRead(AVS_PLUS_DECT_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%d", &avs_fmt);
		} else {
			//AM_DEBUG(1, "cannot read \"%s\"", AVS_PLUS_DECT_FILE);
			avs_fmt = 0;
		}
		//AM_DEBUG(1, "avs_fmt: \"%x\"", avs_fmt);
		if (avs_fmt == 0x148) //bit8
			is_avs_plus = AM_TRUE;
		else
			is_avs_plus = AM_FALSE;

		if (AM_FileRead(AUDIO_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf+2, "%x", &apts);
		} else {
			AM_DEBUG(1, "cannot read \"%s\"", AUDIO_PTS_FILE);
			apts = 0;
		}

		if (AM_FileRead(VIDEO_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf+2, "%x", &vpts);
		} else {
			AM_DEBUG(1, "cannot read \"%s\"", VIDEO_PTS_FILE);
			vpts = 0;
		}

		if (AM_FileRead(TSYNC_MODE_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%d", &tsync_mode);
		} else {
			tsync_mode = 1;
		}

		if (tsync_mode == 0) {
			if (vmaster_time == 0) {
				vmaster_time = now;
			}
			vmaster_dur = now - vmaster_time;
		} else {
			vmaster_time = 0;
			vmaster_dur = 0;
			has_amaster = AM_TRUE;
		}

		if (AM_FileRead(AUDIO_DMX_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%d", &dmx_apts);
		} else {
			AM_DEBUG(1, "cannot read \"%s\"", AUDIO_DMX_PTS_FILE);
			dmx_apts = 0;
		}

		if (AM_FileRead(VIDEO_DMX_PTS_FILE, buf, sizeof(buf)) >= 0) {
			sscanf(buf, "%d", &dmx_vpts);
		} else {
			AM_DEBUG(1, "cannot read \"%s\"", VIDEO_DMX_PTS_FILE);
			dmx_vpts = 0;
		}

		if (apts == last_apts) {
			if (!apts_stop_time)
				apts_stop_time = now;
			apts_stop_dur = now - apts_stop_time;
		} else {
			last_apts = apts;
			apts_stop_time = 0;
			apts_stop_dur = 0;
			apts_stop_time = 0;
		}

		if (vpts == last_vpts) {
			if(!vpts_stop_time)
				vpts_stop_time = now;
			vpts_stop_dur = now - vpts_stop_time;
		} else {
			last_vpts = vpts;
			vpts_stop_time = 0;
			vpts_stop_dur = 0;
			vpts_stop_time = 0;
		}

		if (dmx_apts == last_dmx_apts) {
			if(!dmx_apts_stop_time)
				dmx_apts_stop_time = now;
			dmx_apts_stop_dur = now - dmx_apts_stop_time;
		} else {
			last_dmx_apts = dmx_apts;
			dmx_apts_stop_dur = 0;
			dmx_apts_stop_time = 0;
		}

		if (dmx_vpts == last_dmx_vpts) {
			if (!dmx_vpts_stop_time)
				dmx_vpts_stop_time = now;
			dmx_vpts_stop_dur = now - dmx_vpts_stop_time;
		} else {
			last_dmx_vpts = dmx_vpts;
			dmx_vpts_stop_dur = 0;
			dmx_vpts_stop_time = 0;
		}

#if 0
		AM_DEBUG(1, "audio level:%d cache:%d, video level:%d cache:%d, resample:%d",
				abuf_level, adec_start ? dmx_apts - apts : dmx_apts - first_dmx_apts,
				vbuf_level, vpts ? dmx_vpts - vpts : dmx_vpts - first_dmx_vpts,
				resample_type);
#endif

#ifndef ENABLE_PCR
		if (has_audio && !adec_start) {
			adec_start = AM_TRUE;

			if (abuf_level < ADEC_START_AUDIO_LEVEL)
				adec_start = AM_FALSE;

			if (has_video) {
				if (vbuf_level < ADEC_START_VIDEO_LEVEL) {
					adec_start = AM_FALSE;
				}
			}

			if (abuf_level >= ADEC_FORCE_START_AUDIO_LEVEL)
				adec_start = AM_TRUE;

			if (adec_start) {
				audio_info_t info;

				/*Set audio info*/
				memset(&info, 0, sizeof(info));
				info.valid  = 1;
				ioctl(ts->fd, AMSTREAM_IOC_AUDIO_INFO, (unsigned long)&info);

				adec_start_decode(ts->fd, dev->ts_player.play_para.afmt, has_video, &ts->adec);

				if (av_paused) {
					audio_decode_pause(ts->adec);
				}

				audio_scrambled = AM_FALSE;
				video_scrambled = AM_FALSE;
				resample_type = 0;
				next_resample_type = resample_type;
				next_resample_start_time = 0;
				down_audio_cache_time = 0;
				down_video_cache_time = 0;
				AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
				AM_FileEcho(RESAMPLE_TYPE_FILE, "0");

				AM_DEBUG(1, "start audio decoder vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (!av_paused) {
			if (has_video && (vbuf_level < DEC_STOP_VIDEO_LEVEL))
				av_paused = AM_TRUE;
			if (has_audio && adec_start && (abuf_level < DEC_STOP_AUDIO_LEVEL))
				av_paused = AM_TRUE;

			if (av_paused) {
				if (has_audio && adec_start) {
					audio_decode_pause(ts->adec);
				}
				if (has_video) {
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 1);
				}

				AM_DEBUG(1, "pause av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (av_paused) {
			av_paused = AM_FALSE;

			if (has_video && (vbuf_level < ADEC_START_VIDEO_LEVEL))
				av_paused = AM_TRUE;
			if (has_audio && (abuf_level < ADEC_START_AUDIO_LEVEL))
				av_paused = AM_TRUE;

			if (!av_paused) {
				if (has_audio && adec_start) {
					audio_decode_resume(ts->adec);
				}
				if (has_video) {
					ioctl(ts->vid_fd, AMSTREAM_IOC_VPAUSE, 0);
				}
				apts_stop_time = 0;
				vpts_stop_time = 0;
				resample_type = 0;
				next_resample_type = resample_type;
				next_resample_start_time = 0;
				down_audio_cache_time = 0;
				down_video_cache_time = 0;
				AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
				AM_FileEcho(RESAMPLE_TYPE_FILE, "0");
				AM_DEBUG(1, "resume av play vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}

		if (has_audio && adec_start && !av_paused) {
			AM_Bool_t af = AM_FALSE, vf = AM_FALSE;
			int resample = 0;

			if (has_audio && (abuf_level < UP_RESAMPLE_AUDIO_LEVEL))
				resample = 2;
			if (has_video && (vbuf_level < UP_RESAMPLE_VIDEO_LEVEL))
				resample = 2;

			if (has_audio && dmx_apts && apts) {
				if (down_audio_cache_time == 0) {
					down_audio_cache_time = dmx_apts - apts;
					if (down_audio_cache_time < DOWN_RESAMPLE_CACHE_TIME)
						down_audio_cache_time = DOWN_RESAMPLE_CACHE_TIME;
					else
						down_audio_cache_time *= 2;
				}
				if (has_audio && (dmx_apts - apts > down_audio_cache_time))
					af = AM_TRUE;
			}

			if (has_video && dmx_vpts && vpts) {
				if (down_video_cache_time == 0) {
					down_video_cache_time = dmx_vpts - vpts;
					if (down_video_cache_time < DOWN_RESAMPLE_CACHE_TIME)
						down_video_cache_time = DOWN_RESAMPLE_CACHE_TIME;
					else
						down_video_cache_time *= 2;
				}
				if (has_video && (dmx_vpts - vpts > down_video_cache_time))
					vf = AM_TRUE;
			}

			if (af && vf)
				resample = 1;

			if (has_audio && (abuf_level * 5 > abuf_size * 4))
				resample = 1;

			if (has_video && (vbuf_level * 5 > vbuf_size * 4))
				resample = 1;

#ifdef ENABLE_AUDIO_RESAMPLE
			if (resample != resample_type) {
				if (resample != next_resample_type) {
					next_resample_type = resample;
					next_resample_start_time = now;
				}

				if (now - next_resample_start_time > 500) {
					const char *cmd = "";

					switch (resample) {
						case 1: cmd = "1"; break;
						case 2: cmd = "2"; break;
						default: cmd = "0"; break;
					}
#ifdef ENABLE_AUDIO_RESAMPLE
					AM_FileEcho(ENABLE_RESAMPLE_FILE, resample?"1":"0");
					AM_FileEcho(RESAMPLE_TYPE_FILE, cmd);
					AM_DEBUG(1, "audio resample %d vlevel %d alevel %d",
						resample, vbuf_level, abuf_level);
					resample_type = resample;
#endif
					next_resample_start_time = 0;
				}
			}
#endif
		}

#else /*defined(ENABLE_PCR)*/
		if (has_audio && !adec_start) {
			adec_start = AM_TRUE;

			if (abuf_level < ADEC_START_AUDIO_LEVEL)
				adec_start = AM_FALSE;

			if (has_video) {
				if (vbuf_level < ADEC_START_VIDEO_LEVEL) {
					adec_start = AM_FALSE;
				}
			}

			if (abuf_level >= ADEC_FORCE_START_AUDIO_LEVEL)
				adec_start = AM_TRUE;

			if (adec_start) {
				adec_start_decode(ts->fd, dev->ts_player.play_para.afmt, has_video, &ts->adec);
				AM_DEBUG(1, "start audio decoder vlevel %d alevel %d", vbuf_level, abuf_level);
			}
		}
#endif /*!defined ENABLE_PCR*/

		int status = audio_decoder_get_enable_status(ts->adec);
		if (status == 1) {
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_AC3_LICENCE_RESUME, NULL);
		}
		else if (status == 0) {
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_AC3_NO_LICENCE, NULL);
		}
		else if (status == -1) {

		}

#ifdef ENABLE_BYPASS_DI
		if (has_video && is_hd_video && !bypass_di && (vbuf_level * 6 > vbuf_size * 5)) {
			AM_FileEcho(DI_BYPASS_FILE, "1");
			bypass_di = AM_TRUE;
			AM_DEBUG(1, "bypass HD deinterlace vlevel %d", vbuf_level);
		}
#endif

#ifdef ENABLE_DROP_BFRAME
		if (has_video && is_hd_video && !drop_b_frame) {
			if (vbuf_level * 6 > vbuf_size * 5) {
				drop_b_frame = AM_TRUE;
			}

			if (has_audio && adec_start && has_amaster && AM_ABS(apts - vpts) > 45000) {
				drop_b_frame = AM_TRUE;
			}

			if (drop_b_frame) {
				AM_FileEcho(VIDEO_DROP_BFRAME_FILE ,"1");
				AM_DEBUG(1, "drop B frame vlevel %d", vbuf_level);
			}
		}
#endif

		//check video frame available

		if (has_video) {
			if (AM_FileRead(VIDEO_NEW_FRAME_COUNT_FILE, buf, sizeof(buf)) >= 0) {
				sscanf(buf, "%i", &vframes_now);

				if ((vframes_now >= VIDEO_AVAILABLE_MIN_CNT) && (vframes_now > vframes_last)) {
					if (no_video) {
						AM_DEBUG(1, "video available");
						AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_AVAILABLE, NULL);
					}
					no_video = AM_FALSE;
				} else {
					no_video = AM_TRUE;
				}

				vframes_last = vframes_now;
			} else {
				AM_DEBUG(1, "cannot read \"%s\"", VIDEO_NEW_FRAME_COUNT_FILE);
				vframes_now = 0;
			}
		}

		//first no_data
		if (has_audio && adec_start &&  (dmx_apts_stop_dur > NO_DATA_CHECK_TIME) && (apts_stop_dur > NO_DATA_CHECK_TIME)) {
			AM_Bool_t sf[2];
			AM_DEBUG(1, "audio stoped");
			no_audio_data = AM_TRUE;
			//second scramble
			if (abuf_level_empty_dur > SCRAMBLE_CHECK_TIME) {
				AM_DMX_GetScrambleStatus(0, sf);
				if (sf[1]) {
					audio_scrambled = AM_TRUE;
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AUDIO_SCAMBLED, NULL);
					AM_DEBUG(1, "audio scrambled > stoped");
				} else {
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
				}
			} else {
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
			}
		}

		if (has_video && (dmx_vpts_stop_dur > NO_DATA_CHECK_TIME) && (vpts_stop_dur > NO_DATA_CHECK_TIME)) {
			AM_Bool_t sf[2];
			no_video_data = AM_TRUE;
			AM_DEBUG(1, "video data stopped");
			if (vbuf_level_empty_dur > SCRAMBLE_CHECK_TIME) {
				AM_DMX_GetScrambleStatus(0, sf);
				if (sf[0]) {
					video_scrambled = AM_TRUE;
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_SCAMBLED, NULL);
					AM_DEBUG(1, "video scrambled");
				} else {
					AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
				}
			} else {
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_NO_DATA, NULL);
			}
		} else if (is_avs_plus &&(dev->ts_player.play_para.vfmt == VFORMAT_AVS)) {
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_NOT_SUPPORT, NULL);//not  unsupport , just  FORMAT  is AVS
		}

		//AM_DEBUG(1,"no_audio = %d, dmx_a_stop = %d, a_stop= %d, no_video=%d, dmx_v_stop=%d, v_stop=%d, abuf_empty=%d, vbuf_empty=%d\n",no_audio_data,dmx_apts_stop_dur,apts_stop_dur, no_video_data, dmx_vpts_stop_dur, vpts_stop_dur, abuf_level_empty_dur,vbuf_level_empty_dur);

		if (no_audio_data && dmx_apts_stop_dur == 0) {
			no_audio_data = AM_FALSE;
			audio_scrambled = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_DATA_RESUME, NULL);
			AM_DEBUG(1, "audio data resumed");
		}

		if (no_video_data && dmx_vpts_stop_dur == 0) {
			no_video_data = AM_FALSE;
			video_scrambled = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_AV_DATA_RESUME, NULL);
			AM_DEBUG(1, "video data resumed");
		}

		/*AM_DEBUG(1, "apts_dmx_stop: %d arp_stop: %d vpts_dmx_stop: %d vrp_stop: %d",
					dmx_apts_stop_dur, arp_stop_dur, dmx_vpts_stop_dur, vrp_stop_dur);*/
		need_replay = AM_FALSE;
		if (!no_video_data && !av_paused && (dmx_vpts_stop_dur == 0) && (vrp_stop_dur > NO_DATA_CHECK_TIME))
		{
			need_replay = AM_TRUE;
			AM_DEBUG(1, "apts_dmx_stop: %d arp_stop: %d vpts_dmx_stop: %d vrp_stop: %d",
					dmx_apts_stop_dur, arp_stop_dur, dmx_vpts_stop_dur, vrp_stop_dur);
		}
		if (vbuf_level * 6 > vbuf_size * 5)
		{
			need_replay = AM_TRUE;
			AM_DEBUG(1, "1 replay ts vlevel %d vbuf_size %d",vbuf_level*6, vbuf_size*5);
		}
		if (abuf_level * 6 > abuf_size * 5)
		{
			need_replay = AM_TRUE;
			AM_DEBUG(1, "2 replay ts vlevel %d vbuf_size %d",abuf_level*6, abuf_size*5);
		}
		//if(adec_start && !av_paused && has_amaster && !apts_stop_dur && !vpts_stop_dur && (vmaster_dur > VMASTER_REPLAY_TIME))
			//need_replay = AM_TRUE;
		//AM_DEBUG(0, "vdec status %08x", vdec_status);
#ifdef DECODER_FATAL_ERROR_SIZE_OVERFLOW
		if (has_video && (dev->ts_player.play_para.vfmt == VFORMAT_H264) && (vdec_status & DECODER_FATAL_ERROR_SIZE_OVERFLOW)) {
			AM_DEBUG(1, "switch to h264 4K/2K");
			dev->ts_player.play_para.vfmt = VFORMAT_H264_4K2K;
			need_replay = AM_TRUE;
		} else
#endif
		if (has_video && (dev->ts_player.play_para.vfmt == VFORMAT_H264) && ((vdec_status >> 16) & 0xFFFF)) {
			AM_DEBUG(1, "H264 fatal error");
			need_replay = AM_TRUE;
		}

		if (AM_FileRead("/sys/class/tsync_pcr/tsync_pcr_reset_flag", buf, sizeof(buf)) >= 0) {
			int val = 0;
			sscanf(buf, "%d", &val);
			if (val == 1) {
				need_replay = AM_TRUE;
				AM_DEBUG(1, "pcr need reset");
			}
		}

		//AM_DEBUG(1, "vbuf_level--0x%08x---- abuf_level---0x%08x",vbuf_level,abuf_level);

		if (need_replay) {
			AM_DEBUG(1, "replay ts vlevel %d alevel %d vpts_stop %d vmaster %d",
				vbuf_level, abuf_level, vpts_stop_dur, vmaster_dur);
			aml_close_ts_mode(dev, AM_FALSE);
			aml_open_ts_mode(dev);
			aml_start_ts_mode(dev, &dev->ts_player.play_para, AM_FALSE);
#ifndef ENABLE_PCR
			adec_start = AM_FALSE;
			av_paused  = AM_TRUE;
#else
			adec_start = AM_TRUE;
			av_paused  = AM_FALSE;
#endif
			resample_type = 0;
			next_resample_type = resample_type;
			next_resample_start_time = 0;
			last_apts = 0;
			last_vpts = 0;
			last_dmx_apts = 0;
			last_dmx_vpts = 0;
			apts_stop_time = 0;
			vpts_stop_time = 0;
			dmx_apts_stop_time = 0;
			dmx_vpts_stop_time = 0;
			vmaster_time = 0;
			down_audio_cache_time = 0;
			down_video_cache_time = 0;
			has_amaster = AM_FALSE;
		}
	}

	pthread_mutex_unlock(&gAVMonLock);

#ifndef ENABLE_PCR
	if (resample_type) {
		AM_FileEcho(ENABLE_RESAMPLE_FILE, "0");
	}
#endif

	AM_FileEcho(VDEC_H264_FATAL_ERROR_RESET_FILE, "0");
	AM_FileEcho(VID_BLACKOUT_FILE, dev->video_blackout ? "1" : "0");

	if (bypass_di) {
		AM_FileEcho(DI_BYPASS_FILE, "0");
	}

	if (drop_b_frame) {
		AM_FileEcho(VIDEO_DROP_BFRAME_FILE, "0");
	}

	AM_DEBUG(1, "AV  monitor thread exit now");
	return NULL;
}

static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
#endif
	AV_JPEGData_t *jpeg;
	AV_InjectData_t *inj;
	AV_TimeshiftData_t *tshift;
	AV_TSData_t *ts;
	int fd;

	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = aml_create_data_source(STREAM_VBUF_FILE, dev->dev_no, AM_FALSE);
			if (!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_vbuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}

			fd = open(AMVIDEO_FILE, O_RDWR);
			if (fd == -1)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amvideo\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			ioctl(fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_I);
			src->video_fd = fd;
			dev->vid_player.drv_data = src;

		break;
		case AV_PLAY_AUDIO_ES:
			src = aml_create_data_source(STREAM_ABUF_FILE, dev->dev_no, AM_TRUE);
			if (!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_abuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			dev->aud_player.drv_data = src;
		break;
		case AV_PLAY_TS:
			if (aml_open_ts_mode(dev) != AM_SUCCESS)
				return AM_AV_ERR_SYS;
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = aml_create_fp(dev);
			if (!data)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->file_player.drv_data = data;
#endif
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = aml_create_jpeg_data();
			if (!jpeg)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->vid_player.drv_data = jpeg;
		break;
		case AV_INJECT:
			inj = aml_create_inject_data();
			if (!inj)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->inject_player.drv_data = inj;
		break;
		case AV_TIMESHIFT:
			tshift = aml_create_timeshift_data();
			if (!tshift)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			tshift->dev = dev;
			dev->timeshift_player.drv_data = tshift;
		break;
		default:
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para)
{
	AV_DataSource_t *src;
	int fd, val;
	AV_TSPlayPara_t *tp;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	AV_FilePlayPara_t *pp;
#endif
	AV_JPEGData_t *jpeg;
	AV_InjectPlayPara_t *inj_p;
	AV_InjectData_t *inj;
	AV_TimeShiftPlayPara_t *tshift_p;
	AV_TimeshiftData_t *tshift;

	int ctrl_fd = open("/dev/amvideo", O_RDWR);
	if (ctrl_fd >= 0) {
		ioctl(ctrl_fd, AMSTREAM_IOC_SET_VSYNC_UPINT, 0);
		close(ctrl_fd);
	} else {
		AM_DEBUG(1, "open /dev/amvideo error");
	}

	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			val = (long)para;
			if (ioctl(src->fd, AMSTREAM_IOC_VFORMAT, val) == -1)
			{
				AM_DEBUG(1, "set video format failed");
				return AM_AV_ERR_SYS;
			}
			aml_start_data_source(src, dev->vid_player.para.data, dev->vid_player.para.len, dev->vid_player.para.times+1);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			val = (long)para;
			if (ioctl(src->fd, AMSTREAM_IOC_AFORMAT, val) == -1)
			{
				AM_DEBUG(1, "set audio format failed");
				return AM_AV_ERR_SYS;
			}
			AM_DEBUG(1, "aml start aes:  A[fmt:%d, loop:%d]", val, dev->aud_player.para.times);
			aml_start_data_source(src, dev->aud_player.para.data, dev->aud_player.para.len, dev->aud_player.para.times);
			adec_start_decode(src->fd, val, 0, &src->adec);
		break;
		case AV_PLAY_TS:
			tp = (AV_TSPlayPara_t *)para;
#if defined(ANDROID) || defined(CHIP_8626X)
#ifdef ENABLE_PCR_RECOVER
			AM_FileEcho(PCR_RECOVER_FILE, "1");
#endif
#endif
			AM_TRY(aml_start_ts_mode(dev, tp, AM_TRUE));
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
			pp = (AV_FilePlayPara_t*)para;

			if (pp->start)
			{
				memset((void*)&data->pctl,0,sizeof(play_control_t));

				//player_register_update_callback(&data->pctl.callback_fn, aml_update_player_info_callback, PLAYER_INFO_POP_INTERVAL);

				data->pctl.file_name = strndup(pp->name,FILENAME_LENGTH_MAX);
				data->pctl.video_index = -1;
				data->pctl.audio_index = -1;
				data->pctl.hassub = 1;

				/*data->pctl.t_pos = st;*/
				aml_set_tsync_enable(1);

				if (pp->loop)
				{
					data->pctl.loop_mode =1;
				}
				data->pctl.need_start = 1;
				data->pctl.is_variable = 1;

				data->media_id = player_start(&data->pctl,0);

				if (data->media_id < 0)
				{
					AM_DEBUG(1, "play file failed");
					return AM_AV_ERR_SYS;
				}

				player_start_play(data->media_id);
				//AM_AOUT_SetDriver(AOUT_DEV_NO, &amplayer_aout_drv, (void*)(long)data->media_id);
			}
#endif
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = dev->vid_player.drv_data;
			return aml_decode_jpeg(jpeg, dev->vid_player.para.data, dev->vid_player.para.len, mode, para);
		break;
		case AV_INJECT:
			inj_p = (AV_InjectPlayPara_t *)para;
			inj = dev->inject_player.drv_data;
			if (aml_start_inject(inj, inj_p) != AM_SUCCESS)
				return AM_AV_ERR_SYS;
		break;
		case AV_TIMESHIFT:
			tshift_p = (AV_TimeShiftPlayPara_t *)para;
			tshift = dev->timeshift_player.drv_data;
			m_tshift = tshift;
			if (tshift_p->para.media_info.aud_cnt > 0) {
				tshift->aud_idx = 0;
				tshift->aud_pid = tshift_p->para.media_info.audios[0].pid;
				tshift->aud_fmt = tshift_p->para.media_info.audios[0].fmt;
				tshift->aud_valid = VALID_AUDIO(tshift->aud_pid, tshift->aud_fmt);
			}
			if (aml_start_timeshift(tshift, tshift_p, AM_TRUE, AM_TRUE) != AM_SUCCESS)
				return AM_AV_ERR_SYS;
		break;
		default:
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
#endif
	AV_JPEGData_t *jpeg;
	AV_InjectData_t *inj;
	AV_TimeshiftData_t *tshift;
	int fd, ret;

	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			ioctl(src->video_fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			close(src->video_fd);
			aml_destroy_data_source(src);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			adec_stop_decode(&src->adec);
			aml_destroy_data_source(src);
		break;
		case AV_PLAY_TS:
#if defined(ANDROID) || defined(CHIP_8626X)
#ifdef ENABLE_PCR_RECOVER
			AM_FileEcho(PCR_RECOVER_FILE, "0");
#endif
#endif
		ret = aml_close_ts_mode(dev, AM_TRUE);
		break;
		case AV_PLAY_FILE:
#ifdef PLAYER_API_NEW
			data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
			aml_destroy_fp(data);
			adec_cmd("stop");
#endif
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = dev->vid_player.drv_data;
			aml_destroy_jpeg_data(jpeg);
		break;
		case AV_INJECT:
			inj = dev->inject_player.drv_data;
			aml_destroy_inject_data(inj);
		break;
		case AV_TIMESHIFT:
			tshift = dev->timeshift_player.drv_data;
			aml_destroy_timeshift_data(tshift, AM_TRUE);
			dev->timeshift_player.drv_data = NULL;
			m_tshift = NULL;
		break;
		default:
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src)
{
	char *cmd;

	UNUSED(dev);

	switch (src)
	{
		case AM_AV_TS_SRC_TS0:
			cmd = "ts0";
		break;
		case AM_AV_TS_SRC_TS1:
			cmd = "ts1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case AM_AV_TS_SRC_TS2:
			cmd = "ts2";
		break;
#endif
		case AM_AV_TS_SRC_HIU:
			cmd = "hiu";
		break;
		case AM_AV_TS_SRC_DMX0:
			cmd = "dmx0";
		break;
		case AM_AV_TS_SRC_DMX1:
			cmd = "dmx1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case AM_AV_TS_SRC_DMX2:
			cmd = "dmx2";
		break;
#endif
		default:
			AM_DEBUG(1, "illegal ts source %d", src);
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}

	return AM_FileEcho("/sys/class/stb/source", cmd);
}

static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	AV_FileSeekPara_t *sp;
	int rc = -1;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;

	if (data->media_id < 0)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	switch(cmd)
	{
		case AV_PLAY_START:
			player_start_play(data->media_id);
		break;
		case AV_PLAY_PAUSE:
			player_pause(data->media_id);
		break;
		case AV_PLAY_RESUME:
			player_resume(data->media_id);
		break;
		case AV_PLAY_FF:
			player_forward(data->media_id, (long)para);
		break;
		case AV_PLAY_FB:
			player_backward(data->media_id, (long)para);
		break;
		case AV_PLAY_SEEK:
			sp = (AV_FileSeekPara_t *)para;
			 player_timesearch(data->media_id, sp->pos);
		break;
		default:
			AM_DEBUG(1, "illegal media player command");
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_inject_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
	AV_InjectData_t *data;

	UNUSED(para);

	data = (AV_InjectData_t *)dev->inject_player.drv_data;

	switch (cmd)
	{
		case AV_PLAY_PAUSE:
			if (data->aud_id != -1)
			{
#if defined(ADEC_API_NEW)
				audio_decode_pause(data->adec);
#else
				//TODO
#endif
			}
			if (data->vid_fd != -1 && data->cntl_fd != -1)
			{
				ioctl(data->cntl_fd, AMSTREAM_IOC_VPAUSE, 1);
			}
			AM_DEBUG(1, "pause inject");
		break;
		case AV_PLAY_RESUME:
			if (data->aud_id != -1)
			{
#if defined(ADEC_API_NEW)
				audio_decode_resume(data->adec);
#else
				//TODO
#endif
			}
			if (data->vid_fd != -1 && data->cntl_fd != -1)
			{
				ioctl(data->cntl_fd, AMSTREAM_IOC_VPAUSE, 0);
			}
			AM_DEBUG(1, "resume inject");
		break;
		default:
			AM_DEBUG(1, "illegal media player command");
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	int rc;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
	if (data->media_id == -1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	player_info_t pinfo;

	if (player_get_play_info(data->media_id,&pinfo) < 0)
	{
		AM_DEBUG(1, "player_get_play_info failed");
		return AM_AV_ERR_SYS;
	}
	status->duration = pinfo.full_time;
	status->position = pinfo.current_time;
#endif
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info)
{
#ifdef PLAYER_API_NEW
	AV_FilePlayerData_t *data;
	int rc;

	data = (AV_FilePlayerData_t *)dev->file_player.drv_data;
	if (data->media_id == -1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}

	media_info_t minfo;

	if (player_get_media_info(data->media_id,&minfo) < 0)
	{
		AM_DEBUG(1, "player_get_media_info failed");
		return AM_AV_ERR_SYS;
	}
	info->duration = minfo.stream_info.duration;
	info->size = minfo.stream_info.file_size;
#endif

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val)
{
	const char *name = NULL, *cmd = "";
	char buf[32];
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t *win;

	switch (para)
	{
		case AV_VIDEO_PARA_WINDOW:
			name = VID_AXIS_FILE;
			win = (AV_VideoWindow_t *)val;
			snprintf(buf, sizeof(buf), "%d %d %d %d", win->x, win->y, win->x+win->w, win->y+win->h);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_CONTRAST:
			name = VID_CONTRAST_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_SATURATION:
			name = VID_SATURATION_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_BRIGHTNESS:
			name = VID_BRIGHTNESS_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_ENABLE:
			name = VID_DISABLE_FILE;
			cmd = ((long)val)?"0":"1";
		break;
		case AV_VIDEO_PARA_BLACKOUT:
			if (!(dev->mode & (AV_PLAY_TS | AV_TIMESHIFT))) {
				name = VID_BLACKOUT_FILE;
				cmd = ((long)val)?"1":"0";
			}
			dev->video_blackout = (long)val;
#if 0
#ifdef AMSTREAM_IOC_CLEAR_VBUF
			if((int)val)
			{
				int fd = open(AMVIDEO_FILE, O_RDWR);
				if(fd!=-1)
				{
					ioctl(fd, AMSTREAM_IOC_CLEAR_VBUF, 0);
					close(fd);
				}
			}
#endif
#endif
		break;
		case AV_VIDEO_PARA_RATIO:
#ifndef 	CHIP_8226H
			name = VID_SCREEN_MODE_FILE;
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_AUTO:
					cmd = "0";
				break;
				case AM_AV_VIDEO_ASPECT_16_9:
					cmd = "3";
				break;
				case AM_AV_VIDEO_ASPECT_4_3:
					cmd = "2";
				break;
			}
#else
			name = VID_ASPECT_RATIO_FILE;
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_AUTO:
					cmd = "auto";
				break;
				case AM_AV_VIDEO_ASPECT_16_9:
					cmd = "16x9";
				break;
				case AM_AV_VIDEO_ASPECT_4_3:
					cmd = "4x3";
				break;
			}
 #endif
		break;
		case AV_VIDEO_PARA_RATIO_MATCH:
			name = VID_ASPECT_MATCH_FILE;
#ifndef 	CHIP_8226H
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_MATCH_IGNORE:
					cmd = "1";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX:
					cmd = "3";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN:
					cmd = "2";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_COMBINED:
					cmd = "4";
				break;
			}
#else
			switch ((long)val)
			{
				case AM_AV_VIDEO_ASPECT_MATCH_IGNORE:
					cmd = "ignore";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_LETTER_BOX:
					cmd = "letter box";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_PAN_SCAN:
					cmd = "pan scan";
				break;
				case AM_AV_VIDEO_ASPECT_MATCH_COMBINED:
					cmd = "combined";
				break;
			}
#endif
		break;
		case AV_VIDEO_PARA_MODE:
			name = VID_SCREEN_MODE_FILE;
			cmd = ((AM_AV_VideoDisplayMode_t)val)?"1":"0";
		break;
		case AV_VIDEO_PARA_CLEAR_VBUF:
#if 0
#ifdef AMSTREAM_IOC_CLEAR_VBUF
			{
				int fd = open(AMVIDEO_FILE, O_RDWR);
				if(fd!=-1)
				{
					ioctl(fd, AMSTREAM_IOC_CLEAR_VBUF, 0);
					close(fd);
				}
			}
#endif
#endif
			name = VID_DISABLE_FILE;
			cmd = "2";
		break;
		case AV_VIDEO_PARA_ERROR_RECOVERY_MODE:
			name = VDEC_H264_ERROR_RECOVERY_MODE_FILE;
			snprintf(buf, sizeof(buf), "%ld", (long)val);
			cmd = buf;
		break;
	}

	if(name)
	{
		ret = AM_FileEcho(name, cmd);
	}

	return ret;
}

static AM_ErrorCode_t aml_inject(AM_AV_Device_t *dev, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout)
{
	AV_InjectData_t *inj = (AV_InjectData_t*)dev->inject_player.drv_data;
	int fd, send, ret;

	if ((inj->pkg_fmt == PFORMAT_ES) && (type == AM_AV_INJECT_AUDIO))
		fd = inj->aud_fd;
	else
		fd = inj->vid_fd;

	if (fd == -1)
	{
		AM_DEBUG(1, "device is not openned");
		return AM_AV_ERR_NOT_ALLOCATED;
	}

	if (timeout >= 0)
	{
		struct pollfd pfd;

		pfd.fd = fd;
		pfd.events = POLLOUT;

		ret = poll(&pfd, 1, timeout);
		if (ret != 1)
			return AM_AV_ERR_TIMEOUT;
	}

	send = *size;
	if (send)
	{
		ret = write(fd, data, send);
		if ((ret == -1) && (errno != EAGAIN))
		{
			AM_DEBUG(1, "inject data failed errno:%d msg:%s", errno, strerror(errno));
			return AM_AV_ERR_SYS;
		}
		else if ((ret == -1) && (errno == EAGAIN))
		{
			ret = 0;
		}
	}
	else
		ret = 0;

	*size = ret;

	return AM_SUCCESS;
}

#ifdef AMSTREAM_IOC_GET_VBUF
extern AM_ErrorCode_t ge2d_blit_yuv(struct vbuf_info *info, AM_OSD_Surface_t *surf);
#endif

static AM_ErrorCode_t aml_video_frame(AM_AV_Device_t *dev, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s)
{
#ifdef AMSTREAM_IOC_GET_VBUF
	int fd = -1, rc;
	struct vbuf_info info;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_OSD_PixelFormatType_t type;
	AM_OSD_Surface_t *surf = NULL;
	int w, h, append;

	fd = open(AMVIDEO_FILE, O_RDWR);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", AMVIDEO_FILE);
		ret = AM_AV_ERR_SYS;
		goto end;
	}

	rc = ioctl(fd, AMSTREAM_IOC_GET_VBUF, &info);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_GET_VBUF_INFO failed");
		ret = AM_AV_ERR_SYS;
		goto end;
	}

	type = AM_OSD_FMT_COLOR_RGB_888;
	w = info.width;
	h = info.height;

	ret = AM_OSD_CreateSurface(type, w, h, AM_OSD_SURFACE_FL_HW, &surf);
	if (ret != AM_SUCCESS)
		goto end;

	ret = ge2d_blit_yuv(&info, surf);
	if (ret != AM_SUCCESS)
		goto end;

	*s = surf;
	return AM_SUCCESS;

end:
	if (surf)
		AM_OSD_DestroySurface(surf);
	if (fd != -1)
		close(fd);
	return ret;
#else
	UNUSED(dev);
	UNUSED(para);
	UNUSED(s);

	return AM_AV_ERR_NOT_SUPPORTED;
#endif
}

static AM_ErrorCode_t aml_get_astatus(AM_AV_Device_t *dev, AM_AV_AudioStatus_t *para)
{
	struct am_io_param astatus;
	struct adec_status armadec;

	int fd, rc;
	char buf[32];

	void *adec = NULL;

	pthread_mutex_lock(&gAVMonLock);

#if 1
	switch (dev->mode) {
		case AV_PLAY_TS:
			adec = ((AV_TSData_t *)dev->ts_player.drv_data)->adec;
			break;
		case AV_INJECT:
			adec = ((AV_InjectData_t *)dev->inject_player.drv_data)->adec;
			break;
		case AV_TIMESHIFT:
			adec = ((AV_TimeshiftData_t *)dev->timeshift_player.drv_data)->adec;
			break;
		default:
			AM_DEBUG(1, "only valid in TS/INJ/TIMESHIFT mode");
			break;
	}

	para->lfepresent = -1;
	para->aud_fmt_orig = -1;
	para->resolution_orig = -1;
	para->channels_orig = -1;
	para->sample_rate_orig = -1;
	para->lfepresent_orig = -1;
	rc = audio_decpara_get(adec, &para->sample_rate_orig, &para->channels_orig, &para->lfepresent_orig);
	if (rc == -1)
	{
		AM_DEBUG(1, "cannot get decpara");
		goto get_fail;
	}

	rc = get_decoder_status(adec, &armadec);
	if (rc == -1)
	{
		AM_DEBUG(1, "cannot get status in this mode");
		goto get_fail;
	}

	para->channels    = armadec.channels;
	para->sample_rate = armadec.sample_rate;
	switch (armadec.resolution)
	{
		case 0:
			para->resolution  = 8;
			break;
		case 1:
			para->resolution  = 16;
			break;
		case 2:
			para->resolution  = 32;
			break;
		case 3:
			para->resolution  = 32;
			break;
		case 4:
			para->resolution  = 64;
			break;
		default:
			para->resolution  = 16;
			break;
	}

	para->frames      = 1;
	para->aud_fmt     = -1;
#else
	rc = ioctl(fd, AMSTREAM_IOC_ADECSTAT, (int)&astatus);
	if (rc==-1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_ADECSTAT failed");
		goto get_fail;
	}

	para->channels    = astatus.astatus.channels;
	para->sample_rate = astatus.astatus.sample_rate;
	para->resolution  = astatus.astatus.resolution;
	para->frames      = 1;
	para->aud_fmt     = -1;
#endif

	if (AM_FileRead(ASTREAM_FORMAT_FILE, buf, sizeof(buf)) == AM_SUCCESS)
	{
		if (!strncmp(buf, "amadec_mpeg", 11))
			para->aud_fmt = AFORMAT_MPEG;
		else if (!strncmp(buf, "amadec_pcm_s16le", 16))
			para->aud_fmt = AFORMAT_PCM_S16LE;
		else if (!strncmp(buf, "amadec_aac", 10))
			para->aud_fmt = AFORMAT_AAC;
		else if (!strncmp(buf, "amadec_ac3", 10))
			para->aud_fmt = AFORMAT_AC3;
		else if (!strncmp(buf, "amadec_alaw", 11))
			para->aud_fmt = AFORMAT_ALAW;
		else if (!strncmp(buf, "amadec_mulaw", 12))
			para->aud_fmt = AFORMAT_MULAW;
		else if (!strncmp(buf, "amadec_dts", 10))
			para->aud_fmt = AFORMAT_MULAW;
		else if (!strncmp(buf, "amadec_pcm_s16be", 16))
			para->aud_fmt = AFORMAT_PCM_S16BE;
		else if (!strncmp(buf, "amadec_flac", 11))
			para->aud_fmt = AFORMAT_FLAC;
		else if (!strncmp(buf, "amadec_cook", 11))
			para->aud_fmt = AFORMAT_COOK;
		else if (!strncmp(buf, "amadec_pcm_u8", 13))
			para->aud_fmt = AFORMAT_PCM_U8;
		else if (!strncmp(buf, "amadec_adpcm", 12))
			para->aud_fmt = AFORMAT_ADPCM;
		else if (!strncmp(buf, "amadec_amr", 10))
			para->aud_fmt = AFORMAT_AMR;
		else if (!strncmp(buf, "amadec_raac", 11))
			para->aud_fmt = AFORMAT_RAAC;
		else if (!strncmp(buf, "amadec_wma", 10))
			para->aud_fmt = AFORMAT_WMA;
		else if (!strncmp(buf,"amadec_dra",10))
			para->aud_fmt = AFORMAT_DRA;
	}

	if (para->aud_fmt_orig == -1)
		para->aud_fmt_orig = para->aud_fmt;
	if (para->resolution_orig == -1)
		para->resolution_orig = para->resolution;
	if (para->sample_rate_orig == -1)
		para->sample_rate_orig = para->sample_rate;
	if (para->channels_orig == -1)
		para->channels_orig = para->channels;
	if (para->lfepresent_orig == -1)
		para->lfepresent_orig = 0;
	if (para->lfepresent == -1)
		para->lfepresent = 0;

	fd = get_amstream(dev);
	if (fd == -1)
	{
		//AM_DEBUG(1, "cannot get status in this mode");
		goto get_fail;
	}

	rc = ioctl(fd, AMSTREAM_IOC_AB_STATUS, (long)&astatus);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_AB_STATUS failed");
		goto get_fail;
	}

	para->ab_size = astatus.status.size;
	para->ab_data = astatus.status.data_len;
	para->ab_free = astatus.status.free_len;

	pthread_mutex_unlock(&gAVMonLock);
	return AM_SUCCESS;

get_fail:
	pthread_mutex_unlock(&gAVMonLock);
	return AM_FAILURE;
}

static AM_ErrorCode_t aml_get_vstatus(AM_AV_Device_t *dev, AM_AV_VideoStatus_t *para)
{
	struct am_io_param vstatus;
	char buf[32];
	int fd, rc;

	pthread_mutex_lock(&gAVMonLock);
	fd = get_amstream(dev);
	if (fd == -1)
	{
		//AM_DEBUG(1, "cannot get status in this mode");
		goto get_fail;
	}

	rc = ioctl(fd, AMSTREAM_IOC_VDECSTAT, &vstatus);

	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_VDECSTAT failed");
		goto get_fail;
	}

	para->vid_fmt   = dev->ts_player.play_para.vfmt;
	para->src_w     = vstatus.vstatus.width;
	para->src_h     = vstatus.vstatus.height;
	para->fps       = vstatus.vstatus.fps;
	para->frames    = 1;
	para->interlaced  = 1;

#if 1
	if (AM_FileRead("/sys/class/video/frame_format", buf, sizeof(buf)) >= 0) {
		char *ptr = strstr(buf, "interlace");
		if (ptr) {
			para->interlaced = 1;
		} else {
			para->interlaced = 0;
		}
	}
#else
	if(AM_FileRead("/sys/module/amvdec_mpeg12/parameters/pic_type", buf, sizeof(buf))>=0){
		int i = strtol(buf, NULL, 0);
		if(i==1)
			para->interlaced = 0;
		else if(i==2)
			para->interlaced = 1;
	}
	if(AM_FileRead("/sys/module/amvdec_h264/parameters/pic_type", buf, sizeof(buf))>=0){
		int i = strtol(buf, NULL, 0);
		if(i==1)
			para->interlaced = 0;
		else if(i==2)
			para->interlaced = 1;
       }
       if(AM_FileRead("/sys/module/amvdec_avs/parameters/pic_type", buf, sizeof(buf))>=0){
                int i = strtol(buf, NULL, 0);
                if(i==1)
                        para->interlaced = 0;
                else if(i==2)
                        para->interlaced = 1;
       }
#endif

	rc = ioctl(fd, AMSTREAM_IOC_VB_STATUS, (long)&vstatus);
	if (rc == -1)
	{
		AM_DEBUG(1, "AMSTREAM_IOC_VB_STATUS failed");
		goto get_fail;
	}

	para->vb_size = vstatus.status.size;
	para->vb_data = vstatus.status.data_len;
	para->vb_free = vstatus.status.free_len;

	pthread_mutex_unlock(&gAVMonLock);
	return AM_SUCCESS;

get_fail:
	pthread_mutex_unlock(&gAVMonLock);
	return AM_FAILURE;
}

static AM_ErrorCode_t aml_set_ppmgr_3dcmd(int cmd)
{
	int ppmgr_fd;
	int arg = -1, ret;

	ppmgr_fd = open(PPMGR_FILE, O_RDWR);
	if (ppmgr_fd < 0)
	{
		AM_DEBUG(0, "Open /dev/ppmgr error(%s)!\n", strerror(errno));
		return AM_AV_ERR_SYS;
	}

    switch (cmd)
    {
    case AM_AV_PPMGR_MODE3D_DISABLE:
        arg = MODE_3D_DISABLE;
        AM_DEBUG(1,"3D fucntion (0: Disalbe!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_AUTO:
        arg = MODE_3D_ENABLE|MODE_AUTO;
        AM_DEBUG(1,"3D fucntion (1: Auto!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_2D_TO_3D:
        arg = MODE_3D_ENABLE|MODE_2D_TO_3D;
        AM_DEBUG(1,"3D fucntion (2: 2D->3D!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_LR:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (3: L/R!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_BT:
        arg = MODE_3D_ENABLE|MODE_BT;
        AM_DEBUG(1,"3D fucntion (4: B/T!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (5: LR SWITCH OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_ON_LR_SWITCH:
        arg = MODE_3D_ENABLE|MODE_LR_SWITCH;
        AM_DEBUG(1,"3D fucntion (6: LR SWITCH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_FIELD_DEPTH:
        arg = MODE_3D_ENABLE|MODE_FIELD_DEPTH;
        AM_DEBUG(1,"3D function (7: FIELD_DEPTH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_LR;
        AM_DEBUG(1,"3D fucntion (8: 3D_TO_2D_TURN_OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_L_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_L;
        AM_DEBUG(1,"3D function (9: 3D_TO_2D_L!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_R_3D_TO_2D:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_R;
        AM_DEBUG(1,"3D function (10: 3D_TO_2D_R!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_LR_SWITCH_BT:
        arg = MODE_3D_ENABLE|MODE_BT|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (11: BT SWITCH OFF!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_ON_LR_SWITCH_BT:
        arg = MODE_3D_ENABLE|MODE_LR_SWITCH|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D fucntion (12: BT SWITCH!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_OFF_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_BT;
        AM_DEBUG(1,"3D fucntion (13: 3D_TO_2D_TURN_OFF_BT!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_L_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_L|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (14: 3D TO 2D L BT!)\n");
        break;
    case AM_AV_PPMGR_MODE3D_R_3D_TO_2D_BT:
        arg = MODE_3D_ENABLE|MODE_3D_TO_2D_R|BT_FORMAT_INDICATOR;
        AM_DEBUG(1,"3D function (15: 3D TO 2D R BT!)\n");
        break;
    default:
    	AM_DEBUG(1, "Unkown set 3D cmd %d", cmd);
    	arg = -1;
    	break;
    }

    if (arg != -1)
    {
    	if (ioctl(ppmgr_fd, PPMGR_IOC_ENABLE_PP, arg) == -1)
    	{
    		AM_DEBUG(1, "Set 3D function failed: %s", strerror(errno));
    		close(ppmgr_fd);
    		return AM_AV_ERR_SYS;
    	}
    }

	close(ppmgr_fd);

    return AM_SUCCESS;
}

static int
get_osd_prop(const char *mode, const char *p, const char *defv)
{
	char n[32];
	char v[32];
	int r;

	snprintf(n, sizeof(n), "ubootenv.var.%soutput%s", mode, p);
	property_get(n,v,defv);
	sscanf(v, "%d", &r);

	return r;
}

static void
get_osd_rect(const char *mode, int *x, int *y, int *w, int *h)
{
	const char *m = mode?mode:"720p";
	char defw[16], defh[16];
	int r;

	r = get_osd_prop(m, "x", "0");
	*x = r;
	r = get_osd_prop(m, "y", "0");
	*y = r;
	if (!strncmp(m, "480", 3)) {
		snprintf(defw, sizeof(defw), "%d", 720);
		snprintf(defh, sizeof(defh), "%d", 480);
	} else if (!strncmp(m, "576", 3)) {
		snprintf(defw, sizeof(defw), "%d", 720);
		snprintf(defh, sizeof(defh), "%d", 576);
	} else if (!strncmp(m, "720", 3)) {
		snprintf(defw, sizeof(defw), "%d", 1280);
		snprintf(defh, sizeof(defh), "%d", 720);
	} else if (!strncmp(m, "1080", 4)) {
		snprintf(defw, sizeof(defw), "%d", 1920);
		snprintf(defh, sizeof(defh), "%d", 1080);
	}


	r = get_osd_prop(m, "width", defw);
	*w = r;
	r = get_osd_prop(m, "height", defh);
	*h = r;
}

static AM_ErrorCode_t
aml_set_vpath(AM_AV_Device_t *dev)
{
#if 0
	static char s_bypass_hd[2];
	static char s_bypass_hd_prog[2];
	static char s_bypass_prog[2];
	static char s_bypass_1080p[2];
	static char s_bypass_dynamic[2];

	AM_ErrorCode_t ret;
	int times = 10;

	AM_DEBUG(1, "set video path fs:%d di:%d ppmgr:%d", dev->vpath_fs, dev->vpath_di, dev->vpath_ppmgr);

	//AM_FileEcho("/sys/class/deinterlace/di0/config", "disable");
	/*
	do{
		ret = AM_FileEcho("/sys/class/vfm/map", "rm default");
		if(ret!=AM_SUCCESS){
			usleep(10000);
		}
	}while(ret!=AM_SUCCESS && times--);
	*/

	char video_axis[32];
	AM_FileRead("/sys/class/video/axis", video_axis, sizeof(video_axis));

	if(dev->vpath_fs==AM_AV_FREE_SCALE_ENABLE){
		char mode[16];
		char ppr[32];
		AM_Bool_t blank = AM_TRUE;
#ifdef ANDROID
		{
			int x, y, w, h;

			AM_FileRead("/sys/class/display/mode", mode, sizeof(mode));

			if(!strncmp(mode, "480i", 4)){
				get_osd_rect("480i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "480p", 4)){
				get_osd_rect("480p", &x, &y, &w, &h);
			}else if(!strncmp(mode, "480cvbs", 7)){
				get_osd_rect("480cvbs", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576i", 4)){
				get_osd_rect("576i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576p", 4)){
				get_osd_rect("576p", &x, &y, &w, &h);
			}else if(!strncmp(mode, "576cvbs", 7)){
				get_osd_rect("576cvbs", &x, &y, &w, &h);
			}else if(!strncmp(mode, "720p", 4)){
				get_osd_rect("720p", &x, &y, &w, &h);
				blank = AM_FALSE;
			}else if(!strncmp(mode, "1080i", 5)){
				get_osd_rect("1080i", &x, &y, &w, &h);
			}else if(!strncmp(mode, "1080p", 5)){
				get_osd_rect("1080p", &x, &y, &w, &h);
			}else{
				get_osd_rect(NULL, &x, &y, &w, &h);
			}
			snprintf(ppr, sizeof(ppr), "%d %d %d %d 0", x, y, x+w, y+h);
		}
#endif
		AM_FileRead("/sys/class/graphics/fb0/request2XScale", mode, sizeof(mode));
		if (blank && !strncmp(mode, "2", 1)) {
			blank = AM_FALSE;
		}
		if(blank){
			//AM_FileEcho("/sys/class/graphics/fb0/blank", "1");
		}
		//AM_FileEcho("/sys/class/graphics/fb0/free_scale", "1");
		//AM_FileEcho("/sys/class/graphics/fb1/free_scale", "1");
#ifdef ANDROID
		{
			AM_FileEcho("/sys/class/graphics/fb0/request2XScale", "2");
			AM_FileEcho("/sys/class/graphics/fb1/scale", "0");

			AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "0");
			AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "0");

			s_bypass_hd[1] = '\0';
			s_bypass_hd_prog[1] = '\0';
			s_bypass_prog[1] = '\0';
			s_bypass_1080p[1] = '\0';
			AM_FileEcho("/sys/module/di/parameters/bypass_hd", s_bypass_hd);
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog", s_bypass_hd_prog);
			AM_FileEcho("/sys/module/di/parameters/bypass_prog", s_bypass_prog);
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p", s_bypass_1080p);
#ifdef ENABLE_CORRECR_AV_SYNC
#else
			s_bypass_dynamic[1] = '\0';
			AM_FileEcho("/sys/module/di/parameters/bypass_dynamic", s_bypass_dynamic);
#endif

			AM_FileEcho("/sys/class/ppmgr/ppscaler","1");

			AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", "0");
			usleep(200*1000);
		}
#endif
	}else{
		AM_Bool_t blank = AM_TRUE;
		char m1080scale[8];
		char mode[16];
		char verstr[32];
		char *reqcmd, *osd1axis, *osd1cmd;
		int version;
		AM_Bool_t scale=AM_TRUE;

#ifdef ANDROID
		{
			property_get("ro.platform.has.1080scale",m1080scale,"fail");
			if(!strncmp(m1080scale, "fail", 4)){
				scale = AM_FALSE;
			}

			AM_FileRead("/sys/class/display/mode", mode, sizeof(mode));

			if(strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))){
				scale = AM_FALSE;
			}

			AM_FileRead("/sys/class/graphics/fb0/request2XScale", verstr, sizeof(verstr));
			if(!strncmp(mode, "480i", 4) || !strncmp(mode, "480p", 4) || !strncmp(mode, "480cvbs", 7)){
				reqcmd   = "16 720 480";
				osd1axis = "1280 720 720 480";
				osd1cmd  = "0x10001";
			}else if(!strncmp(mode, "576i", 4) || !strncmp(mode, "576p", 4) || !strncmp(mode, "576cvbs", 7)){
				reqcmd   = "16 720 576";
				osd1axis = "1280 720 720 576";
				osd1cmd  = "0x10001";
			}else if(!strncmp(mode, "1080i", 5) || !strncmp(mode, "1080p", 5)){
				reqcmd   = "8";
				osd1axis = "1280 720 1920 1080";
				osd1cmd  = "0x10001";
			}else{
				reqcmd   = "2";
				osd1axis = NULL;
				osd1cmd  = "0";
				blank    = AM_FALSE;
			}

			if (blank && !strncmp(verstr, reqcmd, strlen(reqcmd))) {
				blank = AM_FALSE;
			}

			property_get("ro.build.version.sdk",verstr,"10");
			if(sscanf(verstr, "%d", &version)==1){
				if(version < 15){
					blank = AM_FALSE;
				}
			}
		}
#endif
		if(blank && scale){
			//AM_FileEcho("/sys/class/graphics/fb0/blank", "1");
		}

		AM_FileEcho("/sys/class/graphics/fb0/free_scale", "0");
		AM_FileEcho("/sys/class/graphics/fb1/free_scale", "0");

#ifdef ANDROID
		{
			if(scale){
				AM_FileEcho("/sys/class/graphics/fb0/request2XScale", reqcmd);
				if(osd1axis){
					AM_FileEcho("/sys/class/graphics/fb1/scale_axis", osd1axis);
				}
				AM_FileEcho("/sys/class/graphics/fb1/scale", osd1cmd);
			}


			AM_FileEcho("/sys/module/amvdec_h264/parameters/dec_control", "3");
			AM_FileEcho("/sys/module/amvdec_mpeg12/parameters/dec_control", "62");

			AM_FileRead("/sys/module/di/parameters/bypass_hd", s_bypass_hd, sizeof(s_bypass_hd));
			AM_FileRead("/sys/module/di/parameters/bypass_hd_prog", s_bypass_hd_prog, sizeof(s_bypass_hd_prog));
			AM_FileRead("/sys/module/di/parameters/bypass_prog", s_bypass_prog, sizeof(s_bypass_prog));
			AM_FileRead("/sys/module/di/parameters/bypass_1080p", s_bypass_1080p, sizeof(s_bypass_1080p));

#ifdef ENABLE_CORRECR_AV_SYNC
			AM_DEBUG(1, "ENABLE_CORRECR_AV_SYNC");

			AM_FileEcho("/sys/module/di/parameters/bypass_hd","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p","0");
#else
			AM_FileRead("/sys/module/di/parameters/bypass_dynamic", s_bypass_dynamic, sizeof(s_bypass_dynamic));

			AM_DEBUG(1, "Not ENABLE_CORRECR_AV_SYNC");

			AM_FileEcho("/sys/module/di/parameters/bypass_hd","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_hd_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_prog","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_1080p","0");
			AM_FileEcho("/sys/module/di/parameters/bypass_dynamic","1");

			//AM_FileEcho("/sys/module/di/parameters/bypass_hd","1");
#endif
			AM_FileEcho("/sys/class/ppmgr/ppscaler","0");

			//AM_FileEcho("/sys/class/ppmgr/ppscaler_rect","0 0 0 0 1");
			//AM_FileEcho("/sys/class/video/axis", "0 0 0 0");
			AM_FileEcho("/sys/module/amvideo/parameters/smooth_sync_enable", AV_SMOOTH_SYNC_VAL);
			AM_FileEcho("/sys/module/di/parameters/bypass_all","0");
			usleep(2000*1000);
		}
#endif
	}

	/*
	if(dev->vpath_ppmgr!=AM_AV_PPMGR_DISABLE){
		if (dev->vpath_ppmgr == AM_AV_PPMGR_MODE3D_2D_TO_3D){
			AM_FileEcho("/sys/class/vfm/map", "add default decoder deinterlace d2d3 amvideo");
			AM_FileEcho("/sys/class/d2d3/d2d3/debug", "enable");
		}else{
			AM_FileEcho("/sys/class/vfm/map", "add default decoder ppmgr amvideo");
			if (dev->vpath_ppmgr!=AM_AV_PPMGR_ENABLE) {
				//Set 3D mode
				AM_TRY(aml_set_ppmgr_3dcmd(dev->vpath_ppmgr));
			}
		}

	}else if(dev->vpath_di==AM_AV_DEINTERLACE_ENABLE){
		AM_FileEcho("/sys/class/vfm/map", "add default decoder deinterlace amvideo");
		AM_FileEcho("/sys/class/deinterlace/di0/config", "enable");
	}else{
		AM_FileEcho("/sys/class/vfm/map", "add default decoder amvideo");
	}
	*/

	AM_FileEcho("/sys/class/video/axis", video_axis);
	AM_FileRead("/sys/class/video/axis", video_axis, sizeof(video_axis));
#else
	UNUSED(dev);
#endif
	return AM_SUCCESS;
}

/**\brief 切换TS播放的音频*/
static AM_ErrorCode_t aml_switch_ts_audio_legacy(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt)
{
	int fd = -1;
	AM_Bool_t audio_valid = VALID_AUDIO(apid, afmt);
	AM_Bool_t has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);
	AV_TSData_t *ts = NULL;

	AM_DEBUG(1, "switch ts audio: A[%d:%d]", apid, afmt);

	if (dev->ts_player.drv_data) {
		ts = (AV_TSData_t*)dev->ts_player.drv_data;
		fd = ts->fd;
	}

	if (fd < 0)
	{
		AM_DEBUG(1, "ts_player fd < 0, error!");
		return AM_AV_ERR_SYS;
	}

	/*Stop Audio first*/
	if (dev->ts_player.av_thread_running)
	{
		dev->ts_player.av_thread_running = AM_FALSE;
		pthread_cond_broadcast(&gAVMonCond);
		pthread_join(dev->ts_player.av_mon_thread, NULL);
	}

	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	adec_set_decode_ad(0, 0, 0, ts->adec);
	adec_stop_decode(&ts->adec);

	/*Set Audio PID & fmt*/
	if (audio_valid)
	{
		if (ioctl(fd, AMSTREAM_IOC_AFORMAT, (int)afmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(fd, AMSTREAM_IOC_AID, (int)apid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}

	/*reset audio*/
	if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
	{
		AM_DEBUG(1, "audio reset failed");
		return AM_AV_ERR_SYS;
	}

	AM_TIME_GetClock(&dev->ts_player.av_start_time);

	dev->ts_player.play_para.apid = apid;
	dev->ts_player.play_para.afmt = afmt;

#ifdef ENABLE_PCR
	if (!show_first_frame_nosync()) {
		property_set("sys.amplayer.drop_pcm", "1");
	}
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");
	aml_set_sync_mode(audio_valid, has_video, afmt);
	/*AM_FileEcho(TSYNC_MODE_FILE, "2");*/
	adec_start_decode(fd, afmt, has_video, &ts->adec);
#endif /*ENABLE_PCR*/


	/*Start Audio*/
	dev->ts_player.av_thread_running = AM_TRUE;
	if (pthread_create(&dev->ts_player.av_mon_thread, NULL, aml_av_monitor_thread, (void*)dev))
	{
		AM_DEBUG(1, "create the av buf monitor thread failed");
		dev->ts_player.av_thread_running = AM_FALSE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_switch_ts_audio_fmt(AM_AV_Device_t *dev)
{
	int fd = -1;
	uint16_t apid; 
	AM_AV_AFormat_t afmt;
	apid = dev->ts_player.play_para.apid ;
	afmt = dev->ts_player.play_para.afmt;
	AM_Bool_t audio_valid = VALID_AUDIO(apid, afmt);
	AM_Bool_t has_video = VALID_VIDEO(dev->ts_player.play_para.vpid, dev->ts_player.play_para.vfmt);
	AV_TSData_t *ts = NULL;

	AM_DEBUG(1, "switch ts audio: A[%d:%d]", apid, afmt);

	if (dev->ts_player.drv_data) {
		ts = (AV_TSData_t *)dev->ts_player.drv_data;
		fd = ts->fd;
	}

	if (fd < 0)
	{
		AM_DEBUG(1, "ts_player fd < 0, error!");
		return AM_AV_ERR_SYS;
	}

	aml_set_ad_source(&ts->ad, 0, 0, 0, ts->adec);
	adec_set_decode_ad(0, 0, 0, ts->adec);
	adec_stop_decode(&ts->adec);

	/*Set Audio PID & fmt*/
	if (audio_valid)
	{
		if (ioctl(fd, AMSTREAM_IOC_AFORMAT, (int)afmt) == -1)
		{
			AM_DEBUG(1, "set audio format failed");
			return AM_AV_ERR_SYS;
		}
		if (ioctl(fd, AMSTREAM_IOC_AID, (int)apid) == -1)
		{
			AM_DEBUG(1, "set audio PID failed");
			return AM_AV_ERR_SYS;
		}
	}
	else
	{
		return AM_SUCCESS;
	}

	/*reset audio*/
	if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
	{
		AM_DEBUG(1, "audio reset failed");
		return AM_AV_ERR_SYS;
	}

#ifdef ENABLE_PCR
	if (!show_first_frame_nosync()) {
		property_set("sys.amplayer.drop_pcm", "1");
	}
	AM_FileEcho(ENABLE_RESAMPLE_FILE, "1");
	aml_set_sync_mode(audio_valid, has_video, afmt);
	/*AM_FileEcho(TSYNC_MODE_FILE, "2");*/
	adec_start_decode(fd, afmt, has_video, &ts->adec);

	uint16_t sub_apid = dev->ts_player.play_para.sub_apid ;
	AM_AV_AFormat_t sub_afmt = dev->ts_player.play_para.sub_afmt;
	if (VALID_PID(sub_apid))
		aml_set_audio_ad(dev, 1, sub_apid, sub_afmt);
#endif /*ENABLE_PCR*/
	AM_DEBUG(1, "switch ts audio: end");
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_switch_ts_audio(AM_AV_Device_t *dev, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AV_TSPlayPara_t *tp = &dev->ts_player.play_para;
	AM_ErrorCode_t err = AM_SUCCESS;

	AM_DEBUG(1, "switch ts audio: A[%d:%d]", apid, afmt);

	dev->audio_switch = AM_TRUE;
	tp->apid = apid;
	tp->afmt = afmt;

	// err = aml_close_ts_mode(dev, AM_TRUE);

	// tp->apid = apid;
	// tp->afmt = afmt;

	// err = aml_open_ts_mode(dev);
	// err = aml_start_ts_mode(dev, &dev->ts_player.play_para, AM_TRUE);

	return err;
}


/**\brief set vdec_h264 error_recovery_mode :0 or 2 -> skip display Mosaic  ,3: display mosaic in case of vdec hit error*/
static AM_ErrorCode_t aml_set_vdec_error_recovery_mode(AM_AV_Device_t *dev, uint8_t error_recovery_mode)
{
    char buf[32];

    UNUSED(dev);

    if (error_recovery_mode > 3)
    {
       AM_DEBUG(1, "set error_recovery_mode input parameters error!");
       return AM_FAILURE;
    }

    snprintf(buf, sizeof(buf), "%d", error_recovery_mode);
    AM_FileEcho(VDEC_H264_ERROR_RECOVERY_MODE_FILE, buf);

    return AM_SUCCESS;
}

AM_ErrorCode_t aml_reset_audio_decoder(AM_AV_Device_t *dev)
{
      int fd = -1;

       if (dev->ts_player.drv_data)
               fd = ((AV_TSData_t *)dev->ts_player.drv_data)->fd;

       if (fd < 0)
       {
               AM_DEBUG(1, "ts_player fd < 0, error!");
              return AM_AV_ERR_SYS;
       }

       /*reset audio*/
       if (ioctl(fd, AMSTREAM_IOC_AUDIO_RESET, 0) == -1)
       {
               AM_DEBUG(1, "audio reset failed");
               return AM_AV_ERR_SYS;
       }

       return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_drm_mode(AM_AV_Device_t *dev, int enable)
{
	int fd = -1;

	if (dev->inject_player.drv_data) {
		fd = ((AV_InjectData_t *)dev->inject_player.drv_data)->vid_fd;
	}

	if (fd < 0)
	{
		AM_DEBUG(1, "inject_player fd < 0, error!");
		return AM_AV_ERR_SYS;
	}

	if (ioctl(fd, AMSTREAM_IOC_SET_DRMMODE, (void *)(long)enable) == -1)
	{
		return AM_AV_ERR_SYS;
	}
	return AM_SUCCESS;
}


static void ad_callback(const uint8_t * data,int len,void * user_data)
{
	printf("ad_callback [%d:%p] [user:%p]\n", len, data, user_data);
	audio_send_associate_data(user_data, (uint8_t *)data, len);
}

static AM_ErrorCode_t aml_set_ad_source(AM_AD_Handle_t *ad, int enable, int pid, int fmt, void *user)
{
	AM_ErrorCode_t err = AM_SUCCESS;

	UNUSED(fmt);

	if (!ad)
		return AM_AV_ERR_INVAL_ARG;

	AM_DEBUG(1, "AD set source enable[%d] pid[%d] fmt[%d]", enable, pid, fmt);

	if (enable) {
		AM_AD_Para_t para = {.dmx_id = 0, .pid = pid, };
		err = AM_AD_Create(ad, &para);
		if (err == AM_SUCCESS) {
			err = AM_AD_SetCallback(*ad, ad_callback, user);
			err = AM_AD_Start(*ad);
			if (err != AM_SUCCESS)
				AM_AD_Destroy(*ad);
		}
	} else {
		if (*ad) {
			err = AM_AD_Stop(*ad);
			err = AM_AD_Destroy(*ad);
			*ad = NULL;
		}
	}
	return err;
}
static AM_ErrorCode_t aml_set_audio_ad(AM_AV_Device_t *dev, int enable, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AM_AD_Handle_t *pad = NULL;
	void *adec = NULL;
	uint16_t sub_apid;
	AM_AV_AFormat_t sub_afmt;
	AM_Bool_t is_valid_audio = VALID_AUDIO(apid, afmt);

	AM_DEBUG(1, "AD aml set audio ad: enable[%d] pid[%d] fmt[%d]", enable, apid, afmt);

	switch (dev->mode) {
		case AV_PLAY_TS:
			adec = ((AV_TSData_t*)dev->ts_player.drv_data)->adec;
			pad = &((AV_TSData_t*)dev->ts_player.drv_data)->ad;
			sub_apid = dev->ts_player.play_para.sub_apid;
			sub_afmt = dev->ts_player.play_para.sub_afmt;
			break;
		case AV_INJECT:
			adec = ((AV_InjectData_t*)dev->inject_player.drv_data)->adec;
			pad = &((AV_InjectData_t*)dev->inject_player.drv_data)->ad;
			sub_apid = dev->inject_player.para.sub_aud_pid;
			sub_afmt = dev->inject_player.para.sub_aud_fmt;
			break;
		case AV_TIMESHIFT:
			adec = ((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->adec;
			pad = &((AV_TimeshiftData_t*)dev->timeshift_player.drv_data)->ad;
			sub_apid = dev->timeshift_player.para.sub_aud_pid;
			sub_afmt = dev->timeshift_player.para.sub_aud_fmt;
			break;
		default:
			AM_DEBUG(1, "only valid in TS/INJ/TIMESHIFT mode");
			return AM_AV_ERR_NOT_SUPPORTED;
	}

	if (enable && !adec) {
		AM_DEBUG(1, "no main audio, this is associate audio setting");
		return AM_AV_ERR_ILLEGAL_OP;
	}

	/*assume ad is enabled if ad handle is not NULL*/
	if ((enable && *pad && (apid == sub_apid) && (afmt == sub_afmt))
		|| (!enable && !*pad))
		return AM_SUCCESS;

	if (enable && is_valid_audio) {

		if ((apid != sub_apid) || (afmt != sub_afmt))
			aml_set_ad_source(pad, 0, sub_apid, sub_afmt, adec);

		/*enable adec's ad*/
		adec_set_decode_ad(1, apid, afmt, adec);

		/*setup date source*/
		aml_set_ad_source(pad, 1, apid, afmt, adec);

	} else if (!enable && pad && *pad) {

		/*shutdown date source*/
		aml_set_ad_source(pad, 0, apid, afmt, adec);

		/*disable adec's ad*/
		adec_set_decode_ad(0, apid, afmt, adec);

		apid = -1;
		afmt = 0;
	}

	switch (dev->mode) {
		case AV_PLAY_TS:
			dev->ts_player.play_para.sub_apid = apid;
			dev->ts_player.play_para.sub_afmt = afmt;
			break;
		case AV_INJECT:
			dev->inject_player.para.sub_aud_pid = apid;
			dev->inject_player.para.sub_aud_fmt = afmt;
			break;
		case AV_TIMESHIFT:
			dev->timeshift_player.para.sub_aud_pid = apid;
			dev->timeshift_player.para.sub_aud_fmt = afmt;
			break;
		default:
			break;
	}

	return AM_SUCCESS;
}

