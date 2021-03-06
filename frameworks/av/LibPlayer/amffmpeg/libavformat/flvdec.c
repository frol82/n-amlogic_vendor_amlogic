/*
 * FLV demuxer
 * Copyright (c) 2003 The FFmpeg Project
 *
 * This demuxer will generate a 1 byte extradata for VP6F content.
 * It is composed of:
 *  - upper 4bits: difference between encoded width and visible width
 *  - lower 4bits: difference between encoded height and visible height
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

#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "libavcodec/bytestream.h"
#include "libavcodec/mpeg4audio.h"
#include "avformat.h"
#include "avio_internal.h"
#include "flv.h"
#include <stdio.h>
#define VALIDATE_INDEX_TS_THRESH 2500

typedef struct {
    int wrong_dts; ///< wrong dts due to negative cts
    struct {
        int64_t dts;
        int64_t pos;
    } validate_index[2];
    int validate_next;
    int validate_count;
    int first_hevc_packet; // hack
} FLVContext;

static int flv_probe(AVProbeData *p)
{
    const uint8_t *d;

    d = p->buf;
    if (d[0] == 'F' && d[1] == 'L' && d[2] == 'V' && d[3] < 5 && d[5] == 0 && AV_RB32(d + 5) > 8) {
        return AVPROBE_SCORE_MAX;
    }
    return 0;
}

static void flv_set_audio_codec(AVFormatContext *s, AVStream *astream, int flv_codecid)
{
    AVCodecContext *acodec = astream->codec;
    switch (flv_codecid) {
    //no distinction between S16 and S8 PCM codec flags
    case FLV_CODECID_PCM:
        acodec->codec_id = acodec->bits_per_coded_sample == 8 ? CODEC_ID_PCM_U8 :
#if HAVE_BIGENDIAN
                           CODEC_ID_PCM_S16BE;
#else
                           CODEC_ID_PCM_S16LE;
#endif
        break;
    case FLV_CODECID_PCM_LE:
        acodec->codec_id = acodec->bits_per_coded_sample == 8 ? CODEC_ID_PCM_U8 : CODEC_ID_PCM_S16LE;
        break;
    case FLV_CODECID_AAC  :
        acodec->codec_id = CODEC_ID_AAC;
        break;
    case FLV_CODECID_ADPCM:
        acodec->codec_id = CODEC_ID_ADPCM_SWF;
        break;
    case FLV_CODECID_SPEEX:
        acodec->codec_id = CODEC_ID_SPEEX;
        acodec->sample_rate = 16000;
        break;
    case FLV_CODECID_MP3  :
        acodec->codec_id = CODEC_ID_MP3      ;
        astream->need_parsing = AVSTREAM_PARSE_FULL;
        break;
    case FLV_CODECID_NELLYMOSER_8KHZ_MONO:
        acodec->sample_rate = 8000; //in case metadata does not otherwise declare samplerate
        acodec->codec_id = CODEC_ID_NELLYMOSER;
        break;
    case FLV_CODECID_NELLYMOSER_16KHZ_MONO:
        acodec->sample_rate = 16000;
        acodec->codec_id = CODEC_ID_NELLYMOSER;
        break;
    case FLV_CODECID_NELLYMOSER:
        acodec->codec_id = CODEC_ID_NELLYMOSER;
        break;
    default:
        av_log(s, AV_LOG_INFO, "Unsupported audio codec (%x)\n", flv_codecid >> FLV_AUDIO_CODECID_OFFSET);
        acodec->codec_tag = flv_codecid >> FLV_AUDIO_CODECID_OFFSET;
    }
}

static int flv_set_video_codec(AVFormatContext *s, AVStream *vstream, int flv_codecid)
{
    AVCodecContext *vcodec = vstream->codec;
    switch (flv_codecid) {
    case FLV_CODECID_H263  :
        vcodec->codec_id = CODEC_ID_FLV1   ;
        break;
    case FLV_CODECID_SCREEN:
        vcodec->codec_id = CODEC_ID_FLASHSV;
        break;
    case FLV_CODECID_SCREEN2:
        vcodec->codec_id = CODEC_ID_FLASHSV2;
        break;
    case FLV_CODECID_VP6   :
        vcodec->codec_id = CODEC_ID_VP6F   ;
    case FLV_CODECID_VP6A  :
        if (flv_codecid == FLV_CODECID_VP6A) {
            vcodec->codec_id = CODEC_ID_VP6A;
        }
        if (vcodec->extradata_size != 1) {
            vcodec->extradata_size = 1;
            vcodec->extradata = av_malloc(1);
        }
        vcodec->extradata[0] = avio_r8(s->pb);
        return 1; // 1 byte body size adjustment for flv_read_packet()
    case FLV_CODECID_H264:
        vstream->need_parsing = AVSTREAM_PARSE_HEADERS;
        vcodec->codec_id = CODEC_ID_H264;
        return 3; // not 4, reading packet type will consume one byte
    case FLV_CODECID_HM91:
    case FLV_CODECID_HM10:
    case FLV_CODECID_HM12:
        vcodec->codec_id = CODEC_ID_HEVC;
        return 3;
    default:
        av_log(s, AV_LOG_INFO, "Unsupported video codec (%x)\n", flv_codecid);
        vcodec->codec_tag = flv_codecid;
    }

    return 0;
}

static int amf_get_string(AVIOContext *ioc, char *buffer, int buffsize)
{
    int length = avio_rb16(ioc);
    if (length >= buffsize) {
        avio_skip(ioc, length);
        return -1;
    }

    avio_read(ioc, buffer, length);

    buffer[length] = '\0';

    return length;
}

static int parse_keyframes_index(AVFormatContext *s, AVIOContext *ioc, AVStream *vstream, int64_t max_pos)
{
    FLVContext *flv = s->priv_data;
    unsigned int timeslen = 0, fileposlen = 0, i;
    char str_val[256];
    int64_t *times = NULL;
    int64_t *filepositions = NULL;
    int ret = AVERROR(ENOSYS);
    int64_t initial_pos = avio_tell(ioc);
    if (vstream->nb_index_entries > 0) {
        av_log(s, AV_LOG_WARNING, "Skiping duplicate index\n");
        return 0;
    }

    if (s->flags & AVFMT_FLAG_IGNIDX) {
        return 0;
    }

    while (avio_tell(ioc) < max_pos - 2 && amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
        int64_t** current_array;
        unsigned int arraylen;

        // Expect array object in context
        if (avio_r8(ioc) != AMF_DATA_TYPE_ARRAY) {
            break;
        }

        arraylen = avio_rb32(ioc);
        if (arraylen >> 28) {
            break;
        }

        if (!strcmp(KEYFRAMES_TIMESTAMP_TAG , str_val) && !times) {
            current_array = &times;
            timeslen = arraylen;
        } else if (!strcmp(KEYFRAMES_BYTEOFFSET_TAG, str_val) && !filepositions) {
            current_array = &filepositions;
            fileposlen = arraylen;
        } else { // unexpected metatag inside keyframes, will not use such metadata for indexing
            break;
        }

        if (!(*current_array = av_mallocz(sizeof(**current_array) * arraylen))) {
            ret = AVERROR(ENOMEM);
            goto finish;
        }

        for (i = 0; i < arraylen && avio_tell(ioc) < max_pos - 1; i++) {
            if (avio_r8(ioc) != AMF_DATA_TYPE_NUMBER) {
                goto finish;
            }
            current_array[0][i] = av_int2dbl(avio_rb64(ioc));
        }
        if (times && filepositions) {
            // All done, exiting at a position allowing amf_parse_object
            // to finish parsing the object
            ret = 0;
            break;
        }
    }

    if (timeslen == fileposlen && fileposlen > 1 && max_pos <= filepositions[0]) {
        for (i = 0; i < fileposlen; i++) {
            av_add_index_entry(vstream, filepositions[i], times[i] * 1000,
                               0, 0, AVINDEX_KEYFRAME);
            if (i < 2) {
                flv->validate_index[i].pos = filepositions[i];
                flv->validate_index[i].dts = times[i] * 1000;
                flv->validate_count = i + 1;
            }
        }
        // Here update duration
        if (i > 10) {
            int64_t pts_first = times[0] * 1000;
            int64_t pts_pre_last = times[i - 2] * 1000;
            int64_t pts_last = times[i - 1] * 1000;
            int num = vstream->time_base.num;
            int den = vstream->time_base.den;
            double exchange = 90000 * ((double)num / (double)den);
            int64_t duration_1 = ((int64_t)(exchange * (pts_pre_last - pts_first))) / 90000;
            int64_t duration_2 = ((int64_t)(exchange * (pts_last - pts_first))) / 90000;
            if (duration_1 > 0 && duration_2 > 0 && s->duration != AV_NOPTS_VALUE) {
                int64_t diff_s = llabs(duration_2 - duration_1);
                if (diff_s > 600) { // diff exceed 10min use duration_1
                    if (s->duration > 0 && llabs(s->duration / AV_TIME_BASE - duration_1) > 600) {
                        s->duration = duration_1 * AV_TIME_BASE;
                    }
                } else {
                    if (s->duration > 0 && llabs(s->duration / AV_TIME_BASE - duration_2) > 600) {
                        s->duration = duration_2 * AV_TIME_BASE;
                    }
                }
            }
            av_log(NULL, AV_LOG_INFO, "Take chance to update duration: %lld %lld\n", duration_1, duration_2);
        }
    } else {
invalid:
        av_log(s, AV_LOG_WARNING, "Invalid keyframes object, skipping.\n");
    }

finish:
    av_freep(&times);
    av_freep(&filepositions);
    avio_seek(ioc, initial_pos, SEEK_SET);
    return ret;
}

static int get_sstrtoi(char *datastr)
{
    char *p, *q;
    int i;
    int value = 0;

    p = strstr(datastr, ":");
    p = strstr(p, "\"");
    p++;
    if (p == NULL) {
        return 0;
    }
    q = strstr(p, "\"");
    if (q == NULL) {
        return 0;
    }

    i = (int)(q - p);
    if (i <= 0) {
        return 0;
    }

    while (i > 0) {
        value = value * 10 + *p - '0';
        p++;
        i--;
    }

    return value;
}

static int amf_parse_object(AVFormatContext *s, AVStream *astream, AVStream *vstream, const char *key, int64_t max_pos, int depth)
{
    AVCodecContext *acodec, *vcodec;
    AVIOContext *ioc;
    AMFDataType amf_type;
    char str_val[256];
    double num_val;
    unsigned int temp32;

    num_val = 0;
    ioc = s->pb;

    amf_type = avio_r8(ioc);

    switch (amf_type) {
    case AMF_DATA_TYPE_NUMBER:
        num_val = av_int2dbl(avio_rb64(ioc));
        break;
    case AMF_DATA_TYPE_BOOL:
        num_val = avio_r8(ioc);
        break;
    case AMF_DATA_TYPE_STRING:
        if (amf_get_string(ioc, str_val, sizeof(str_val)) < 0) {
            return -1;
        }
        break;
    case AMF_DATA_TYPE_LONG_STRING:
        temp32 = avio_rb32(ioc);
        av_log(NULL, AV_LOG_ERROR, "long string length %d\n", temp32);
        avio_skip(ioc, temp32);
        break;
    case AMF_DATA_TYPE_OBJECT: {
        unsigned int keylen;

        if (ioc->seekable && key && !strcmp(KEYFRAMES_TAG, key) && depth == 1)
            if (parse_keyframes_index(s, ioc, vstream, max_pos) < 0) {
                av_log(s, AV_LOG_ERROR, "Keyframe index parsing failed\n");
            }

        while (avio_tell(ioc) < max_pos - 2 && (keylen = avio_rb16(ioc))) {
            avio_skip(ioc, keylen); //skip key string
            if (amf_parse_object(s, NULL, NULL, NULL, max_pos, depth + 1) < 0) {
                return -1;    //if we couldn't skip, bomb out.
            }
        }
        if (avio_r8(ioc) != AMF_END_OF_OBJECT) {
            return -1;
        }
    }
    break;
    case AMF_DATA_TYPE_NULL:
    case AMF_DATA_TYPE_UNDEFINED:
    case AMF_DATA_TYPE_UNSUPPORTED:
        break; //these take up no additional space
    case AMF_DATA_TYPE_MIXEDARRAY:
        avio_skip(ioc, 4); //skip 32-bit max array index
        while (avio_tell(ioc) < max_pos - 2 && amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
            //this is the only case in which we would want a nested parse to not skip over the object
            if (amf_parse_object(s, astream, vstream, str_val, max_pos, depth + 1) < 0) {
                return -1;
            }
        }
        if (avio_r8(ioc) != AMF_END_OF_OBJECT) {
            return -1;
        }
        break;
    case AMF_DATA_TYPE_ARRAY: {
        unsigned int arraylen, i;

        arraylen = avio_rb32(ioc);
        for (i = 0; i < arraylen && avio_tell(ioc) < max_pos - 1; i++) {
            if (amf_parse_object(s, NULL, NULL, NULL, max_pos, depth + 1) < 0) {
                return -1;    //if we couldn't skip, bomb out.
            }
        }
    }
    break;
    case AMF_DATA_TYPE_DATE:
        avio_skip(ioc, 8 + 2); //timestamp (double) and UTC offset (int16)
        break;
    default: //unsupported type, we couldn't skip
        return -1;
    }

    if (depth == 1 && key) { //only look for metadata values when we are not nested and key != NULL
        acodec = astream ? astream->codec : NULL;
        vcodec = vstream ? vstream->codec : NULL;

        if (amf_type == AMF_DATA_TYPE_BOOL) {
            av_strlcpy(str_val, num_val > 0 ? "true" : "false", sizeof(str_val));
            av_dict_set(&s->metadata, key, str_val, 0);
        } else if (amf_type == AMF_DATA_TYPE_NUMBER) {
            snprintf(str_val, sizeof(str_val), "%.f", num_val);
            av_dict_set(&s->metadata, key, str_val, 0);
            if (!strcmp(key, "duration")) {
                s->duration = num_val * AV_TIME_BASE;
            } else if (!strcmp(key, "videodatarate") && vcodec && 0 <= (int)(num_val * 1024.0)) {
                vcodec->bit_rate = num_val * 1024.0;
            } else if (!strcmp(key, "audiodatarate") && acodec && 0 <= (int)(num_val * 1024.0)) {
                acodec->bit_rate = num_val * 1024.0;
            } else if (!strcmp(key, "audiosamplerate")) {
                acodec->sample_rate = (float)num_val;
            } else if (!strcmp(key, "stereo")) {
                acodec->channels = (num_val > 0) ? 2 : 1;
            } else if (!strcmp(key, "framerate")) {
                vstream->special_fps = (float)num_val;
            } else if (!strcmp(key, "width")) {
                vcodec->width = (float)num_val;
            } else if (!strcmp(key, "height")) {
                vcodec->height = (float)num_val;
            }
        } else if (amf_type == AMF_DATA_TYPE_STRING) {
            av_dict_set(&s->metadata, key, str_val, 0);
            if (!strcmp(key, "creator")) {  // this is special for #90533
                char *data_tmp;
                av_log(NULL, AV_LOG_INFO, "[%s:%d]special creator\n", __FUNCTION__, __LINE__);
                data_tmp = strstr(str_val, "width");
                if (data_tmp != NULL) {
                    vcodec->width = get_sstrtoi(data_tmp);
                }

                data_tmp = strstr(str_val, "height");
                if (data_tmp != NULL) {
                    vcodec->height = get_sstrtoi(data_tmp);
                }
            }
        }
    }

    return 0;
}

static int flv_read_metabody(AVFormatContext *s, int64_t next_pos)
{
    AMFDataType type;
    AVStream *stream, *astream, *vstream;
    AVIOContext *ioc;
    int i;
    char buffer[11]; //only needs to hold the string "onMetaData". Anything longer is something we don't want.

    astream = NULL;
    vstream = NULL;
    ioc = s->pb;

    //first object needs to be "onMetaData" string
    type = avio_r8(ioc);
    if (type != AMF_DATA_TYPE_STRING || amf_get_string(ioc, buffer, sizeof(buffer)) < 0 || strcmp(buffer, "onMetaData")) {
        return -1;
    }

    //find the streams now so that amf_parse_object doesn't need to do the lookup every time it is called.
    for (i = 0; i < s->nb_streams; i++) {
        stream = s->streams[i];
        if (stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            astream = stream;
        } else if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vstream = stream;
        }
    }

    //parse the second object (we want a mixed array)
    if (amf_parse_object(s, astream, vstream, buffer, next_pos, 0) < 0) {
        return -1;
    }

    return 0;
}

static AVStream *create_stream(AVFormatContext *s, int is_audio)
{
    AVStream *st = av_new_stream(s, is_audio);
    if (!st) {
        return NULL;
    }
    st->codec->codec_type = is_audio ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;
    av_set_pts_info(st, 32, 1, 1000); /* 32 bit pts in ms */
    return st;
}

