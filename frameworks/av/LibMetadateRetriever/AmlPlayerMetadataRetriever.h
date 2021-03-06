/*
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AMLPLAYERMETADATARETRIEVER_H
#define ANDROID_AMLPLAYERMETADATARETRIEVER_H

#include <utils/threads.h>
#include <utils/Errors.h>
#include <media/MediaMetadataRetrieverInterface.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>


//#include <player_thumbnail.h>
#include "AmThumbnail/AmThumbnail.h"

namespace android
{

class String8;
class DataSource;

class AmlPlayerMetadataRetriever : public MediaMetadataRetrieverInterface
{
public:
    AmlPlayerMetadataRetriever();
    virtual ~AmlPlayerMetadataRetriever();
    virtual status_t setDataSource(
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers = NULL);
    virtual status_t setDataSource(int fd, int64_t offset, int64_t length);
    virtual status_t setDataSource(const sp<DataSource>& source);
    virtual VideoFrame *getFrameAtTime(int64_t timeUs, int option);
    virtual MediaAlbumArt *extractAlbumArt();
    virtual const char* extractMetadata(int keyCode);

private:
    AmThumbnailInt* mClient;
    sp<DataSource> mSource;

    bool mOpened;
    bool mParsedMetaData;
    KeyedVector<int, String8> mMetaData;
    MediaAlbumArt *mAlbumArt;
    bool mIsSlowMedia;

    status_t setdatasource(const char* url, int fd, int64_t offset, int64_t length);
    void parseMetaData();
    void clearMetadata();
};

}

#endif
