/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef RTSP_SOURCE_H_

#define RTSP_SOURCE_H_

#include "AmNuPlayerSource.h"

#include "AmATSParser.h"

namespace android {

struct ALooper;
struct AReplyToken;
struct AmAnotherPacketSource;
struct MyHandler;
struct SDPLoader;

struct AmNuPlayer::RTSPSource : public AmNuPlayer::Source {
    RTSPSource(
            const sp<AMessage> &notify,
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers,
            bool uidValid = false,
            uid_t uid = 0,
            bool isSDP = false);

    virtual void prepareAsync();
    virtual void start();
    virtual void stop();

    virtual status_t feedMoreTSData();

    virtual status_t dequeueAccessUnit(bool audio, sp<ABuffer> *accessUnit);

    virtual status_t getDuration(int64_t *durationUs);
    virtual status_t seekTo(int64_t seekTimeUs);

    void onMessageReceived(const sp<AMessage> &msg);

protected:
    virtual ~RTSPSource();

    virtual sp<MetaData> getFormatMeta(bool audio);

private:
    enum {
        kWhatNotify          = 'noti',
        kWhatDisconnect      = 'disc',
        kWhatPerformSeek     = 'seek',
        kWhatPollBuffering   = 'poll',
    };

    enum State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        SEEKING,
    };

    enum Flags {
        // Don't log any URLs.
        kFlagIncognito = 1,
    };

    // Buffer Prepare/Underflow/Overflow/Resume Marks
    static const int64_t kPrepareMarkUs;
    static const int64_t kUnderflowMarkUs;
    static const int64_t kOverflowMarkUs;
    static const int64_t kStartServerMarkUs;

    struct TrackInfo {
        sp<AmAnotherPacketSource> mSource;

        int32_t mTimeScale;
        uint32_t mRTPTime;
        int64_t mNormalPlaytimeUs;
        bool mNPTMappingValid;
    };

    sp<IMediaHTTPService> mHTTPService;
    AString mURL;
    KeyedVector<String8, String8> mExtraHeaders;
    bool mUIDValid;
    uid_t mUID;
    uint32_t mFlags;
    bool mIsSDP;
    State mState;
    status_t mFinalResult;
    sp<AReplyToken> mDisconnectReplyID;
    Mutex mBufferingLock;
    bool mBuffering;
    bool mInPreparationPhase;

    sp<ALooper> mLooper;
    sp<MyHandler> mHandler;
    sp<SDPLoader> mSDPLoader;

    Vector<TrackInfo> mTracks;
    sp<AmAnotherPacketSource> mAudioTrack;
    sp<AmAnotherPacketSource> mVideoTrack;

    sp<AmATSParser> mTSParser;

    int32_t mSeekGeneration;

    int64_t mEOSTimeoutAudio;
    int64_t mEOSTimeoutVideo;

    sp<AReplyToken> mSeekReplyID;

    sp<AmAnotherPacketSource> getSource(bool audio);

    void onConnected();
    void onSDPLoaded(const sp<AMessage> &msg);
    void onDisconnected(const sp<AMessage> &msg);
    void finishDisconnectIfPossible();

    void performSeek(int64_t seekTimeUs);
    void schedulePollBuffering();
    void checkBuffering(bool *prepared, bool *underflow, bool *overflow, bool *startServer);
    void onPollBuffering();

    bool haveSufficientDataOnAllTracks();

    void setEOSTimeout(bool audio, int64_t timeout);
    void setError(status_t err);
    void startBufferingIfNecessary();
    bool stopBufferingIfNecessary();
    void finishSeek(status_t err);

    DISALLOW_EVIL_CONSTRUCTORS(RTSPSource);
};

}  // namespace android

#endif  // RTSP_SOURCE_H_