static int flv_read_avcodec_info(AVFormatContext *s)
{
    AVStream *ast = NULL, *vst = NULL, *st;
    int64_t old_offset, next;
    int type, size, info;
    int i = 0;

    if ((!s->seekable) || (!s->support_seek)) {
        return 0;
    }

    /* find av stream */
    do {
        st = s->streams[i];
        if (st->id == 0 && vst == NULL) {
            vst = st;
        }
        if (st->id == 1 && ast == NULL) {
            ast = st;
        }
        i ++;
    } while (i < s->nb_streams && (ast == NULL || vst == NULL));

    old_offset = avio_tell(s->pb);

    int find_stream_nb = 0, audio_flag = 0, video_flag = 0;
    do {
        type = avio_r8(s->pb);
        size = avio_rb24(s->pb);
        avio_skip(s->pb, 7); /* dts:4bytes streamid:3bytes */

        if (url_feof(s->pb)) {
            return AVERROR_EOF;
        }

        if (size == 0) {
            continue;
        }

        next = size + avio_tell(s->pb);
        if (type == FLV_TAG_TYPE_VIDEO) {
            if (!vst) {
                if (!create_stream(s, 0)) {
                    return AVERROR(ENOMEM);
                }
                vst = s->streams[0];
            }
            if (vst && vst->codec->codec_id == 0) {
                info = avio_r8(s->pb);  //get video info
                if ((info & 0xf0) == 0x50) { /* video info / command frame */
                    goto skip;
                }
                flv_set_video_codec(s, vst, info & FLV_VIDEO_CODECID_MASK);
                av_log(s, AV_LOG_INFO, "[%s]vst->codec->codec_id =%x\n", __FUNCTION__, vst->codec->codec_id);
            }
        } else if (type == FLV_TAG_TYPE_AUDIO) {
            if (!ast) {
                if (!create_stream(s, 1)) {
                    return AVERROR(ENOMEM);
                }
                ast = s->streams[1];
            }
            if (ast && ast->codec->codec_id == 0) {
                info = avio_r8(s->pb);  //get audio info
                flv_set_audio_codec(s, ast, info & FLV_AUDIO_CODECID_MASK);
                av_log(s, AV_LOG_INFO, "[%s]ast->codec->codec_id =%x\n", __FUNCTION__, ast->codec->codec_id);
            }
        }

        if (!video_flag && vst && vst->codec->codec_id != 0) {
            find_stream_nb++;
            video_flag = 1;
        }
        if (!audio_flag && ast && ast->codec->codec_id != 0) {
            find_stream_nb++;
            audio_flag = 1;
        }
        if (find_stream_nb == s->nb_streams) {
            break;
        }

skip:
        avio_seek(s->pb, next, SEEK_SET);
        avio_skip(s->pb, 4);
        continue;
    } while (1);
    avio_seek(s->pb, old_offset, SEEK_SET);
    return 0;
}

