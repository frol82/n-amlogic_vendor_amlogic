/*
 * Copyright (C) 2010 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#include <cutils/properties.h>
//#include <gui/SurfaceTextureClient.h>
#include <media/ICrypto.h>

#define LOG_TAG "MediaConvertorTest"


#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AHandler.h>

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>

//#include <MetadataBufferType.h>

#include <ui/GraphicBuffer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/IGraphicBufferAlloc.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>

#include "esconvertor.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/videodev2.h>
#include <hardware/hardware.h>
#include <hardware/aml_screen.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>


int main(int argc, char **argv) {
    using namespace android;
    int err;
    int isAudio;
    int dump_time = 0, video_dump_size = 0, audio_dump_size = 0;
    MediaBuffer *tVideoBuffer, *tAudioBuffer;

    ProcessState::self()->startThreadPool();
    int  video_file   = open("/data/temp/video.es", O_CREAT | O_RDWR, 0666);
    ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
    sp<ESConvertor> mH264Convertor = new ESConvertor(0);

    //mH264Convertor->setVideoCrop(100,100,400,300);
    MetaData* pMeta;
    pMeta = new MetaData();
    pMeta->setInt32(kKeyWidth, 1280);
    pMeta->setInt32(kKeyHeight, 720);
    pMeta->setInt32(kKeyFrameRate, 30);
    pMeta->setInt32(kKeyBitRate, 1000000);

    err = mH264Convertor->start(pMeta);
    if (err != OK) {
        ALOGE("[%s %d]\n", __FUNCTION__, __LINE__);
        return OK;
    }

    while (1) {
	    tVideoBuffer = NULL;
        err = mH264Convertor->read(&tVideoBuffer);

        if (err != OK) {
            usleep(100);
            continue;
        }
        ALOGE("[%s %d] video read size:%d dump_time:%d\n", __FUNCTION__, __LINE__, tVideoBuffer->range_length(), dump_time);

        dump_time++;

        write(video_file, tVideoBuffer->data(), tVideoBuffer->range_length());
        video_dump_size += tVideoBuffer->range_length();
        ALOGE("[%s %d] video dump_time:%d size:%d dump_size:%d\n", __FUNCTION__, __LINE__, dump_time, tVideoBuffer->range_length(), video_dump_size);

        tVideoBuffer->release();
        tVideoBuffer = NULL;

        if (dump_time > 1000)
            break;
    }

    ALOGE("mH264Convertor stop\n");

    mH264Convertor->stop();
    close(video_file);

    ALOGE("file close\n");
    return 0;
}

