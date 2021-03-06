/*
 * PMP demuxer.
 * Copyright (c) 2011 Reimar Döffinger
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/intreadwrite.h"
#include "avformat.h"

typedef struct {
    int cur_stream;
    int num_streams;
    int audio_packets;
    int current_packet;
    uint32_t *packet_sizes;
    int packet_sizes_alloc;
} PMPContext;

static int pmp_probe(AVProbeData *p)
{
    if (AV_RN32(p->buf) == AV_RN32("pmpm") &&
        AV_RL32(p->buf + 4) == 1) {
        return AVPROBE_SCORE_MAX;
    }
    return 0;
}

static int pmp_header(AVFormatContext *s, AVFormatParameters *ap)
{
    PMPContext *pmp = s->priv_data;
    AVIOContext *pb = s->pb;
    int tb_num, tb_den;
    int index_cnt;
    int audio_codec_id = CODEC_ID_NONE;
    int srate, channels;
    int i;
    uint64_t pos;
    AVStream *vst = av_new_stream(s, 0);
    if (!vst) {
        return AVERROR(ENOMEM);
    }
    vst->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    avio_skip(pb, 8);
    switch (avio_rl32(pb)) {
    case 0:
        vst->codec->codec_id = CODEC_ID_MPEG4;
        break;
    case 1:
        vst->codec->codec_id = CODEC_ID_H264;
        break;
    default:
        av_log(s, AV_LOG_ERROR, "Unsupported video format\n");
        break;
    }
    index_cnt = avio_rl32(pb);
    vst->codec->width  = avio_rl32(pb);
    vst->codec->height = avio_rl32(pb);

    tb_num = avio_rl32(pb);
    tb_den = avio_rl32(pb);
    av_set_pts_info(vst, 32, tb_num, tb_den);
    vst->nb_frames = index_cnt;
    vst->duration = index_cnt;

    switch (avio_rl32(pb)) {
    case 0:
        audio_codec_id = CODEC_ID_MP3;
        break;
    case 1:
        av_log(s, AV_LOG_ERROR, "AAC not yet correctly supported\n");
        audio_codec_id = CODEC_ID_AAC;
        break;
    default:
        av_log(s, AV_LOG_ERROR, "Unsupported audio format\n");
        break;
    }
    pmp->num_streams = avio_rl16(pb) + 1;
    avio_skip(pb, 10);
    srate = avio_rl32(pb);
    channels = avio_rl32(pb) + 1;
    for (i = 1; i < pmp->num_streams; i++) {
        AVStream *ast = av_new_stream(s, i);
        if (!ast) {
            return AVERROR(ENOMEM);
        }
        ast->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        ast->codec->codec_id = audio_codec_id;
        ast->codec->channels = channels;
        ast->codec->sample_rate = srate;

        if (audio_codec_id == CODEC_ID_AAC) { // William workaround : set aac profile value as a default
            ast->codec->audio_profile = 2;
        }

        av_set_pts_info(ast, 32, 1, srate);
    }
    pos = avio_tell(pb) + 4 * index_cnt;
    for (i = 0; i < index_cnt; i++) {
        int size = avio_rl32(pb);
        int flags = size & 1 ? AVINDEX_KEYFRAME : 0;
        size >>= 1;
        av_add_index_entry(vst, pos, i, size, 0, flags);
        pos += size;
    }
    return 0;
}

#define PMP_PTS_FREQ 90000
static int pmp_packet(AVFormatContext *s, AVPacket *pkt)
{
    PMPContext *pmp = s->priv_data;
    AVIOContext *pb = s->pb;
    int ret = 0;
    int i;

    if (url_feof(pb)) {
        return AVERROR_EOF;
    }
    if (pmp->cur_stream == 0) {
        int num_packets;
        pmp->audio_packets = avio_r8(pb);
        if (!pmp->audio_packets) {
            av_log_ask_for_sample(s, "0 audio packets\n");
            return AVERROR_EOF;
            //return AVERROR_PATCHWELCOME;
        }
        num_packets = (pmp->num_streams - 1) * pmp->audio_packets + 1;
        avio_skip(pb, 8);
        pmp->current_packet = 0;
        av_fast_malloc(&pmp->packet_sizes,
                       &pmp->packet_sizes_alloc,
                       num_packets * sizeof(*pmp->packet_sizes));
        if (!pmp->packet_sizes_alloc) {
            av_log(s, AV_LOG_ERROR, "Cannot (re)allocate packet buffer\n");
            return AVERROR(ENOMEM);
        }
        for (i = 0; i < num_packets; i++) {
            pmp->packet_sizes[i] = avio_rl32(pb);
        }
    }
    ret = av_get_packet(pb, pkt, pmp->packet_sizes[pmp->current_packet]);
    if (ret >= 0) {
        ret = 0;
        // FIXME: this is a hack that should be remove once
        // compute_pkt_fields can handle
        if (pmp->cur_stream == 0) {
            pkt->dts = s->streams[0]->cur_dts++;
        } else {
            /* PMP media data format :  |Video-audio-audio ** |Video-audio-audio -**  |Video-audio-audio**|
            * Video has dts-pts ,but audio not have
            * So,we need to rebuild audio pts, only set first audio pts after video packeage
            */
            if (pmp->current_packet % pmp->audio_packets == 1) {
                double pts_video_cur = (PMP_PTS_FREQ * (double)(s->streams[0]->cur_dts - 1) * s->streams[0]->time_base.num / s->streams[0]->time_base.den);
                double pts_audio_each = (PMP_PTS_FREQ * (double)(s->streams[pmp->cur_stream]->time_base.num) / s->streams[pmp->cur_stream]->time_base.den);
                pkt->pts = (int64_t)(pts_video_cur / pts_audio_each);
                //av_log(s,NULL," pts_video_cur:%lf  pts_audio_each:%lf pts_audio:%ld",pts_video_cur,pts_audio_each,(int64_t)(pkt->pts*pts_audio_each));
            }
        }

        pkt->stream_index = pmp->cur_stream;
    }
    if (pmp->current_packet % pmp->audio_packets == 0) {
        pmp->cur_stream = (pmp->cur_stream + 1) % pmp->num_streams;
    }
    pmp->current_packet++;
    return ret;
}

static int pmp_seek(AVFormatContext *s, int stream_index,
                    int64_t ts, int flags)
{
    PMPContext *pmp = s->priv_data;
    pmp->cur_stream = 0;
    // fallback to default seek now
    flags = flags & AVSEEK_FLAG_BACKWARD;
    AVStream *st = s->streams[pmp->cur_stream];
    int index = av_index_search_timestamp(st, ts, flags);
    int64_t pos = -1;
    //av_log(s,NULL,"search ok,ts:%lld index:%d \n",ts,index);
    if (index < 0) {
        return -1;
    }
    st->cur_dts = index;
    pos = st->index_entries[index].pos;
    avio_seek(s->pb, pos, SEEK_SET);
    return 0;
}

static int pmp_close(AVFormatContext *s)
{
    PMPContext *pmp = s->priv_data;
    av_freep(&pmp->packet_sizes);
    return 0;
}

AVInputFormat ff_pmp_demuxer = {
    .name           = "pmp",
    .long_name      = NULL_IF_CONFIG_SMALL("Playstation Portable PMP format"),
    .priv_data_size = sizeof(PMPContext),
    .read_probe     = pmp_probe,
    .read_header    = pmp_header,
    .read_packet    = pmp_packet,
    .read_seek      = pmp_seek,
    .read_close     = pmp_close,
};