static int flv_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    int offset, flags;

    avio_skip(s->pb, 4);
    flags = avio_r8(s->pb);
    /* old flvtool cleared this field */
    /* FIXME: better fix needed */
    if (!flags) {
        flags = FLV_HEADER_FLAG_HASVIDEO | FLV_HEADER_FLAG_HASAUDIO;
        av_log(s, AV_LOG_WARNING, "Broken FLV file, which says no streams present, this might fail\n");
    }

    if ((flags & (FLV_HEADER_FLAG_HASVIDEO | FLV_HEADER_FLAG_HASAUDIO))
        == 0) {
        s->ctx_flags |= AVFMTCTX_NOHEADER;
    }

    if (flags & FLV_HEADER_FLAG_HASVIDEO) {
        if (!create_stream(s, 0)) {
            return AVERROR(ENOMEM);
        }
    }
    if (flags & FLV_HEADER_FLAG_HASAUDIO) {
        if (!create_stream(s, 1)) {
            return AVERROR(ENOMEM);
        }
    }

    offset = avio_rb32(s->pb);
    avio_seek(s->pb, offset, SEEK_SET);
    avio_skip(s->pb, 4);

    s->start_time = 0;

    flv_read_avcodec_info(s);
    return 0;
}

static int flv_get_extradata(AVFormatContext *s, AVStream *st, int size)
{
    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata) {
        return AVERROR(ENOMEM);
    }
    st->codec->extradata_size = size;
    avio_read(s->pb, st->codec->extradata, st->codec->extradata_size);
    return 0;
}

