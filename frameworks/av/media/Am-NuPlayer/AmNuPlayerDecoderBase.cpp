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

//#define LOG_NDEBUG 0
#define LOG_TAG "NuPlayerDecoderBase"
#include <utils/Log.h>
#include <inttypes.h>

#include "AmNuPlayerDecoderBase.h"

#include "AmNuPlayerRenderer.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

namespace android {

AmNuPlayer::DecoderBase::DecoderBase(const sp<AMessage> &notify)
    :  mNotify(notify),
       mBufferGeneration(0),
       mPaused(false),
       mStats(new AMessage),
       mRequestInputBuffersPending(false) {
    // Every decoder has its own looper because MediaCodec operations
    // are blocking, but AmNuPlayer needs asynchronous operations.
    mDecoderLooper = new ALooper;
    mDecoderLooper->setName("NPDecoder");
    mDecoderLooper->start(false, false, ANDROID_PRIORITY_AUDIO);
}

AmNuPlayer::DecoderBase::~DecoderBase() {
    mDecoderLooper->unregisterHandler(id());
    mDecoderLooper->stop();
}

static
status_t PostAndAwaitResponse(
        const sp<AMessage> &msg, sp<AMessage> *response) {
    status_t err = msg->postAndAwaitResponse(response);

    if (err != OK) {
        return err;
    }

    if (!(*response)->findInt32("err", &err)) {
        err = OK;
    }

    return err;
}

void AmNuPlayer::DecoderBase::configure(const sp<AMessage> &format) {
    sp<AMessage> msg = new AMessage(kWhatConfigure, this);
    msg->setMessage("format", format);
    msg->post();
}

void AmNuPlayer::DecoderBase::init() {
    mDecoderLooper->registerHandler(this);
}

void AmNuPlayer::DecoderBase::setParameters(const sp<AMessage> &params) {
    sp<AMessage> msg = new AMessage(kWhatSetParameters, this);
    msg->setMessage("params", params);
    msg->post();
}

void AmNuPlayer::DecoderBase::setRenderer(const sp<Renderer> &renderer) {
    sp<AMessage> msg = new AMessage(kWhatSetRenderer, this);
    msg->setObject("renderer", renderer);
    msg->post();
}

void AmNuPlayer::DecoderBase::pause() {
    sp<AMessage> msg = new AMessage(kWhatPause, this);

    sp<AMessage> response;
    PostAndAwaitResponse(msg, &response);
}

status_t AmNuPlayer::DecoderBase::getInputBuffers(Vector<sp<ABuffer> > *buffers) const {
    sp<AMessage> msg = new AMessage(kWhatGetInputBuffers, this);
    msg->setPointer("buffers", buffers);

    sp<AMessage> response;
    return PostAndAwaitResponse(msg, &response);
}

void AmNuPlayer::DecoderBase::signalFlush() {
    (new AMessage(kWhatFlush, this))->post();
}

void AmNuPlayer::DecoderBase::signalResume(bool notifyComplete) {
    sp<AMessage> msg = new AMessage(kWhatResume, this);
    msg->setInt32("notifyComplete", notifyComplete);
    msg->post();
}

void AmNuPlayer::DecoderBase::initiateShutdown() {
    (new AMessage(kWhatShutdown, this))->post();
}

void AmNuPlayer::DecoderBase::onRequestInputBuffers() {
    if (mRequestInputBuffersPending) {
        return;
    }

    // doRequestBuffers() return true if we should request more data
    if (doRequestBuffers()) {
        mRequestInputBuffersPending = true;

        sp<AMessage> msg = new AMessage(kWhatRequestInputBuffers, this);
        msg->post(10 * 1000ll);
    }
}

void AmNuPlayer::DecoderBase::onMessageReceived(const sp<AMessage> &msg) {

    switch (msg->what()) {
        case kWhatConfigure:
        {
            sp<AMessage> format;
            CHECK(msg->findMessage("format", &format));
            onConfigure(format);
            break;
        }

        case kWhatSetParameters:
        {
            sp<AMessage> params;
            CHECK(msg->findMessage("params", &params));
            onSetParameters(params);
            break;
        }

        case kWhatSetRenderer:
        {
            sp<RefBase> obj;
            CHECK(msg->findObject("renderer", &obj));
            onSetRenderer(static_cast<Renderer *>(obj.get()));
            break;
        }

        case kWhatPause:
        {
            sp<AReplyToken> replyID;
            CHECK(msg->senderAwaitsResponse(&replyID));

            mPaused = true;

            (new AMessage)->postReply(replyID);
            break;
        }

        case kWhatGetInputBuffers:
        {
            sp<AReplyToken> replyID;
            CHECK(msg->senderAwaitsResponse(&replyID));

            Vector<sp<ABuffer> > *dstBuffers;
            CHECK(msg->findPointer("buffers", (void **)&dstBuffers));

            onGetInputBuffers(dstBuffers);

            (new AMessage)->postReply(replyID);
            break;
        }

        case kWhatRequestInputBuffers:
        {
            mRequestInputBuffersPending = false;
            onRequestInputBuffers();
            break;
        }

        case kWhatFlush:
        {
            onFlush();
            break;
        }

        case kWhatResume:
        {
            int32_t notifyComplete;
            CHECK(msg->findInt32("notifyComplete", &notifyComplete));

            onResume(notifyComplete);
            break;
        }

        case kWhatShutdown:
        {
            onShutdown(true);
            break;
        }

        default:
            TRESPASS();
            break;
    }
}

void AmNuPlayer::DecoderBase::handleError(int32_t err)
{
    // We cannot immediately release the codec due to buffers still outstanding
    // in the renderer.  We signal to the player the error so it can shutdown/release the
    // decoder after flushing and increment the generation to discard unnecessary messages.

    ++mBufferGeneration;

    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", kWhatError);
    notify->setInt32("err", err);
    notify->post();
}

}  // namespace android

