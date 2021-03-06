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




#ifndef AMLOGICPLAYERSTREAMSOURCE__HH
#define AMLOGICPLAYERSTREAMSOURCE__HH


#ifdef __cplusplus
extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "AmlogicPlayerStreamSourceListener.h"
namespace android
{

#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/RefBase.h>


class AmlogicPlayerStreamSource : public RefBase
{

public:
    AmlogicPlayerStreamSource(const sp<IStreamSource> &source);
    virtual               ~AmlogicPlayerStreamSource();
    static int            init(void);
    char                *GetPathString();

private:
    static int          amopen(URLContext *h, const char *filename, int flags);
    static int          amread(URLContext *h, unsigned char *buf, int size);
    static int          amwrite(URLContext *h, unsigned char *buf, int size);
    static int64_t    amseek(URLContext *h, int64_t pos, int whence);
    static int          amclose(URLContext *h);
    static int          get_file_handle(URLContext *h);
    ///*-------------------------------------------------------*//
    int              Source_open();
    int              Source_read(unsigned char *buf, int size);
    int              Source_write(unsigned char *buf, int size);
    int64_t        Source_seek(int64_t pos, int whence);
    int              Source_close();

private:
    sp<IStreamSource> mSource;
    sp<AmlogicPlayerStreamSourceListener> mStreamListener;
    char sourcestring[128];
    Mutex mMoreDataLock;
    Condition   mWaitCondition;
    char localbuf[188];
    int    localdatasize;
    int64_t  pos;
    int dropdatalen;

};


}////namespace android
#endif

#endif