/***************** defined for lentoid hevc ************************/
static const uint8_t nal_start_code[] = {0x00, 0x00, 0x00, 0x01};
static void flv_extradata_process(AVStream *st)
{
    int len = 1024, offset = 6;
    uint8_t * buf = av_malloc(len);
    uint8_t * buf_t = buf;
    uint8_t * ptr = st->codec->extradata;
    int size_0, size_1, size;
    len = 0;
    int count = 0;
    while (st->codec->extradata_size - offset > 0) {
        count++;
        size_0 = (*(ptr + offset) & 0xFF) * 0x100;
        size_1 = *(ptr + offset + 1) & 0xFF;
        size = size_0 + size_1;
        if (count == 2) {
            size += 1;    // very ugly
        }
        memcpy(buf_t, nal_start_code, 4);
        buf_t += 4;
        memcpy(buf_t, ptr + offset + 2, size);
        buf_t += size;
        offset = offset + size + 2;
        len = len + size + 4;
    }
    st->codec->extradata = av_realloc(st->codec->extradata, len);
    memcpy(st->codec->extradata, buf, len);
    st->codec->extradata_size = len;
    av_free(buf);
}

static int flv_get_hevc_packet(AVFormatContext *s, AVPacket *pkt, int size)
{
    int len = 0;
    int ret = av_new_packet(pkt, size);
    if (ret < 0) {
        return ret;
    }
    int offset = 0;
    while (size - offset > 0) {
        len = avio_rb32(s->pb);
        memcpy(pkt->data + offset, nal_start_code, 4);
        len = avio_read(s->pb, pkt->data + offset + 4, len);
        offset = offset + len + 4;
    }
    return 0;
}
/***************** defined for lentoid hevc ************************/

