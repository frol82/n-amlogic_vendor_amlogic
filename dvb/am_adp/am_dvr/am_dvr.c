#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <am_debug.h>
#include "am_dvr_internal.h"
#include <am_dmx.h>
#include <am_fend.h>
#include <am_time.h>
#include <am_mem.h>

#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifdef CHIP_8226H
#define DVR_DEV_COUNT      (2)
#elif defined(CHIP_8226M) || defined(CHIP_8626X)
#define DVR_DEV_COUNT      (3)
#else
#define DVR_DEV_COUNT      (2)
#endif

/****************************************************************************
 * Static data
 ***************************************************************************/
 
#ifdef EMU_DVR
extern const AM_DVR_Driver_t emu_dvr_drv;
#else
extern const AM_DVR_Driver_t linux_dvb_dvr_drv;
#endif

static ssize_t dvr_read(int dvr_no, void *buf, size_t size);
static ssize_t dvr_write(int dvr_no, void *buf, size_t size);
static loff_t dvr_seek(int dev_no, loff_t offset, int whence);

static AM_DVR_Device_t dvr_devices[DVR_DEV_COUNT] =
{
#ifdef EMU_DVR
{
.drv = &emu_dvr_drv,
},
{
.drv = &emu_dvr_drv,
}
#else
{
.drv = &linux_dvb_dvr_drv,
},
{
.drv = &linux_dvb_dvr_drv,
}
#if defined(CHIP_8226M) || defined(CHIP_8626X)
,
{
.drv = &linux_dvb_dvr_drv,
}
#endif
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief ???????????????????????????????????????*/
static AM_INLINE AM_ErrorCode_t dvr_get_dev(int dev_no, AM_DVR_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=DVR_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid dvr device number %d, must in(%d~%d)", dev_no, 0, DVR_DEV_COUNT-1);
		return AM_DVR_ERR_INVALID_DEV_NO;
	}
	
	*dev = &dvr_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief ??????????????????????????????????????????????????????????????????*/
static AM_INLINE AM_ErrorCode_t dvr_get_openned_dev(int dev_no, AM_DVR_Device_t **dev)
{
	AM_TRY(dvr_get_dev(dev_no, dev));
	
	if(!(*dev)->open_cnt)
	{
		AM_DEBUG(1, "dvr device %d has not been openned", dev_no);
		return AM_DVR_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief ???????????????*/
static AM_ErrorCode_t dvr_add_stream(AM_DVR_Device_t *dev, uint16_t pid)
{
	int i;
	
	if (dev->stream_cnt >= AM_DVR_MAX_PID_COUNT)
	{
		AM_DEBUG(1, "PID count overflow, Max support %d", AM_DVR_MAX_PID_COUNT);
		return AM_DVR_ERR_TOO_MANY_STREAMS;
	}

	i = dev->stream_cnt;

	dev->streams[i].pid = pid;
	dev->streams[i].fid = -1;
	dev->stream_cnt++;
	
	return AM_SUCCESS;
}


/**\brief ????????????????????????*/
static AM_ErrorCode_t dvr_start_all_streams(AM_DVR_Device_t *dev)
{
	int i;
	struct dmx_pes_filter_params pparam;

	AM_DEBUG(1, "Start DVR%d recording, pid count %d", dev->dev_no, dev->stream_cnt);
	for(i=0; i<dev->stream_cnt; i++)
	{
		if (dev->streams[i].fid == -1)
		{
			if (AM_DMX_AllocateFilter(dev->dmx_no, &dev->streams[i].fid) == AM_SUCCESS)
			{
				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = dev->streams[i].pid;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TS_TAP;
				pparam.pes_type = DMX_PES_OTHER;
				AM_DMX_SetPesFilter(dev->dmx_no, dev->streams[i].fid, &pparam);
				AM_DMX_StartFilter(dev->dmx_no, dev->streams[i].fid);
				AM_DEBUG(1, "Stream(pid=%d) start recording...", dev->streams[i].pid);
			}
			else
			{
				dev->streams[i].fid = -1;
				AM_DEBUG(1, "Cannot alloc filter, stream(pid=%d) will not record.", dev->streams[i].pid);
			}
		}
	}

	return AM_SUCCESS;
}

/**\brief ????????????????????????*/
static AM_ErrorCode_t dvr_stop_all_streams(AM_DVR_Device_t *dev)
{
	int i;
	
	for(i=0; i<dev->stream_cnt; i++)
	{
		if (dev->streams[i].fid != -1)
		{
			AM_DEBUG(1, "Stop stream(pid=%d)...", dev->streams[i].pid);
			AM_DMX_StopFilter(dev->dmx_no, dev->streams[i].fid);
			AM_DMX_FreeFilter(dev->dmx_no, dev->streams[i].fid);
			dev->streams[i].fid = -1;
		}
	}

	dev->stream_cnt = 0;

	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/
 
/**\brief ??????DVR??????
 * \param dev_no DVR?????????
 * \param[in] para DVR??????????????????
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_Open(int dev_no, const AM_DVR_OpenPara_t *para)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	assert(para);
	
	AM_TRY(dvr_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->open_cnt)
	{
		AM_DEBUG(1, "dvr device %d has already been openned", dev_no);
		dev->open_cnt++;
		goto final;
	}

	dev->dev_no = dev_no;
	/*DVR????????????DMX???????????????*/
	dev->dmx_no = dev_no;

	for(i=0; i<AM_DVR_MAX_PID_COUNT; i++)
	{
		dev->streams[i].fid = -1;
	}
	
	if(dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_init(&dev->lock, NULL);
		dev->open_cnt++;
		dev->record = AM_FALSE;
		dev->stream_cnt = 0;
	}
	
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief ??????DVR??????
 * \param dev_no DVR?????????
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_Close(int dev_no)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	if(dev->open_cnt > 0){
		/*???????????????*/
		dvr_stop_all_streams(dev);
			
		if(dev->drv->close)
		{
			dev->drv->close(dev);
		}

		pthread_mutex_destroy(&dev->lock);
		dev->record = AM_FALSE;

		dev->open_cnt--;
	}
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief ??????DVR?????????????????????
 * \param dev_no DVR?????????
 * \param size ???????????????
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_SetBufferSize(int dev_no, int size)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->drv->set_buf_size)
	{
		AM_DEBUG(1, "do not support set_buf_size");
		ret = AM_DVR_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
		ret = dev->drv->set_buf_size(dev, size);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief ????????????
 * \param dev_no DVR?????????
 * \param [in] para ????????????
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_StartRecord(int dev_no, const AM_DVR_StartRecPara_t *para)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int pid_cnt;

	assert(para);
	if (para->pid_count <= 0)
	{
		AM_DEBUG(1, "Invalid pid count %d", para->pid_count);
		return AM_DVR_ERR_INVALID_ARG;
	}
	pid_cnt = para->pid_count;
	if (pid_cnt > AM_DVR_MAX_PID_COUNT)
		pid_cnt = AM_DVR_MAX_PID_COUNT;
		
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if (dev->record)
	{
		AM_DEBUG(1, "dvr device %d already recording.", dev_no);
		ret = AM_DVR_ERR_BUSY;
	}
	else
	{
		int i;

		for (i=0; i<pid_cnt; i++)
		{
			dvr_add_stream(dev, (uint16_t)para->pids[i]);
		}
		
		/*????????????*/
		dvr_start_all_streams(dev);
		dev->record = AM_TRUE;
		dev->start_para = *para;
	}
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief ????????????
 * \param dev_no DVR?????????
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_StopRecord(int dev_no)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if (dev->record)
	{
		/*???????????????*/
		dvr_stop_all_streams(dev);
		
		dev->record = AM_FALSE;
	}
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief ???DVR??????????????????
 * \param dev_no DVR?????????
 * \param	[out] buf ?????????
 * \param size	???????????????????????????
 * \param timeout ?????????????????? ms 
 * \return
 *   - ????????????????????????
 */
int AM_DVR_Read(int dev_no, uint8_t *buf, int size, int timeout_ms)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret;
	int cnt = -1;

	AM_TRY(dvr_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	if(!dev->drv->read)
	{
		AM_DEBUG(1, "do not support read");
	}
	else
	{
		ret = dev->drv->poll(dev, timeout_ms);
		if(ret==AM_SUCCESS)
		{
			cnt = size;
			ret = dev->drv->read(dev, buf, &cnt);
			if (ret != AM_SUCCESS)
				cnt = -1;
		}
	}
	pthread_mutex_unlock(&dev->lock);

	return cnt;
}

/**\brief ??????DVR???
 * \param dev_no DVR?????????
 * \param	src DVR???
 * \return
 *   - AM_SUCCESS ??????
 *   - ????????? ????????????(???am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_SetSource(int dev_no, AM_DVR_Source_t src)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if(!dev->drv->set_source)
	{
		AM_DEBUG(1, "do not support set_source");
		ret = AM_DVR_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_source(dev, src);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_lock(&am_gAdpLock);
		dev->src = src;
		pthread_mutex_unlock(&am_gAdpLock);
	}

	return ret;
}