static void clear_index_entries(AVFormatContext *s, int64_t pos)
{
    int i, j, out;
    av_log(s, AV_LOG_WARNING, "Found invalid index entries, clearing the index.\n");
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        /* Remove all index entries that point to >= pos */
        out = 0;
        for (j = 0; j < st->nb_index_entries; j++) {
            if (st->index_entries[j].pos < pos) {
                st->index_entries[out++] = st->index_entries[j];
            }
        }
        st->nb_index_entries = out;
    }
}
static int flv_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    FLVContext *flv = s->priv_data;
    int ret, i, type, size, flags, is_audio;
    int64_t next, pos;
    int64_t dts, pts = AV_NOPTS_VALUE;
    AVStream *st = NULL;

    for (;; avio_skip(s->pb, 4)) { /* pkt size is repeated at end. skip it */
        pos = avio_tell(s->pb);
        type = avio_r8(s->pb);
        size = avio_rb24(s->pb);
        dts = avio_rb24(s->pb);
        dts |= avio_r8(s->pb) << 24;
        av_dlog(s, "type:%d, size:%d, dts:%"PRId64"\n", type, size, dts);
        if (url_feof(s->pb)) {
            return AVERROR_EOF;
        }
        if (url_interrupt_cb()) {
            return AVERROR_EXIT;
        }
        avio_skip(s->pb, 3); /* stream id, always 0 */
        flags = 0;

        if (flv->validate_next < flv->validate_count) {
            int64_t validate_pos = flv->validate_index[flv->validate_next].pos;
            if (pos == validate_pos) {
                if (FFABS(dts - flv->validate_index[flv->validate_next].dts) <=
                    VALIDATE_INDEX_TS_THRESH) {
                    flv->validate_next++;
                } else {
                    clear_index_entries(s, validate_pos);
                    flv->validate_count = 0;
                }
            } else if (pos > validate_pos) {
                clear_index_entries(s, validate_pos);
                flv->validate_count = 0;
            }
        }
        if (size == 0) {
            continue;
        }

        next = size + avio_tell(s->pb);

        if (type == FLV_TAG_TYPE_AUDIO) {
            is_audio = 1;
            flags = avio_r8(s->pb);
            size--;
        } else if (type == FLV_TAG_TYPE_VIDEO) {
            is_audio = 0;
            flags = avio_r8(s->pb);
            size--;
            if ((flags & 0xf0) == 0x50) { /* video info / command frame */
                goto skip;
            }
        } else {
            if (type == FLV_TAG_TYPE_META && size > 13 + 1 + 4) {
                flv_read_metabody(s, next);
            } else { /* skip packet */
                av_log(s, AV_LOG_DEBUG, "skipping flv packet: type %d, size %d, flags %d\n", type, size, flags);
            }
skip:
            avio_seek(s->pb, next, SEEK_SET);
            continue;
        }

        /* skip empty data packets */
        if (!size) {
            continue;
        }

        /* now find stream */
        for (i = 0; i < s->nb_streams; i++) {
            st = s->streams[i];
            if (st->id == is_audio) {
                break;
            }
        }
        if (i == s->nb_streams) {
            av_log(s, AV_LOG_ERROR, "invalid stream\n");
            st = create_stream(s, is_audio);
            s->ctx_flags &= ~AVFMTCTX_NOHEADER;
        }
        av_dlog(s, "%d %X %d \n", is_audio, flags, st->discard);
        if ((st->discard >= AVDISCARD_NONKEY && !((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY ||         is_audio))
            || (st->discard >= AVDISCARD_BIDIR  && ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_DISP_INTER && !is_audio))
            || st->discard >= AVDISCARD_ALL
           ) {
            avio_seek(s->pb, next, SEEK_SET);
            continue;
        }
        if ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY) {
            av_add_index_entry(st, pos, dts, size, 0, AVINDEX_KEYFRAME);
        }
        break;
    }

    // if not streamed and no duration from metadata then seek to end to find the duration from the timestamps
    if (s->pb->seekable && (!s->duration || s->duration == AV_NOPTS_VALUE)) {
        int size;
        const int64_t pos = avio_tell(s->pb);
        const int64_t fsize = avio_size(s->pb);
        avio_seek(s->pb, fsize - 4, SEEK_SET);
        size = avio_rb32(s->pb);
        avio_seek(s->pb, fsize - 3 - size, SEEK_SET);
        if (size == avio_rb24(s->pb) + 11) {
            uint32_t ts = avio_rb24(s->pb);
            ts |= avio_r8(s->pb) << 24;
            s->duration = ts * (int64_t)AV_TIME_BASE / 1000;
        }
        avio_seek(s->pb, pos, SEEK_SET);
    }

    if (is_audio) {
        if (!st->codec->channels || !st->codec->sample_rate || !st->codec->bits_per_coded_sample) {
            st->codec->channels = (flags & FLV_AUDIO_CHANNEL_MASK) == FLV_STEREO ? 2 : 1;
            st->codec->sample_rate = (44100 << ((flags & FLV_AUDIO_SAMPLERATE_MASK) >> FLV_AUDIO_SAMPLERATE_OFFSET) >> 3);
            st->codec->bits_per_coded_sample = (flags & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;
        }
        if (!st->codec->codec_id) {
            flv_set_audio_codec(s, st, flags & FLV_AUDIO_CODECID_MASK);
        }
    } else {
        size -= flv_set_video_codec(s, st, flags & FLV_VIDEO_CODECID_MASK);
    }

    if (st->codec->codec_id == CODEC_ID_AAC ||
        st->codec->codec_id == CODEC_ID_H264 ||
        st->codec->codec_id == CODEC_ID_HEVC) {
        int type = avio_r8(s->pb);
        size--;
        if (st->codec->codec_id == CODEC_ID_H264 || st->codec->codec_id == CODEC_ID_HEVC) {
            int32_t cts = (avio_rb24(s->pb) + 0xff800000) ^ 0xff800000; // sign extension
            pts = dts + cts;
            if (cts < 0) { // dts are wrong
                flv->wrong_dts = 1;
                av_log(s, AV_LOG_WARNING, "negative cts, previous timestamps might be wrong.cts=%d, dts=%lld, pts=%lld\n", cts, dts, pts);
            }
            if (flv->wrong_dts) {
                pts = AV_NOPTS_VALUE;
            }
        }
        if (type == 0) {
            if ((ret = flv_get_extradata(s, st, size)) < 0) {
                return ret;
            }

            if (st->codec->codec_id == CODEC_ID_HEVC) {
                flv_extradata_process(st);
            }

            if (st->codec->codec_id == CODEC_ID_AAC) {
                MPEG4AudioConfig cfg;
                ff_mpeg4audio_get_config(&cfg, st->codec->extradata,
                                         st->codec->extradata_size);
                st->codec->channels = cfg.channels;
                if (cfg.ext_sample_rate) {
                    st->codec->sample_rate = cfg.ext_sample_rate;
                } else {
                    st->codec->sample_rate = cfg.sample_rate;
                }
                av_dlog(s, "mp4a config channels %d sample rate %d\n",
                        st->codec->channels, st->codec->sample_rate);
            }

            ret = AVERROR(EAGAIN);
            goto leave;
        }
    }

    /* skip empty data packets */
    if (!size) {
        ret = AVERROR(EAGAIN);
        goto leave;
    }

    if (st->codec->codec_id == CODEC_ID_HEVC && flv->first_hevc_packet != -1) {
        flv_get_hevc_packet(s, pkt, size);
        ret = size;
        flv->first_hevc_packet = -1;
    } else {
        ret = av_get_packet(s->pb, pkt, size);
    }
    if (ret < 0) {
        return AVERROR(EIO);
    }
    /* note: we need to modify the packet size here to handle the last
       packet */
    pkt->size = ret;
    pkt->dts = dts;
    pkt->pts = pts == AV_NOPTS_VALUE ? dts : pts;
    pkt->stream_index = st->index;

    if (is_audio || ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY)) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }

leave:
    avio_skip(s->pb, 4);
    return ret;
}

static int flv_read_seek(AVFormatContext *s, int stream_index,
                         int64_t ts, int flags)
{
    FLVContext *flv = s->priv_data;
    flv->validate_count = 0;
    return avio_seek_time(s->pb, stream_index, ts, flags);
}

#if 0 /* don't know enough to implement this */
static int flv_read_seek2(AVFormatContext *s, int stream_index,
                          int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
    int ret = AVERROR(ENOSYS);

    if (ts - min_ts > (uint64_t)(max_ts - ts)) {
        flags |= AVSEEK_FLAG_BACKWARD;
    }

    if (!s->pb->seekable) {
        if (stream_index < 0) {
            stream_index = av_find_default_stream_index(s);
            if (stream_index < 0) {
                return -1;
            }

            /* timestamp for default must be expressed in AV_TIME_BASE units */
            ts = av_rescale_rnd(ts, 1000, AV_TIME_BASE,
                                flags & AVSEEK_FLAG_BACKWARD ? AV_ROUND_DOWN : AV_ROUND_UP);
        }
        ret = avio_seek_time(s->pb, stream_index, ts, flags);
    }

    if (ret == AVERROR(ENOSYS)) {
        ret = av_seek_frame(s, stream_index, ts, flags);
    }
    return ret;
}
#endif

AVInputFormat ff_flv_demuxer = {
    "flv",
    NULL_IF_CONFIG_SMALL("FLV format"),
    sizeof(FLVContext),
    flv_probe,
    flv_read_header,
    flv_read_packet,
    .read_seek = flv_read_seek,
#if 0
    .read_seek2 = flv_read_seek2,
#endif
    .extensions = "flv",
    .value = CODEC_ID_FLV1,
};
