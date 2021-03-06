/*
 * various utility functions for use within FFmpeg
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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

/* #define DEBUG */

#define TRACE() av_log(NULL, AV_LOG_INFO, "[%s:%d]\n", __FUNCTION__, __LINE__)
#include "avformat.h"
#include "avio_internal.h"
#include "internal.h"
#include "libavcodec/internal.h"
#include "libavcodec/raw.h"
#include "libavcodec/hevc.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavutil/pixdesc.h"
#include "metadata.h"
#include "id3v2.h"
#include "id3v1.h"
#include "libavutil/avstring.h"
#include "riff.h"
#include "audiointerleave.h"
#include "url.h"
#include <sys/time.h>
#include <time.h>
#include <strings.h>
#include <stdarg.h>
#include "file_list.h"
#include "nsc.h"
#define FF_FDEBUG_TS 1
#include "tcp_pool.h"

#if CONFIG_NETWORK
#include "network.h"
#endif

#undef NDEBUG
#include <assert.h>


//added by aml
#include <cutils/properties.h>

#define DURATION_MAX_READ_SIZE 250000LL
#define DURATION_ESTIMATE_MAX_READ_SIZE 4096LL
#define DURATION_MAX_READ_LOCALPLAY_SIZE 1500000LL
#define DURATION_MAX_RETRY 4
#define DURATION_ESTIMATE_MAX_RETRY 250

#define CHECK_FULL_ZERO_SIZE DURATION_MAX_READ_SIZE
#define RETRY_CHECK_MAX 14 //check size 150000*1024*16
#define ERROR_URL_NOT_M3U8    123456

/**
 * @file
 * various utility functions for use within FFmpeg
 */

unsigned avformat_version(void)
{
    return LIBAVFORMAT_VERSION_INT;
}

const char *avformat_configuration(void)
{
    return FFMPEG_CONFIGURATION;
}

const char *avformat_license(void)
{
#define LICENSE_PREFIX "libavformat license: "
    return LICENSE_PREFIX FFMPEG_LICENSE + sizeof(LICENSE_PREFIX) - 1;
}

/* fraction handling */

/**
 * f = val + (num / den) + 0.5.
 *
 * 'num' is normalized so that it is such as 0 <= num < den.
 *
 * @param f fractional number
 * @param val integer value
 * @param num must be >= 0
 * @param den must be >= 1
 */
static void av_frac_init(AVFrac *f, int64_t val, int64_t num, int64_t den)
{
    num += (den >> 1);
    if (num >= den) {
        val += num / den;
        num = num % den;
    }
    f->val = val;
    f->num = num;
    f->den = den;
}

/**
 * Fractional addition to f: f = f + (incr / f->den).
 *
 * @param f fractional number
 * @param incr increment, can be positive or negative
 */
static void av_frac_add(AVFrac *f, int64_t incr)
{
    int64_t num, den;
    num = f->num + incr;
    den = f->den;
    if (num < 0) {
        f->val += num / den;
        num = num % den;
        if (num < 0) {
            num += den;
            f->val--;
        }
    } else if (num >= den) {
        f->val += num / den;
        num = num % den;
    }
    f->num = num;
}

/** head of registered input format linked list */
static AVInputFormat *first_iformat = NULL;
/** head of registered output format linked list */
static AVOutputFormat *first_oformat = NULL;

AVInputFormat  *av_iformat_next(AVInputFormat  *f)
{
    if (f) {
        return f->next;
    } else {
        return first_iformat;
    }
}

AVOutputFormat *av_oformat_next(AVOutputFormat *f)
{
    if (f) {
        return f->next;
    } else {
        return first_oformat;
    }
}

void av_register_input_format(AVInputFormat *format)
{
    AVInputFormat **p;
    p = &first_iformat;
    while (*p != NULL) {
        p = &(*p)->next;
    }
    *p = format;
    format->next = NULL;
}

void av_register_output_format(AVOutputFormat *format)
{
    AVOutputFormat **p;
    p = &first_oformat;
    while (*p != NULL) {
        p = &(*p)->next;
    }
    *p = format;
    format->next = NULL;
}

int av_match_ext(const char *filename, const char *extensions)
{
    const char *ext, *p;
    char ext1[32], *q;
    if (!filename) {
        return 0;
    }
    ext = strrchr(filename, '.');
    if (ext) {
        ext++;
        p = extensions;
        for (;;) {
            q = ext1;
            while (*p != '\0' && *p != ',' && q - ext1 < sizeof(ext1) - 1) {
                *q++ = *p++;
            }
            *q = '\0';
            if (!strcasecmp(ext1, ext)) {
                return 1;
            }
            if (*p == '\0') {
                break;
            }
            p++;
        }
    }
    return 0;
}

static int match_format(const char *name, const char *names)
{
    const char *p;
    int len, namelen;
    if (!name || !names) {
        return 0;
    }
    namelen = strlen(name);
    while ((p = strchr(names, ','))) {
        len = FFMAX(p - names, namelen);
        if (!strncasecmp(name, names, len)) {
            return 1;
        }
        names = p + 1;
    }
    return !strcasecmp(name, names);
}

AVOutputFormat *av_guess_format(const char *short_name, const char *filename,
                                const char *mime_type)
{
    AVOutputFormat *fmt = NULL, *fmt_found;
    int score_max, score;
    /* specific test for image sequences */
#if CONFIG_IMAGE2_MUXER
    if (!short_name && filename &&
        av_filename_number_test(filename) &&
        ff_guess_image2_codec(filename) != CODEC_ID_NONE) {
        return av_guess_format("image2", NULL, NULL);
    }
#endif
    /* Find the proper file type. */
    fmt_found = NULL;
    score_max = 0;
    while ((fmt = av_oformat_next(fmt))) {
        score = 0;
        if (fmt->name && short_name && !strcmp(fmt->name, short_name)) {
            score += 100;
        }
        if (fmt->mime_type && mime_type && !strcmp(fmt->mime_type, mime_type)) {
            score += 10;
        }
        if (filename && fmt->extensions &&
            av_match_ext(filename, fmt->extensions)) {
            score += 5;
        }
        if (score > score_max) {
            score_max = score;
            fmt_found = fmt;
        }
    }
    return fmt_found;
}

enum CodecID av_guess_codec(AVOutputFormat *fmt, const char *short_name,
                            const char *filename, const char *mime_type, enum AVMediaType type)
{
    if (type == AVMEDIA_TYPE_VIDEO) {
        enum CodecID codec_id = CODEC_ID_NONE;
#if CONFIG_IMAGE2_MUXER
        if (!strcmp(fmt->name, "image2") || !strcmp(fmt->name, "image2pipe")) {
            codec_id = ff_guess_image2_codec(filename);
        }
#endif
        if (codec_id == CODEC_ID_NONE) {
            codec_id = fmt->video_codec;
        }
        return codec_id;
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        return fmt->audio_codec;
    } else if (type == AVMEDIA_TYPE_SUBTITLE) {
        return fmt->subtitle_codec;
    } else {
        return CODEC_ID_NONE;
    }
}

AVInputFormat *av_find_input_format(const char *short_name)
{
    AVInputFormat *fmt = NULL;
    while ((fmt = av_iformat_next(fmt))) {
        if (match_format(short_name, fmt->name)) {
            return fmt;
        }
    }
    return NULL;
}


int av_get_packet(AVIOContext *s, AVPacket *pkt, int size)
{
    int ret = av_new_packet(pkt, size);
    if (ret < 0) {
        return ret;
    }
    pkt->pos = avio_tell(s);
    ret = avio_read(s, pkt->data, size);
    if (ret <= 0) {
        av_free_packet(pkt);
    } else {
        av_shrink_packet(pkt, ret);
    }
    return ret;
}

int av_append_packet(AVIOContext *s, AVPacket *pkt, int size)
{
    int ret;
    int old_size;
    if (!pkt->size) {
        return av_get_packet(s, pkt, size);
    }
    old_size = pkt->size;
    ret = av_grow_packet(pkt, size);
    if (ret < 0) {
        return ret;
    }
    ret = avio_read(s, pkt->data + old_size, size);
    av_shrink_packet(pkt, old_size + FFMAX(ret, 0));
    return ret;
}


int av_filename_number_test(const char *filename)
{
    char buf[1024];
    return filename && (av_get_frame_filename(buf, sizeof(buf), filename, 1) >= 0);
}

AVInputFormat *av_probe_input_format3(AVProbeData *pd, int is_opened, int *score_ret)
{
    AVProbeData lpd = *pd;
    AVInputFormat *fmt1 = NULL, *fmt = NULL, *cmf_fmt = NULL;
    AVIOContext * pb = pd->s;
    int score, score_max = 0, cmf_flag = 0;
    if (lpd.buf_size > 10 && ff_id3v2_match(lpd.buf, ID3v2_DEFAULT_MAGIC)) {
        int id3len = ff_id3v2_tag_len(lpd.buf);
        if (lpd.buf_size > id3len + 16) {
            lpd.buf += id3len;
            lpd.buf_size -= id3len;
        }
    }

    while ((fmt1 = av_iformat_next(fmt1))) {
        if (!is_opened == !(fmt1->flags & AVFMT_NOFILE)) {
            continue;
        }
        // iocontext that allotted in hls demuxer will crash extractor below, shield them.
        if (pb && pb->mhls_inner_format > 0 &&
            (!strncmp(fmt1->name, "DRMdemux", 8) || (!strncmp(fmt1->name, "Demux_no_prot", 13)))) {
            continue;
        }

        score = 0;
        if (fmt1->read_probe) {
            score = fmt1->read_probe(&lpd);
            if (!score && fmt1->extensions && av_match_ext(lpd.filename, fmt1->extensions)) {
                score = 1;
            }
        } else if (fmt1->extensions) {
            if (av_match_ext(lpd.filename, fmt1->extensions)) {
                score = 50;
            }
        }
        if (!strncmp(fmt1->name, "cmf", 3) && score == AVPROBE_SCORE_MAX) {
            cmf_fmt = fmt1;
        }
        if (cmf_fmt && score == AVPROBE_SCORE_MAX) {
            if (am_getconfig_bool("media.libplayer.tsincmf") && !strncmp(fmt1->name, "mpegts", 6)) {
                cmf_flag = 1;
            } else if (!strncmp(fmt1->name, "flv", 3) || !strncmp(fmt1->name, "mov", 3)) {
                cmf_flag = 1;
            }
        }
        if (score > score_max) {
            if (strncmp(fmt1->name, "cmf", 3)) { // skip cmf
                score_max = score;
                fmt = fmt1;
            }
        } else if (score == score_max) {
            fmt = NULL;
        }
    }
    *score_ret = score_max;
    if (lpd.pads[0] != 0) {
        memcpy(pd->pads, lpd.pads, sizeof(lpd.pads));
    }
    if (cmf_flag) {
        fmt = cmf_fmt;
    }
    return fmt;
}

AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max)
{
    int score_ret;
    AVInputFormat *fmt = av_probe_input_format3(pd, is_opened, &score_ret);
    if (score_ret > *score_max) {
        *score_max = score_ret;
        return fmt;
    } else {
        return NULL;
    }
}

AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)
{
    int score = 0;
    return av_probe_input_format2(pd, is_opened, &score);
}

static int set_codec_from_probe_data(AVFormatContext *s, AVStream *st, AVProbeData *pd)
{
    static const struct {
        const char *name;
        enum CodecID id;
        enum AVMediaType type;
    } fmt_id_type[] = {
        { "aac"      , CODEC_ID_AAC       , AVMEDIA_TYPE_AUDIO },
        { "ac3"      , CODEC_ID_AC3       , AVMEDIA_TYPE_AUDIO },
        { "dts"      , CODEC_ID_DTS       , AVMEDIA_TYPE_AUDIO },
        { "eac3"     , CODEC_ID_EAC3      , AVMEDIA_TYPE_AUDIO },
        { "h264"     , CODEC_ID_H264      , AVMEDIA_TYPE_VIDEO },
        { "hevc"     , CODEC_ID_HEVC      , AVMEDIA_TYPE_VIDEO },
        { "m4v"      , CODEC_ID_MPEG4     , AVMEDIA_TYPE_VIDEO },
        { "mp3"      , CODEC_ID_MP3       , AVMEDIA_TYPE_AUDIO },
        { "mp2"      , CODEC_ID_MP2       , AVMEDIA_TYPE_AUDIO },
        { "mpegvideo", CODEC_ID_MPEG2VIDEO, AVMEDIA_TYPE_VIDEO },
        { 0 }
    };
    int score;
    AVInputFormat *fmt = av_probe_input_format3(pd, 1, &score);
    if ((fmt && score > 1) || ((!strcmp(s->iformat->name, "rtp") && score == 1))) { //the condition : score > 1 ,rererence from ffmpeg 2.6,to aviod wrong probe;
        int i;
        av_log(s, AV_LOG_INFO, "Probe with size=%d, packets=%d detected %s with score=%d\n",
               pd->buf_size, MAX_PROBE_PACKETS - st->probe_packets, fmt->name, score);
        for (i = 0; fmt_id_type[i].name; i++) {
            if (!strcmp(fmt->name, fmt_id_type[i].name)) {
                st->codec->codec_id   = fmt_id_type[i].id;
                st->codec->codec_type = fmt_id_type[i].type;
                break;
            }
        }
    }
    if (st->codec->mpegps_video_idprobed
        && (score < AVPROBE_SCORE_MAX / 10)) {
        st->codec->codec_id   = CODEC_ID_MPEG2VIDEO;
        st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
        av_log(NULL, AV_LOG_ERROR, "[%s:%d]probe score %d\n", __FUNCTION__, __LINE__, score);
    }
    return score;
}

/************************************************************/
/* input media file */

#if FF_API_FORMAT_PARAMETERS
static AVDictionary *convert_format_parameters(AVFormatParameters *ap)
{
    char buf[1024];
    AVDictionary *opts = NULL;
    if (!ap) {
        return NULL;
    }
    if (ap->time_base.num) {
        snprintf(buf, sizeof(buf), "%d/%d", ap->time_base.den, ap->time_base.num);
        av_dict_set(&opts, "framerate", buf, 0);
    }
    if (ap->sample_rate) {
        snprintf(buf, sizeof(buf), "%d", ap->sample_rate);
        av_dict_set(&opts, "sample_rate", buf, 0);
    }
    if (ap->channels) {
        snprintf(buf, sizeof(buf), "%d", ap->channels);
        av_dict_set(&opts, "channels", buf, 0);
    }
    if (ap->width || ap->height) {
        snprintf(buf, sizeof(buf), "%dx%d", ap->width, ap->height);
        av_dict_set(&opts, "video_size", buf, 0);
    }
    if (ap->pix_fmt != PIX_FMT_NONE) {
        av_dict_set(&opts, "pixel_format", av_get_pix_fmt_name(ap->pix_fmt), 0);
    }
    if (ap->channel) {
        snprintf(buf, sizeof(buf), "%d", ap->channel);
        av_dict_set(&opts, "channel", buf, 0);
    }
    if (ap->standard) {
        av_dict_set(&opts, "standard", ap->standard, 0);
    }
    if (ap->mpeg2ts_compute_pcr) {
        av_dict_set(&opts, "mpeg2ts_compute_pcr", "1", 0);
    }
    if (ap->initial_pause) {
        av_dict_set(&opts, "initial_pause", "1", 0);
    }
    return opts;
}

/**
 * Open a media file from an IO stream. 'fmt' must be specified.
 */
int av_open_input_stream(AVFormatContext **ic_ptr,
                         AVIOContext *pb, const char *filename,
                         AVInputFormat *fmt, AVFormatParameters *ap)
{
    int err;
    AVDictionary *opts;
    AVFormatContext *ic;
    AVFormatParameters default_ap;
    if (!ap) {
        ap = &default_ap;
        memset(ap, 0, sizeof(default_ap));
    }
    opts = convert_format_parameters(ap);
    if (!ap->prealloced_context) {
        ic = avformat_alloc_context();
    } else {
        ic = *ic_ptr;
    }
    if (!ic) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    ic->pb = pb;
    err = avformat_open_input(ic_ptr, filename, fmt, &opts);
fail:
    av_dict_free(&opts);
    return err;
}
#endif

int av_demuxer_open(AVFormatContext *ic, AVFormatParameters *ap)
{
    int err;
    if (ic->iformat->read_header) {
        err = ic->iformat->read_header(ic, ap);
        if (err < 0) {
            return err;
        }
    }
    if (ic->pb && !ic->data_offset) {
        ic->data_offset = avio_tell(ic->pb);
    }
    return 0;
}


/** size of probe buffer, for guessing file type from file contents */
#define PROBE_BUF_MIN 2048
#define PROBE_BUF_MAX (1<<20)*2

int av_probe_input_buffer(AVIOContext *pb, AVInputFormat **fmt,
                          const char *filename, void *logctx,
                          unsigned int offset, unsigned int max_probe_size)
{
    AVProbeData pd = { filename ? filename : "", NULL, -offset, pb, 0, 0, 0 };
    unsigned char *buf = NULL;
    int ret = 0, probe_size;
    int data_offset = 0;
    int pre_data = 0;
    int probe_flag = 0;
    int64_t oldoffset;
    int64_t old_dataoff;
    AVFormatContext *s = logctx;
    int maxretry = 0;
    int eof_flag = 0;
    int64_t filesize = avio_size(pb);
    int probe_time_s = 0;
    int ret2 = 0;
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    if (property_get("media.amplayer.probe_time_s", prop_value, "30") > 0) {
        probe_time_s = atoi(prop_value);
    }

    av_log(NULL, AV_LOG_INFO, "%s:size=%lld\n", pd.filename, filesize);
    if (!max_probe_size) {
        max_probe_size = PROBE_BUF_MAX;
    } else if (max_probe_size > PROBE_BUF_MAX) {
        max_probe_size = PROBE_BUF_MAX;
    } else if (max_probe_size < PROBE_BUF_MIN) {
        return AVERROR(EINVAL);
    }
    if (offset >= max_probe_size) {
        return AVERROR(EINVAL);
    }
    oldoffset = avio_tell(pb);
    if (s) {
        old_dataoff = s->data_offset;
    } else {
        old_dataoff = 0;
    }
    if (av_match_ext(filename, "ts") || av_match_ext(filename, "m2ts")) {
        probe_flag = 1;
        do {
            pre_data = avio_r8(pb);
            data_offset ++;
            if (pre_data == 0x47) {
                avio_seek(pb, -1, SEEK_CUR);
                data_offset --;
                av_log(NULL, AV_LOG_INFO, "*****[%s] [%llx] data_offset=%d\n", __FUNCTION__, avio_tell(pb), data_offset);
                if (s) {
                    s->data_offset = data_offset;
                }
                break;
            }
            /*find the ts sync header if no erroris,not eof and interrupt*/
        } while (!(pb->error || pb->eof_reached || url_interrupt_cb()));
    }
    if (strstr(filename, "rtp://") != NULL) {
        extern AVInputFormat ff_mpegts_demuxer;
        *fmt = &ff_mpegts_demuxer;
        av_log(NULL, AV_LOG_INFO, "[%s]rtp set mpegts,skip probe format\n", __FUNCTION__);
        goto exit;
    }
    int64_t probe_startUs;
retry_probe:
    probe_startUs = av_gettime();
    for (probe_size = PROBE_BUF_MIN; probe_size <= max_probe_size && !*fmt && ret >= 0;
         probe_size = FFMIN(FFMAX(pd.buf_size << 1, PROBE_BUF_MIN), FFMAX(max_probe_size, probe_size + 1))) {
        int ret, score = probe_size < max_probe_size ? AVPROBE_SCORE_MAX / 4 : 0;
        // int buf_offset = (probe_size == PROBE_BUF_MIN) ? 0 : probe_size>>1;
        int buf_offset =  pd.buf_size ;
        if (probe_size < offset) {
            continue;
        }
        /* read probe data */
        buf = av_realloc(buf, probe_size + AVPROBE_PADDING_SIZE);
        if ((ret = avio_read(pb, buf + buf_offset, probe_size - buf_offset)) < 0) {
            /* fail if error was not end of file, otherwise, lower score */
            if (ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
                av_free(buf);
                return ret;
            } else if (ret == AVERROR_EOF) {
                eof_flag = 1;
            }
            if (pb->mhls_inner_format > 0) {
                if (((av_gettime() - probe_startUs) / 1000000) > probe_time_s) {
                    return -1;
                }
            } else {
                maxretry++;
                if (maxretry > 1000) {
                    return -1;
                }
            }
            score = 0;
            ret = 0;            /* error was end of file, nothing read */
        } else {
            maxretry = 0;
        }
        if (ret > 0 || eof_flag == 1) {
            pd.buf_size += ret;
            pd.buf = &buf[offset];
            memset(pd.buf + pd.buf_size, 0, AVPROBE_PADDING_SIZE);
            av_log(NULL, AV_LOG_INFO, "[%s]read %d bytes for probe format\n", __FUNCTION__, pd.buf_size);
            /* guess file format */
            *fmt = av_probe_input_format2(&pd, 1, &score);
            if (*fmt) {
                if (score <= AVPROBE_SCORE_MAX / 4) { //this can only be true in the last iteration
                    av_log(logctx, AV_LOG_WARNING, "Format %s detected only with low score of %d, misdetection possible!\n", (*fmt)->name, score);
                } else {
                    if (!strcmp((*fmt)->name, "psxstr") && (score == 50)) {
                        /* it is psxstr but something like mpeg ps for score 50 */
                        AVInputFormat *fmt1 = NULL;
                        while ((fmt1 = av_iformat_next(fmt1))) {
                            if (!strcmp(fmt1->name, "mpeg")) {
                                *fmt = fmt1;
                                break;
                            }
                        }
                    }
                    av_log(logctx, AV_LOG_INFO, "Format %s probed with size=%d and score=%d\n", (*fmt)->name, probe_size, score);
                }
            }
        } else {
            av_log(NULL, AV_LOG_DEBUG, "[%s]no new data for probe,retry\n", __FUNCTION__);
        }
        if (eof_flag == 1) {
            av_log(NULL, AV_LOG_WARNING, "[%s]read end, exit probe format\n", __FUNCTION__);
            break;
        }
    }
    if (!*fmt) {
        av_free(buf);
        if (probe_flag) {
            if (s) {
                s->data_offset = old_dataoff;
            }
            probe_flag = 0;
            buf = NULL;
            pd.buf = NULL;
            pd.buf_size = 0;
            avio_seek(pb, oldoffset, SEEK_SET);
            av_log(logctx, AV_LOG_INFO, "not real ts/m2ts,probe again\n");
            goto retry_probe;
        }
        return AVERROR_INVALIDDATA;
    } else if (strcmp((*fmt)->name, "mpegts") && probe_flag) {
        s->data_offset = old_dataoff;
        probe_flag = 0;
        data_offset = 0;
        *fmt = NULL;
        av_free(buf);
        buf = NULL;
        pd.buf = NULL;
        pd.buf_size = 0;
        avio_seek(pb, oldoffset, SEEK_SET);
        av_log(logctx, AV_LOG_INFO, "Format not ts, probe again\n");
        goto retry_probe;
    }
exit:
    /* rewind. reuse probe buffer to avoid seeking */
    if ((ret2 = ffio_rewind_with_probe_data(pb, buf, pd.buf_size + data_offset)) < 0) {
        av_log(logctx, AV_LOG_INFO, "warnning ffio_rewind_with_probe_data error,:%d\n", ret2);
        av_free(buf);
    }
    memcpy(pb->proppads, pd.pads, sizeof(pd.pads));
    return ret;
}

#if FF_API_FORMAT_PARAMETERS
int av_open_input_file(AVFormatContext **ic_ptr, const char *filename,
                       AVInputFormat *fmt,
                       int buf_size,
                       AVFormatParameters *ap)
{
    int err;
    AVDictionary *opts = convert_format_parameters(ap);
    if (!ap || !ap->prealloced_context) {
        *ic_ptr = NULL;
    }
    err = avformat_open_input(ic_ptr, filename, fmt, &opts);
    av_dict_free(&opts);
    return err;
}

int av_open_input_file_header(AVFormatContext **ic_ptr, const char *filename,
                              AVInputFormat *fmt,
                              int buf_size,
                              AVFormatParameters *ap,
                              const char *headers
                             )
{
    int err;
    AVDictionary *opts = convert_format_parameters(ap);
    if (!ap || !ap->prealloced_context) {
        *ic_ptr = NULL;
    }
    err = avformat_open_input_header(ic_ptr, filename, fmt, &opts, headers);
    av_dict_free(&opts);
    return err;
}

player_notify_t player_notify = NULL;
int ffmpeg_register_notify(const player_notify_t notify_fn)
{
    if (notify_fn) {
        player_notify = notify_fn;
    }
    return 0;
}

int ffmpeg_notify(URLContext *h, int msg, unsigned long ext1, unsigned long ext2)
{
    if ((player_notify) && (h)) {
        player_notify(h->notify_id, msg, ext1, ext2);
    }
    return 0;
}

#endif

#define EXT_X_PLAYREADYHEADER       "#EXT-X-PLAYREADYHEADER"  //PlayReady DRM  tag
static int cryptopr_probe(ByteIOContext *s, const char *file)
{
    char line[1024 + 1];
    int ret;
    int linecnt = 0;
    if (!s) {
        return 0;
    }
    do {
        if (url_interrupt_cb()) {
            av_log(NULL, AV_LOG_ERROR, "[cryptopr_probe] return.\n");
            return 0;
        }
        ret = ff_get_assic_line(s, line, 1024);
        if (!strncmp(line, EXT_X_PLAYREADYHEADER, strlen(EXT_X_PLAYREADYHEADER))) {
            av_log(NULL, AV_LOG_INFO, "[cryptopr_probe] HLS PlayReady Tag Found.\n");
            return 100;
        }
    } while (ret > 0 && linecnt++ < 50);
    return 0;
}

typedef struct auto_switch_protol {
    char *prefix;
    int (*probe_check)(ByteIOContext *, char *);
} auto_switch_protol_t;

#include "hlsproto.h"
#include "mmsh.h"
auto_switch_protol_t switch_table[] = {
    {"cryptopr:", cryptopr_probe},
    {"list:", url_is_file_list},
    /*  {"hls+",hlsproto_probe},*/
    {"nsc:", is_nsc_file},
    {"mmsh:", is_mmsh_file},
    {NULL, NULL}
};

static auto_switch_protol_t *try_get_mached_new_prot(ByteIOContext *pb, const char *name)
{
    auto_switch_protol_t *p = switch_table;
    int i;
    int64_t off = url_ftell(pb);
    for (i = 0; p != NULL && p->prefix && p->probe_check; i++) {
        if (url_interrupt_cb()) {
            av_log(NULL, AV_LOG_ERROR, "[try_get_mached_new_prot] return.\n");
            return NULL;
        }

        av_log(NULL, AV_LOG_INFO, "auto_switch_protol_t:%s:\n", p->prefix);
        if (p->probe_check(pb, name) >= 100) {
            return p;
        }
        url_fseek(pb, off, SEEK_SET);
        p++;
    }
    //return &switch_table[1];
    return NULL;
}

// add this func for mms:// protocol transfer by senbai.tao
static char *transfer_mms_protocol(AVFormatContext *s, const char *filename, const char *headers)
{
    int ret;
    char *file = (char *)av_malloc(strlen(filename) + 1);
    snprintf(file, strlen(filename) + 5, "mmsh%s", filename + 3);
    ret = avio_open_h(&s->pb, file, AVIO_FLAG_READ, headers);
    if (ret >= 0) {
        return file;
    }
    snprintf(file, strlen(filename) + 5, "mmst%s", filename + 3);
    ret = avio_open_h(&s->pb, file, AVIO_FLAG_READ, headers);
    if (ret >= 0) {
        return file;
    } else {
        av_free(file);
        file = NULL;
        return NULL;
    }
}


//add this api for open third-parts libmms supports,by peter,20121121
#include "ammodule.h"
static int is_use_external_module(const char *mod_name)
{
    int ret = -1;
    const char *ex_mod = "media.libplayer.modules";
    char value[CONFIG_VALUE_MAX];
    ret = am_getconfig(ex_mod, value, NULL);
    if (ret < 1) {
        return 0;
    }
    ret = ammodule_match_check(value, mod_name);
    if (ret > 0) {
        return 1;
    } else {
        return 0;
    }
}
#define PLAYER_EVENTS_ERROR 3
/* open input file and probe the format if necessary */
static int init_input(AVFormatContext *s, const char *orig_filename, const char *headers, const AVDictionary **options)

{
    int ret;
    char * filename = orig_filename;
#if 0
    if (!strncmp(orig_filename, "vhls:", strlen("vhls:"))
        || !strncmp(orig_filename, "list:", strlen("list:"))) { // remove vhls or list tag.
        filename = orig_filename + 5;
    }
#endif
    AVProbeData pd = {filename, NULL, 0, NULL};
    auto_switch_protol_t *newp = NULL;

    if (s->pb) {
        s->pb->is_segment_media = 0;
        s->flags |= AVFMT_FLAG_CUSTOM_IO;
        if (!s->iformat) {
            return av_probe_input_buffer(s->pb, &s->iformat, filename, s, 0, 0);
        } else if (s->iformat->flags & AVFMT_NOFILE) {
            return AVERROR(EINVAL);
        }
        return 0;
    }
    if ((s->iformat && s->iformat->flags & AVFMT_NOFILE) ||
        (!s->iformat && (s->iformat = av_probe_input_format(&pd, 0)))) {
        s->pd.filename = pd.filename;
        s->pd.buf = pd.buf;
        s->pd.s = pd.s;
        memcpy(s->pd.pads, pd.pads, 8 * sizeof(long));
        av_log(NULL, AV_LOG_WARNING, "[init-input] iformat name%s return\n", s->iformat->name);
        return 0;
    }
    if (!strncmp(filename, "mms:", strlen("mms:"))) {
        char *mms_prot = transfer_mms_protocol(s, filename, headers);
        if (mms_prot) {
            s->pb->filename = mms_prot;
            av_log(NULL, AV_LOG_INFO, "[%s:%d]Tranfer mms mms_prot=%s\n", __FUNCTION__, __LINE__, mms_prot);
        }
    } else {
        if (strstr(filename, "shttp") != NULL && strstr(filename, "vhls") != NULL) { // hls protocol
            int iptv_v5_support = (int)am_getconfig_bool_def("media.amplayer.hlsv5_support", 0);
            if (iptv_v5_support == 0) {
                // use v3 code
                int flags = AVIO_FLAG_READ;
                av_log(NULL, AV_LOG_INFO, "[%s:%d] HLS use v3 in\n", __FUNCTION__, __LINE__);
                if (options && *options) {
                    ret = avio_open_h2(&s->pb, filename, flags, headers, (unsigned long)options);
                } else {
                    ret = avio_open_h(&s->pb, filename, flags, headers);
                }
                if (ret) {
                    /*when url is not m3u8 should try http*/
                    if (ret == -ERROR_URL_NOT_M3U8) {
                        char *listfile = av_mallocz(MAX_URL_SIZE);
                        strcpy(listfile, filename + 6);
                        av_log(NULL, AV_LOG_INFO, "[%s:%d] open hls fail try other,url:%s\n", __FUNCTION__, __LINE__, listfile);
                        if (options && *options) {
                            ret = avio_open_h2(&s->pb, listfile, flags, headers, (unsigned long)options);
                        } else {
                            ret = avio_open_h(&s->pb, listfile, flags, headers);
                        }
                        if (ret < 0) {
                            return ret;
                        }

                        if (s->pb->filename) {
                            av_free(s->pb->filename);
                        }
                        s->pb->filename = listfile;
                        goto PASS_THROUGH;

                    } else {
                        av_log(NULL, AV_LOG_INFO, "[%s:%d] open hls fail\n", __FUNCTION__, __LINE__);
                        return ret;
                    }
                } else {
                    av_log(NULL, AV_LOG_INFO, "[%s:%d] HLS use v3 mode\n", __FUNCTION__, __LINE__);
                    goto PASS_THROUGH;
                }
            }

            char *listfile = av_mallocz(MAX_URL_SIZE);
            strcpy(listfile, "mhls:");
            strcpy(listfile + 5, filename + 5);

            av_log(NULL, AV_LOG_ERROR, "--[%s:%d] url to %s\n", __FUNCTION__, __LINE__, listfile);

            AVProbeData pd_tmp = {listfile, NULL, 0, NULL};
            s->iformat = av_probe_input_format(&pd_tmp, 0);
            if (s->iformat) {
                s->pd.filename = pd.filename;
                s->pd.buf = pd.buf;
                s->pd.s = pd.s;
                memcpy(s->pd.pads, pd.pads, 8 * sizeof(long));
                av_log(NULL, AV_LOG_INFO, "[%s:%d] mhls probe success !", __FUNCTION__, __LINE__);
                return 0;
            } else {
                av_log(NULL, AV_LOG_ERROR, "[%s:%d] mhls probe failed !", __FUNCTION__, __LINE__);
                return AVERROR(EIO);
            }
        } else if (strstr(filename, "shttp") != NULL && strstr(filename, ".mpd") != NULL) { // dash protocol
            char *listfile = av_mallocz(MAX_URL_SIZE);
            strcpy(listfile, "dash");
            strcpy(listfile + 4, filename + 5);
            av_log(NULL, AV_LOG_ERROR, "--[%s:%d] url to %s\n", __FUNCTION__, __LINE__, listfile);
            if ((ret = avio_open_h(&s->pb, listfile, AVIO_FLAG_READ, headers)) < 0) {
                return ret;
            }

            if (s->pb->filename) {
                av_free(s->pb->filename);
            }

            s->pb->filename = listfile;
            goto PASS_THROUGH;
        } else if (strstr(filename, "rtp://") != NULL && strstr(filename, "ChannelFECPort") != NULL && am_getconfig_bool("media.libplayer.usefec")) { // rtp fec protocol
            char *listfile = av_mallocz(MAX_URL_SIZE);
            strcpy(listfile, "rtpfec");
            strcpy(listfile + 6, filename + 3);
            av_log(NULL, AV_LOG_ERROR, "[%s:%d] ---url to %s\n", __FUNCTION__, __LINE__, listfile);
            if ((ret = avio_open_h(&s->pb, listfile, AVIO_FLAG_READ, headers)) < 0) {
                return ret;
            }

            s->pb->filename = listfile;
            goto PASS_THROUGH;
        } else {
            int flags = AVIO_FLAG_READ;
            if (av_strstart(filename, "rtp:", NULL)) {
                flags |= AVIO_FLAG_CACHE;
            }
            av_log(NULL, AV_LOG_INFO, "--1--%s---\n", __FUNCTION__);
            if (options && *options) {
                ret = avio_open_h2(&s->pb, filename, flags, headers, (unsigned long)options);
            } else {
                ret = avio_open_h(&s->pb, filename, flags, headers);
            }
            if (ret < 0) {
                return ret;
            }
            if (!strncmp(filename, "rtp:", strlen("rtp:")) || !strncmp(filename, "rtsp:", strlen("rtsp:"))) { //no need to try in new protocol matching
                goto PASS_THROUGH;
            }
        }
    }
    s->pb->is_segment_media = 0;
    newp = try_get_mached_new_prot(s->pb, filename);
    if (newp != NULL) {
        char *listfile;
        int err;
        char *ptr = NULL;
        listfile = av_mallocz(MAX_URL_SIZE);
        if (!listfile) {
            return AVERROR(ENOMEM);
        }
        if (av_strstart(newp->prefix, "mmsh:", &ptr) && (is_use_external_module("mms_mod") > 0)) {
            strcpy(listfile, "mmsx:");
            strcpy(listfile + strlen("mmsx:"), filename);
        } else {
            if (s->pb->is_slowmedia && av_strstart(newp->prefix, "list:", &ptr) && (!strstr(filename, "AmlogicPlayerDataSouceProtocol")) && (is_use_external_module("vhls_mod") > 0)) {
                int iptv_v5_support = (int)am_getconfig_bool_def("media.amplayer.hlsv5_support", 0);
                if (iptv_v5_support) {
                    strcpy(listfile, "mhls:");
                    s->pb->is_mhls = 1;
                } else {
                    strcpy(listfile, "vhls:");
                }

            } else {
                strcpy(listfile, newp->prefix);
            }
            if (s->pb->reallocation != NULL) {
                strcpy(listfile + strlen(newp->prefix), s->pb->reallocation);
            } else {
                strcpy(listfile + strlen(newp->prefix), filename);
            }
        }
        if (s->pb->is_mhls == 1) {
            if (s->pb->filename) {
                av_free(s->pb->filename);
            }
            s->pb->filename = listfile;
            AVProbeData pd_tmp = {listfile, NULL, 0, NULL};
            s->iformat = av_probe_input_format(&pd_tmp, 0);
            if (s->iformat) {
                s->pd.filename = pd.filename;
                s->pd.buf = pd.buf;
                s->pd.s = pd.s;
                memcpy(s->pd.pads, pd.pads, 8 * sizeof(long));
                av_log(NULL, AV_LOG_INFO, "[%s:%d] mhls probe success !", __FUNCTION__, __LINE__);
                return 0;
            } else {
                av_log(NULL, AV_LOG_ERROR, "[%s:%d] mhls probe failed !", __FUNCTION__, __LINE__);
                return AVERROR(EIO);
            }
        }

        url_fclose(s->pb);
        s->pb = NULL;
        av_log(NULL, AV_LOG_INFO, "[%s:%d]----Use new url=%s to open\n", __FUNCTION__, __LINE__, listfile);
        if (options && *options) {
            err = avio_open_h2(&s->pb, listfile, AVIO_FLAG_READ, headers, (unsigned long)options);
        } else {
            err = avio_open_h(&s->pb, listfile, AVIO_FLAG_READ, headers);
        }
        if (err < 0) {
            av_log(NULL, AV_LOG_ERROR, "init_input:%s failed,line=%d err=0x%x\n", listfile, __LINE__, err);
            av_free(listfile);
            return AVERROR(EIO);
        }
        s->pb->filename = listfile;
    }
    if (s->iformat) {
        return 0;
    }
PASS_THROUGH:
    return av_probe_input_buffer(s->pb, &s->iformat, orig_filename, s, 0, 0);
}


int avformat_open_input_header(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options, const char *headers)
{
    AVFormatContext *s = *ps;
    int ret = 0;
    AVFormatParameters ap = { 0 };
    AVDictionary *tmp = NULL;
retry_open:
    if (!s && !(s = avformat_alloc_context())) {
        return AVERROR(ENOMEM);
    }
    if (fmt) {
        s->iformat = fmt;
    }
    if (options) {
        av_dict_copy(&tmp, *options, 0);
    }
    if ((ret = av_opt_set_dict(s, &tmp)) < 0) {
        goto fail;
    }

    if ((ret = init_input(s, filename, headers, &tmp)) < 0 || url_interrupt_cb()) {
        if (ret >= 0) {
            ret = -1;
        }
        av_log(NULL, AV_LOG_INFO, "-%s---,ret=%d\n", __FUNCTION__, ret);
        goto fail;
    }
    /* check filename in case an image number is expected */
    if (s->iformat->flags & AVFMT_NEEDNUMBER) {
        if (!av_filename_number_test(filename)) {
            ret = AVERROR(EINVAL);
            goto fail;
        }
    }
    s->duration = s->start_time = AV_NOPTS_VALUE;
    av_strlcpy(s->filename, filename, sizeof(s->filename));
    if (headers) {
        s->headers = strdup(headers);
    }
    /* allocate private data */
    if (s->iformat->priv_data_size > 0) {
        if (!(s->priv_data = av_mallocz(s->iformat->priv_data_size))) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        if (s->iformat->priv_class) {
            *(const AVClass **)s->priv_data = s->iformat->priv_class;
            av_opt_set_defaults(s->priv_data);
            if ((ret = av_opt_set_dict(s->priv_data, &tmp)) < 0) {
                goto fail;
            }
        }
    }
    /* e.g. AVFMT_NOFILE formats will not have a AVIOContext */
    if (s->pb) {
        ff_id3v2_read(s, ID3v2_DEFAULT_MAGIC);
    }
    if (!strcmp(s->iformat->name, "flv")) {
        if (!s->pb->is_streamed) {
            s->seekable = 1;
            s->support_seek = 1;
        }
    }
    if (!(s->flags & AVFMT_FLAG_PRIV_OPT) && s->iformat->read_header)
        if ((ret = s->iformat->read_header(s, &ap)) < 0) {
            if (ret == AVERROR_MODULE_UNSUPPORT) {
                avformat_free_context(s);
                s = NULL;
                extern AVInputFormat ff_mpegts_demuxer;
                fmt = &ff_mpegts_demuxer;
                av_log(NULL, AV_LOG_ERROR, "[%s:%d]read_header unsupport error change url=%s\n", __FUNCTION__, __LINE__, filename);
                goto retry_open;
            }
            goto fail;
        }
    if (!(s->flags & AVFMT_FLAG_PRIV_OPT) && s->pb && !s->data_offset) {
        s->data_offset = avio_tell(s->pb);
    }
    s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
    if (options) {
        av_dict_free(options);
        *options = tmp;
    }
    *ps = s;
    return 0;
fail:
    av_dict_free(&tmp);
    if (s->pb && !(s->flags & AVFMT_FLAG_CUSTOM_IO)) {
        avio_close(s->pb);
    }
    avformat_free_context(s);
    *ps = NULL;
    return ret;
}

int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options)
{
    return avformat_open_input_header(ps, filename, fmt, options, NULL);
}

/*******************************************************/

static AVPacket *add_to_pktbuf(AVPacketList **packet_buffer, AVPacket *pkt,
                               AVPacketList **plast_pktl)
{
    AVPacketList *pktl = av_mallocz(sizeof(AVPacketList));
    if (!pktl) {
        return NULL;
    }
    if (*packet_buffer) {
        (*plast_pktl)->next = pktl;
    } else {
        *packet_buffer = pktl;
    }
    /* add the packet in the buffered packet list */
    *plast_pktl = pktl;
    pktl->pkt = *pkt;
    return &pktl->pkt;
}

int av_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret, i;
    AVStream *st;
    AVProbeData *pd = NULL;
    for (;;) {
        AVPacketList *pktl = s->raw_packet_buffer;
        if (pktl) {
            *pkt = pktl->pkt;
            if (s->raw_packet_buffer_remaining_size <= 0 && s->streams[pkt->stream_index]->request_probe > 0) {
                s->streams[pkt->stream_index]->probe_packets = 0;
                s->streams[pkt->stream_index]->request_probe = -1;
                if (s->streams[pkt->stream_index]->codec->codec_id != CODEC_ID_NONE) {
                    av_log(s, AV_LOG_ERROR, "probed stream %d codec_id:%d\n", s->streams[pkt->stream_index]->index,
                           s->streams[pkt->stream_index]->codec->codec_id);
                } else {
                    av_log(s, AV_LOG_ERROR, "probed stream %d failed\n", s->streams[pkt->stream_index]->index);

                    if (!strcmp(s->iformat->name, "rtp")) {
                        s->streams[pkt->stream_index]->codec->codec_id = CODEC_ID_H264;
                        s->streams[pkt->stream_index]->codec->codec_type = AVMEDIA_TYPE_VIDEO;
                        av_log(s, AV_LOG_ERROR, "probed stream failed 1. force h264\n");
                    }
                }
            }

            if (!strcmp(s->iformat->name, "rtp") && s->streams[pkt->stream_index]->codec->codec_id != CODEC_ID_NONE) {
                s->streams[pkt->stream_index]->probe_packets = 0;
                s->streams[pkt->stream_index]->request_probe = -1;
                av_log(s, AV_LOG_ERROR, "[%s %d] force probe end stream_index:%d codec_id:%d\n",
                       __FUNCTION__, __LINE__, pkt->stream_index, s->streams[pkt->stream_index]->codec->codec_id);
            }
            if (s->streams[pkt->stream_index]->request_probe <= 0) {
                s->raw_packet_buffer = pktl->next;
                s->raw_packet_buffer_remaining_size += pkt->size;
                av_free(pktl);
                if (pd) {
                    if (pd->buf) {
                        av_log(s, AV_LOG_DEBUG, "free probing stream1 %d, pd->buf = 0x%x\n", st->index, pd->buf);
                        av_freep(&pd->buf);
                    }
                }
                return 0;
            }
        }
        av_init_packet(pkt);
        ret = s->iformat->read_packet(s, pkt);
        if (ret < 0) {
            if (!pktl || ret == AVERROR(EAGAIN)) {
                return ret;
            }
            for (i = 0; i < s->nb_streams; i++)
                if (s->streams[i]->request_probe > 0) {
                    s->streams[i]->request_probe = -1;
                }
            continue;
        }
        if (!(s->flags & AVFMT_FLAG_KEEP_SIDE_DATA)) {
            av_packet_merge_side_data(pkt);
        }
        st = s->streams[pkt->stream_index];
        switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            if (s->video_codec_id) {
                st->codec->codec_id = s->video_codec_id;
            }
            break;
        case AVMEDIA_TYPE_AUDIO:
            if (s->audio_codec_id) {
                st->codec->codec_id = s->audio_codec_id;
            }
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            if (s->subtitle_codec_id) {
                st->codec->codec_id = s->subtitle_codec_id;
            }
            break;
        }
        if (!pktl && st->request_probe <= 0) {
            if (pd) {
                if (pd->buf) {
                    av_log(s, AV_LOG_DEBUG, "free probing stream2 %d, pd->buf = 0x%x\n", st->index, pd->buf);
                    av_freep(&pd->buf);
                }
            }
            return ret;
        }
        add_to_pktbuf(&s->raw_packet_buffer, pkt, &s->raw_packet_buffer_end);
        s->raw_packet_buffer_remaining_size -= pkt->size;
        if (st->request_probe > 0) {
            pd = &st->probe_data;
            int end;
            av_log(s, AV_LOG_DEBUG, "probing stream %d pp:%d\n", st->index, st->probe_packets);
            --st->probe_packets;
            pd->buf = av_realloc(pd->buf, pd->buf_size + pkt->size + AVPROBE_PADDING_SIZE);
            av_log(s, AV_LOG_DEBUG, "probing stream %d pd->buf = 0x%x\n", st->index, pd->buf);
            memcpy(pd->buf + pd->buf_size, pkt->data, pkt->size);
            pd->buf_size += pkt->size;
            memset(pd->buf + pd->buf_size, 0, AVPROBE_PADDING_SIZE);
            end =    s->raw_packet_buffer_remaining_size <= 0
                     || st->probe_packets <= 0;
            if (end || av_log2(pd->buf_size) != av_log2(pd->buf_size - pkt->size)) {
                int score = set_codec_from_probe_data(s, st, pd);
                if ((st->codec->codec_id != CODEC_ID_NONE && score > AVPROBE_SCORE_MAX / 4)
                    || end
                    || (st->codec->codec_id != CODEC_ID_NONE && s->pb && s->pb->fastdetectedinfo && score > 0)) { /*if is slowmedia do short detect.*/
                    pd->buf_size = 0;
                    av_freep(&pd->buf);
                    st->request_probe = -1;
                    if (st->codec->codec_id != CODEC_ID_NONE) {
                        av_log(s, AV_LOG_ERROR, "probed stream %d codec_id:%d\n", st->index, s->streams[pkt->stream_index]->codec->codec_id);
                    } else {
                        av_log(s, AV_LOG_ERROR, "probed stream %d failed\n", st->index);
                        if (!strcmp(s->iformat->name, "rtp")) {
                            s->streams[pkt->stream_index]->codec->codec_id = CODEC_ID_H264;
                            s->streams[pkt->stream_index]->codec->codec_type = AVMEDIA_TYPE_VIDEO;
                            av_log(s, AV_LOG_ERROR, "probed stream failed 2. force h264\n");
                        }
                    }
                }
            }
        }
    }
    if (pd) {
        if (pd->buf) {
            av_log(s, AV_LOG_DEBUG, "free probing stream3 %d, pd->buf = 0x%x\n", st->index, pd->buf);
            av_freep(&pd->buf);
        }
    }
}

/**********************************************************/

/**
 * Get the number of samples of an audio frame. Return -1 on error.
 */
static int get_audio_frame_size(AVCodecContext *enc, int size)
{
    int frame_size;
    if (enc->codec_id == CODEC_ID_VORBIS) {
        return -1;
    }
    if (enc->frame_size <= 1) {
        int bits_per_sample = av_get_bits_per_sample(enc->codec_id);
        if (bits_per_sample) {
            if (enc->channels == 0) {
                return -1;
            }
            frame_size = (size << 3) / (bits_per_sample * enc->channels);
        } else {
            /* used for example by ADPCM codecs */
            if (enc->bit_rate == 0) {
                return -1;
            }
            frame_size = ((int64_t)size * 8 * enc->sample_rate) / enc->bit_rate;
        }
    } else {
        frame_size = enc->frame_size;
    }
    return frame_size;
}


/**
 * Return the frame duration in seconds. Return 0 if not available.
 */
static void compute_frame_duration(int *pnum, int *pden, AVStream *st,
                                   AVCodecParserContext *pc, AVPacket *pkt)
{
    int frame_size;
    *pnum = 0;
    *pden = 0;
    switch (st->codec->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        if (st->time_base.num * 1000LL > st->time_base.den) {
            *pnum = st->time_base.num;
            *pden = st->time_base.den;
        } else if (st->codec->time_base.num * 1000LL > st->codec->time_base.den) {
            *pnum = st->codec->time_base.num;
            *pden = st->codec->time_base.den;
            if (pc && pc->repeat_pict) {
                *pnum = (*pnum) * (1 + pc->repeat_pict);
            }
            //If this codec can be interlaced or progressive then we need a parser to compute duration of a packet
            //Thus if we have no parser in such case leave duration undefined.
            if (st->codec->ticks_per_frame > 1 && !pc) {
                *pnum = *pden = 0;
            }
        }
        break;
    case AVMEDIA_TYPE_AUDIO:
        frame_size = get_audio_frame_size(st->codec, pkt->size);
        if (frame_size <= 0 || st->codec->sample_rate <= 0) {
            break;
        }
        *pnum = frame_size;
        *pden = st->codec->sample_rate;
        break;
    default:
        break;
    }
}

static int is_intra_only(AVCodecContext *enc)
{
    if (enc->codec_type == AVMEDIA_TYPE_AUDIO) {
        return 1;
    } else if (enc->codec_type == AVMEDIA_TYPE_VIDEO) {
        switch (enc->codec_id) {
        case CODEC_ID_MJPEG:
        case CODEC_ID_MJPEGB:
        case CODEC_ID_LJPEG:
        case CODEC_ID_RAWVIDEO:
        case CODEC_ID_DVVIDEO:
        case CODEC_ID_HUFFYUV:
        case CODEC_ID_FFVHUFF:
        case CODEC_ID_ASV1:
        case CODEC_ID_ASV2:
        case CODEC_ID_VCR1:
        case CODEC_ID_DNXHD:
        case CODEC_ID_JPEG2000:
            return 1;
        default:
            break;
        }
    }
    return 0;
}

static void update_initial_timestamps(AVFormatContext *s, int stream_index,
                                      int64_t dts, int64_t pts)
{
    AVStream *st = s->streams[stream_index];
    AVPacketList *pktl = s->packet_buffer;
    if (st->first_dts != AV_NOPTS_VALUE || dts == AV_NOPTS_VALUE || st->cur_dts == AV_NOPTS_VALUE) {
        return;
    }
    st->first_dts = dts - st->cur_dts;
    st->cur_dts = dts;
    for (; pktl; pktl = pktl->next) {
        if (pktl->pkt.stream_index != stream_index) {
            continue;
        }
        //FIXME think more about this check
        if (pktl->pkt.pts != AV_NOPTS_VALUE && pktl->pkt.pts == pktl->pkt.dts) {
            pktl->pkt.pts += st->first_dts;
        }
        if (pktl->pkt.dts != AV_NOPTS_VALUE) {
            pktl->pkt.dts += st->first_dts;
        }
        if (st->start_time == AV_NOPTS_VALUE && pktl->pkt.pts != AV_NOPTS_VALUE) {
            st->start_time = pktl->pkt.pts;
        }
    }
    if (st->start_time == AV_NOPTS_VALUE) {
        st->start_time = pts;
    }
}

static void update_initial_durations(AVFormatContext *s, AVStream *st, AVPacket *pkt)
{
    AVPacketList *pktl = s->packet_buffer;
    int64_t cur_dts = 0;
    if (st->first_dts != AV_NOPTS_VALUE) {
        cur_dts = st->first_dts;
        for (; pktl; pktl = pktl->next) {
            if (pktl->pkt.stream_index == pkt->stream_index) {
                if (pktl->pkt.pts != pktl->pkt.dts || pktl->pkt.dts != AV_NOPTS_VALUE || pktl->pkt.duration) {
                    break;
                }
                cur_dts -= pkt->duration;
            }
        }
        pktl = s->packet_buffer;
        st->first_dts = cur_dts;
    } else if (st->cur_dts) {
        return;
    }
    for (; pktl; pktl = pktl->next) {
        if (pktl->pkt.stream_index != pkt->stream_index) {
            continue;
        }
        if (pktl->pkt.pts == pktl->pkt.dts && pktl->pkt.dts == AV_NOPTS_VALUE
            && !pktl->pkt.duration) {
            pktl->pkt.dts = cur_dts;
            if (!st->codec->has_b_frames) {
                pktl->pkt.pts = cur_dts;
            }
            cur_dts += pkt->duration;
            pktl->pkt.duration = pkt->duration;
        } else {
            break;
        }
    }
    if (st->first_dts == AV_NOPTS_VALUE) {
        st->cur_dts = cur_dts;
    }
}

static void compute_pkt_fields(AVFormatContext *s, AVStream *st,
                               AVCodecParserContext *pc, AVPacket *pkt)
{
    int num, den, presentation_delayed, delay, i;
    int64_t offset;
    int codec_id_cond = st->codec->codec_id != CODEC_ID_H264 && st->codec->codec_id != CODEC_ID_HEVC;
    if (s->flags & AVFMT_FLAG_NOFILLIN) {
        return;
    }
    if ((s->flags & AVFMT_FLAG_IGNDTS) && pkt->pts != AV_NOPTS_VALUE) {
        pkt->dts = AV_NOPTS_VALUE;
    }
    if (codec_id_cond && pc && pc->pict_type == AV_PICTURE_TYPE_B)
        //FIXME Set low_delay = 0 when has_b_frames = 1
    {
        st->codec->has_b_frames = 1;
    }
    /* do we have a video B-frame ? */
    delay = st->codec->has_b_frames;
    presentation_delayed = 0;
    // ignore delay caused by frame threading so that the mpeg2-without-dts
    // warning will not trigger
    if (delay && st->codec->active_thread_type & FF_THREAD_FRAME) {
        delay -= st->codec->thread_count - 1;
    }
    /* XXX: need has_b_frame, but cannot get it if the codec is
        not initialized */
    if (delay &&
        pc && pc->pict_type != AV_PICTURE_TYPE_B) {
        presentation_delayed = 1;
    }
    if (pkt->pts != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && pkt->dts > pkt->pts && st->pts_wrap_bits < 63
        /*&& pkt->dts-(1LL<<st->pts_wrap_bits) < pkt->pts*/) {
        pkt->dts -= 1LL << st->pts_wrap_bits;
    }
    // some mpeg2 in mpeg-ps lack dts (issue171 / input_file.mpg)
    // we take the conservative approach and discard both
    // Note, if this is misbehaving for a H.264 file then possibly presentation_delayed is not set correctly.
    if (delay == 1 && pkt->dts == pkt->pts && pkt->dts != AV_NOPTS_VALUE && presentation_delayed) {
        av_log(s, AV_LOG_DEBUG, "invalid dts/pts combination\n");
        if ((st->codec->codec_id == CODEC_ID_MPEG2VIDEO) &&
            !strcmp(s->iformat->name, "mpeg")) {
            pkt->dts = pkt->pts = AV_NOPTS_VALUE;
        }
    }
    if (pkt->duration == 0 && st->need_parsing && st->parser != NULL && st->codec->codec_id == CODEC_ID_DTS && st->codec->sample_rate > 0
        && st->time_base.den != 0 && st->time_base.num != 0) {
        pkt->duration = av_rescale_rnd(1, st->parser->duration * (int64_t)st->time_base.den, st->codec->sample_rate * (int64_t)st->time_base.num, AV_ROUND_DOWN);
    }
    if (pkt->duration == 0) {
        compute_frame_duration(&num, &den, st, pc, pkt);
        if (den && num) {
            pkt->duration = av_rescale_rnd(1, num * (int64_t)st->time_base.den, den * (int64_t)st->time_base.num, AV_ROUND_DOWN);
            if (pkt->duration != 0 && s->packet_buffer) {
                update_initial_durations(s, st, pkt);
            }
        }
    }
    /* correct timestamps with byte offset if demuxers only have timestamps
       on packet boundaries */
    if (pc && st->need_parsing == AVSTREAM_PARSE_TIMESTAMPS && pkt->size) {
        /* this will estimate bitrate based on this frame's duration and size */
        offset = av_rescale(pc->offset, pkt->duration, pkt->size);
        if (pkt->pts != AV_NOPTS_VALUE) {
            pkt->pts += offset;
        }
        if (pkt->dts != AV_NOPTS_VALUE) {
            pkt->dts += offset;
        }
    }
    if (pc && pc->dts_sync_point >= 0) {
        // we have synchronization info from the parser
        int64_t den = st->codec->time_base.den * (int64_t) st->time_base.num;
        if (den > 0) {
            int64_t num = st->codec->time_base.num * (int64_t) st->time_base.den;
            if (pkt->dts != AV_NOPTS_VALUE) {
                // got DTS from the stream, update reference timestamp
                st->reference_dts = pkt->dts - pc->dts_ref_dts_delta * num / den;
                pkt->pts = pkt->dts + pc->pts_dts_delta * num / den;
            } else if (st->reference_dts != AV_NOPTS_VALUE) {
                // compute DTS based on reference timestamp
                pkt->dts = st->reference_dts + pc->dts_ref_dts_delta * num / den;
                pkt->pts = pkt->dts + pc->pts_dts_delta * num / den;
            }
            if (pc->dts_sync_point > 0) {
                st->reference_dts = pkt->dts;    // new reference
            }
        }
    }
    /* This may be redundant, but it should not hurt. */
    if (pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->pts > pkt->dts) {
        presentation_delayed = 1;
    }
    //    av_log(NULL, AV_LOG_DEBUG, "IN delayed:%d pts:%"PRId64", dts:%"PRId64" cur_dts:%"PRId64" st:%d pc:%p\n", presentation_delayed, pkt->pts, pkt->dts, st->cur_dts, pkt->stream_index, pc);
    /* interpolate PTS and DTS if they are not present */
    //We skip H264 currently because delay and has_b_frames are not reliably set
    if ((delay == 0 || (delay == 1 && pc)) && codec_id_cond) {
        if (presentation_delayed) {
            /* DTS = decompression timestamp */
            /* PTS = presentation timestamp */
            if (pkt->dts == AV_NOPTS_VALUE) {
                pkt->dts = st->last_IP_pts;
            }
            update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts);
            if (pkt->dts == AV_NOPTS_VALUE) {
                pkt->dts = st->cur_dts;
            }
            /* this is tricky: the dts must be incremented by the duration
            of the frame we are displaying, i.e. the last I- or P-frame */
            if (st->last_IP_duration == 0) {
                st->last_IP_duration = pkt->duration;
            }
            if (pkt->dts != AV_NOPTS_VALUE) {
                st->cur_dts = pkt->dts + st->last_IP_duration;
            }
            st->last_IP_duration  = pkt->duration;
            st->last_IP_pts = pkt->pts;
            /* cannot compute PTS if not present (we can compute it only
            by knowing the future */
        } else if (pkt->pts != AV_NOPTS_VALUE || pkt->dts != AV_NOPTS_VALUE || pkt->duration) {
            if (pkt->pts != AV_NOPTS_VALUE && pkt->duration) {
                int64_t old_diff = FFABS(st->cur_dts - pkt->duration - pkt->pts);
                int64_t new_diff = FFABS(st->cur_dts - pkt->pts);
                if (old_diff < new_diff && old_diff < (pkt->duration >> 3)) {
                    pkt->pts += pkt->duration;
                    //                av_log(NULL, AV_LOG_DEBUG, "id:%d old:%"PRId64" new:%"PRId64" dur:%d cur:%"PRId64" size:%d\n", pkt->stream_index, old_diff, new_diff, pkt->duration, st->cur_dts, pkt->size);
                }
            }
            /* presentation is not delayed : PTS and DTS are the same */
            if (pkt->pts == AV_NOPTS_VALUE) {
                pkt->pts = pkt->dts;
            }
            update_initial_timestamps(s, pkt->stream_index, pkt->pts, pkt->pts);
            if (pkt->pts == AV_NOPTS_VALUE) {
                pkt->pts = st->cur_dts;
            }
            pkt->dts = pkt->pts;
            if (pkt->pts != AV_NOPTS_VALUE) {
                st->cur_dts = pkt->pts + pkt->duration;
            }
        }
    }
    if (pkt->pts != AV_NOPTS_VALUE && delay <= MAX_REORDER_DELAY) {
        st->pts_buffer[0] = pkt->pts;
        for (i = 0; i < delay && st->pts_buffer[i] > st->pts_buffer[i + 1]; i++) {
            FFSWAP(int64_t, st->pts_buffer[i], st->pts_buffer[i + 1]);
        }
        if (pkt->dts == AV_NOPTS_VALUE) {
            pkt->dts = st->pts_buffer[0];
        }
        if (!codec_id_cond) { //we skiped it above so we try here
            //update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts); // this should happen on the first packet
            update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->dts);
        }
        if (pkt->dts > st->cur_dts) {
            st->cur_dts = pkt->dts;
        }
    }
    //    av_log(NULL, AV_LOG_ERROR, "OUTdelayed:%d/%d pts:%"PRId64", dts:%"PRId64" cur_dts:%"PRId64"\n", presentation_delayed, delay, pkt->pts, pkt->dts, st->cur_dts);
    /* update flags */
    if (is_intra_only(st->codec)) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    } else if (pc) {
        pkt->flags = 0;
        /* keyframe computation */
        if (pc->key_frame == 1) {
            pkt->flags |= AV_PKT_FLAG_KEY;
        } else if (pc->key_frame == -1 && pc->pict_type == AV_PICTURE_TYPE_I) {
            pkt->flags |= AV_PKT_FLAG_KEY;
        }
    }
    if (pc) {
        pkt->convergence_duration = pc->convergence_duration;
    }
    if (pc == NULL && pkt->pts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && delay == 0) {
        pkt->pts = pkt->dts;
    }
}


static int av_read_frame_internal(AVFormatContext *s, AVPacket *pkt)
{
    AVStream *st;
    int len, ret, i;
    av_init_packet(pkt);
    for (;;) {
        /* select current input stream component */
        st = s->cur_st;
        if (st) {
            if (!st->need_parsing || !st->parser) {
                /* no parsing needed: we just output the packet as is */
                /* raw data support */
                *pkt = st->cur_pkt;
                st->cur_pkt.data = NULL;
                compute_pkt_fields(s, st, NULL, pkt);
                s->cur_st = NULL;
                if ((st->codec->codec_type == AVMEDIA_TYPE_VIDEO) && (s->iformat->flags & AVFMT_GENERIC_INDEX) &&
                    (pkt->flags & AV_PKT_FLAG_KEY) && pkt->dts != AV_NOPTS_VALUE) {
                    ff_reduce_index(s, st->index);
                    av_add_index_entry(st, pkt->pos, pkt->dts, 0, 0, AVINDEX_KEYFRAME);
                }
                break;
            } else if (st->cur_len > 0 && st->discard < AVDISCARD_ALL) {
                len = av_parser_parse2(st->parser, st->codec, &pkt->data, &pkt->size,
                                       st->cur_ptr, st->cur_len,
                                       st->cur_pkt.pts, st->cur_pkt.dts,
                                       st->cur_pkt.pos);
                st->cur_pkt.pts = AV_NOPTS_VALUE;
                st->cur_pkt.dts = AV_NOPTS_VALUE;
                /* increment read pointer */
                st->cur_ptr += len;
                st->cur_len -= len;
                /* return packet if any */
                if (pkt->size) {
got_packet:
                    pkt->duration = 0;
                    pkt->stream_index = st->index;
                    pkt->pts = st->parser->pts;
                    pkt->dts = st->parser->dts;
                    pkt->pos = st->parser->pos;
                    if (pkt->data == st->cur_pkt.data && pkt->size == st->cur_pkt.size) {
                        s->cur_st = NULL;
                        pkt->destruct = st->cur_pkt.destruct;
                        st->cur_pkt.destruct = NULL;
                        st->cur_pkt.data    = NULL;
                        assert(st->cur_len == 0);
                    } else {
                        pkt->destruct = NULL;
                    }
                    int64_t old_pts = pkt->pts;
                    int64_t old_dts = pkt->dts;
                    compute_pkt_fields(s, st, st->parser, pkt);
                    if (!strcmp(s->iformat->name, "mpegts") &&
                        !(s->pb && (s->pb->seekflags & PLAYER_ON_SEEKING)))
                        /*for mpegts compute pts error,
                          but if on seek,compute all pts,ignore the errors.
                        */
                    {
                        pkt->pts = old_pts;
                        pkt->dts = old_dts;
                    }
                    if ((st->codec->codec_type == AVMEDIA_TYPE_VIDEO) && (s->iformat->flags & AVFMT_GENERIC_INDEX) && pkt->flags & AV_PKT_FLAG_KEY) {
                        int64_t pos = (st->parser->flags & PARSER_FLAG_COMPLETE_FRAMES) ? pkt->pos : st->parser->frame_offset;
                        ff_reduce_index(s, st->index);
                        av_add_index_entry(st, pos, pkt->dts,
                                           0, 0, AVINDEX_KEYFRAME);
                    }
                    break;
                }
            } else {
                /* free packet */
                av_free_packet(&st->cur_pkt);
                s->cur_st = NULL;
            }
        } else {
            AVPacket cur_pkt;
            /* read next packet */
            ret = av_read_packet(s, &cur_pkt);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN)) {
                    return ret;
                }
                /* return the last frames, if any */
                for (i = 0; i < s->nb_streams; i++) {
                    st = s->streams[i];
                    if (st->parser && st->need_parsing) {
                        av_parser_parse2(st->parser, st->codec,
                                         &pkt->data, &pkt->size,
                                         NULL, 0,
                                         AV_NOPTS_VALUE, AV_NOPTS_VALUE,
                                         AV_NOPTS_VALUE);
                        if (pkt->size) {
                            goto got_packet;
                        }
                    }
                }
                /* no more packets: really terminate parsing */
                return ret;
            }
            st = s->streams[cur_pkt.stream_index];
            st->cur_pkt = cur_pkt;
            if (st->cur_pkt.pts != AV_NOPTS_VALUE &&
                st->cur_pkt.dts != AV_NOPTS_VALUE &&
                st->cur_pkt.pts < st->cur_pkt.dts) {
                av_log(s, AV_LOG_DEBUG, "Invalid timestamps stream=%d, pts=%"PRId64", dts=%"PRId64", size=%d\n",
                       st->cur_pkt.stream_index,
                       st->cur_pkt.pts,
                       st->cur_pkt.dts,
                       st->cur_pkt.size);
                //                av_free_packet(&st->cur_pkt);
                //                return -1;
            }
            if (s->debug & FF_FDEBUG_TS)
                av_log(s, AV_LOG_DEBUG, "av_read_packet stream=%d, pts=%"PRId64", dts=%"PRId64", size=%d, duration=%d, flags=%d\n",
                       st->cur_pkt.stream_index,
                       st->cur_pkt.pts,
                       st->cur_pkt.dts,
                       st->cur_pkt.size,
                       st->cur_pkt.duration,
                       st->cur_pkt.flags);
            s->cur_st = st;
            st->cur_ptr = st->cur_pkt.data;
            st->cur_len = st->cur_pkt.size;
            if (st->need_parsing && !st->parser && !(s->flags & AVFMT_FLAG_NOPARSE)) {
                AVCodec *codec = NULL;
                st->parser = av_parser_init(st->codec->codec_id);
                if (!st->parser) {
                    /* no parser available: just output the raw packets */
                    st->need_parsing = AVSTREAM_PARSE_NONE;
                } else if (st->need_parsing == AVSTREAM_PARSE_HEADERS) {
                    st->parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
                } else if (st->need_parsing == AVSTREAM_PARSE_FULL_ONCE) {
                    st->parser->flags |= PARSER_FLAG_ONCE;
                }
                if (st->parser && !st->codec->codec) {
                    codec = avcodec_find_decoder(st->codec->codec_id);
                    if ((st->codec->codec_id == CODEC_ID_VC1)/*VC1 need codec init for parser.*/
                        && codec && !st->codec->codec) {
                        avcodec_open(st->codec, codec);
                    }
                }
            }
        }
    }
    if (s->debug & FF_FDEBUG_TS)
        av_log(s, AV_LOG_DEBUG, "av_read_frame_internal stream=%d, pts=%"PRId64", dts=%"PRId64", size=%d, duration=%d, flags=%d\n",
               pkt->stream_index,
               pkt->pts,
               pkt->dts,
               pkt->size,
               pkt->duration,
               pkt->flags);
    return 0;
}

int av_read_frame(AVFormatContext *s, AVPacket *pkt)
{
    AVPacketList *pktl;
    int eof = 0;
    const int genpts = s->flags & AVFMT_FLAG_GENPTS;
    for (;;) {
        pktl = s->packet_buffer;
        if (pktl) {
            AVPacket *next_pkt = &pktl->pkt;
            if (genpts && next_pkt->dts != AV_NOPTS_VALUE) {
                int wrap_bits = s->streams[next_pkt->stream_index]->pts_wrap_bits;
                while (pktl && next_pkt->pts == AV_NOPTS_VALUE) {
                    if (pktl->pkt.stream_index == next_pkt->stream_index
                        && (0 > av_compare_mod(next_pkt->dts, pktl->pkt.dts, 2LL << (wrap_bits - 1)))
                        && av_compare_mod(pktl->pkt.pts, pktl->pkt.dts, 2LL << (wrap_bits - 1))) { //not b frame
                        next_pkt->pts = pktl->pkt.dts;
                    }
                    pktl = pktl->next;
                }
                pktl = s->packet_buffer;
            }
            if (next_pkt->pts != AV_NOPTS_VALUE
                || next_pkt->dts == AV_NOPTS_VALUE
                || !genpts || eof) {
                /* read packet from packet buffer, if there is data */
                *pkt = *next_pkt;
                s->packet_buffer = pktl->next;
                av_free(pktl);
                return 0;
            }
        }
        if (genpts) {
            int ret = av_read_frame_internal(s, pkt);
            if (ret < 0) {
                if (pktl && ret != AVERROR(EAGAIN)) {
                    eof = 1;
                    continue;
                } else {
                    return ret;
                }
            }
            if (av_dup_packet(add_to_pktbuf(&s->packet_buffer, pkt,
                                            &s->packet_buffer_end)) < 0) {
                return AVERROR(ENOMEM);
            }
        } else {
            assert(!s->packet_buffer);
            return av_read_frame_internal(s, pkt);
        }
    }
}
int av_buffering_data(AVFormatContext *s, int size)
{
    /*can add buffering on top level.
    size <0 .get buffering time;
    */
    if (s && s->iformat && s->iformat->bufferingdata) {
        return s->iformat->bufferingdata(s, size);
    }
    return -1;
}
/* XXX: suppress the packet queue */
static void flush_packet_queue(AVFormatContext *s)
{
    AVPacketList *pktl;
    for (;;) {
        pktl = s->packet_buffer;
        if (!pktl) {
            break;
        }
        s->packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    while (s->raw_packet_buffer) {
        pktl = s->raw_packet_buffer;
        s->raw_packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    s->packet_buffer_end =
        s->raw_packet_buffer_end = NULL;
    s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
}

/*******************************************************/
/* seek support */

int av_find_default_stream_index(AVFormatContext *s)
{
    int first_audio_index = -1;
    int i;
    AVStream *st;
    if (s->nb_streams <= 0) {
        return -1;
    }
    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            return i;
        }
        if (first_audio_index < 0 && st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            first_audio_index = i;
        }
    }
    return first_audio_index >= 0 ? first_audio_index : 0;
}

/**
 * Flush the frame reader.
 */
void ff_read_frame_flush(AVFormatContext *s)
{
    AVStream *st;
    int i, j;
    flush_packet_queue(s);
    s->cur_st = NULL;
    /* for each stream, reset read state */
    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (st->parser) {
            av_parser_close(st->parser);
            st->parser = NULL;
            av_free_packet(&st->cur_pkt);
        }
        st->last_IP_pts = AV_NOPTS_VALUE;
        st->cur_dts = AV_NOPTS_VALUE; /* we set the current DTS to an unspecified origin */
        st->reference_dts = AV_NOPTS_VALUE;
        /* fail safe */
        st->cur_ptr = NULL;
        st->cur_len = 0;
        st->probe_packets = MAX_PROBE_PACKETS;
        for (j = 0; j < MAX_REORDER_DELAY + 1; j++) {
            st->pts_buffer[j] = AV_NOPTS_VALUE;
        }
    }
}

void av_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp)
{
    int i;
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        st->cur_dts = av_rescale(timestamp,
                                 st->time_base.den * (int64_t)ref_st->time_base.num,
                                 st->time_base.num * (int64_t)ref_st->time_base.den);
    }
}

void ff_reduce_index(AVFormatContext *s, int stream_index)
{
    AVStream *st = s->streams[stream_index];
    unsigned int max_entries = s->max_index_size / sizeof(AVIndexEntry);
    if ((unsigned)st->nb_index_entries >= max_entries) {
        int i;
        for (i = 0; 2 * i < st->nb_index_entries; i++) {
            st->index_entries[i] = st->index_entries[2 * i];
        }
        st->nb_index_entries = i;
    }
}

int ff_add_index_entry(AVIndexEntry **index_entries,
                       int *nb_index_entries,
                       unsigned int *index_entries_allocated_size,
                       int64_t pos, int64_t timestamp, int size, int distance, int flags)
{
    AVIndexEntry *entries, *ie;
    int index;
    if ((unsigned)*nb_index_entries + 1 >= UINT_MAX / sizeof(AVIndexEntry)) {
        return -1;
    }
    entries = av_fast_realloc(*index_entries,
                              index_entries_allocated_size,
                              (*nb_index_entries + 1) *
                              sizeof(AVIndexEntry));
    if (!entries) {
        return -1;
    }
    *index_entries = entries;
    index = ff_index_search_timestamp(*index_entries, *nb_index_entries, timestamp, AVSEEK_FLAG_ANY);
    if (index < 0) {
        index = (*nb_index_entries)++;
        ie = &entries[index];
        assert(index == 0 || ie[-1].timestamp < timestamp);
    } else {
        ie = &entries[index];
        if (ie->timestamp != timestamp) {
            if (ie->timestamp <= timestamp) {
                return -1;
            }
            memmove(entries + index + 1, entries + index, sizeof(AVIndexEntry) * (*nb_index_entries - index));
            (*nb_index_entries)++;
        } else if (ie->pos == pos && distance < ie->min_distance) { //do not reduce the distance
            distance = ie->min_distance;
        }
    }
    ie->pos = pos;
    ie->timestamp = timestamp;
    ie->min_distance = distance;
    ie->size = size;
    ie->flags = flags;
    return index;
}

int av_add_index_entry(AVStream *st,
                       int64_t pos, int64_t timestamp, int size, int distance, int flags)
{
    return ff_add_index_entry(&st->index_entries, &st->nb_index_entries,
                              &st->index_entries_allocated_size, pos,
                              timestamp, size, distance, flags);
}

int ff_index_search_timestamp(const AVIndexEntry *entries, int nb_entries,
                              int64_t wanted_timestamp, int flags)
{
    int a, b, m;
    int64_t timestamp;
    a = - 1;
    b = nb_entries;
    //optimize appending index entries at the end
    if (b && entries[b - 1].timestamp < wanted_timestamp) {
        a = b - 1;
    }
    while (b - a > 1) {
        m = (a + b) >> 1;
        timestamp = entries[m].timestamp;
        if (timestamp >= wanted_timestamp) {
            b = m;
        }
        if (timestamp <= wanted_timestamp) {
            a = m;
        }
    }
    a = (a == -1) ? 0 : a;
    m = (flags & AVSEEK_FLAG_BACKWARD) ? a : b;
    if (!(flags & AVSEEK_FLAG_ANY)) {
        while (m >= 0 && m < nb_entries && !(entries[m].flags & AVINDEX_KEYFRAME)) {
            m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
        }
    }
    if (m == nb_entries) {
        return -1;
    }
    return  m;
}

int av_index_search_timestamp(AVStream *st, int64_t wanted_timestamp,
                              int flags)
{
    return ff_index_search_timestamp(st->index_entries, st->nb_index_entries,
                                     wanted_timestamp, flags);
}

int av_seek_frame_binary(AVFormatContext *s, int stream_index, int64_t target_ts, int flags)
{
    AVInputFormat *avif = s->iformat;
    int64_t av_uninit(pos_min), av_uninit(pos_max), pos, pos_limit;
    int64_t ts_min, ts_max, ts;
    int index;
    int64_t ret;
    AVStream *st;
    int64_t ts_limit = 0;
    if (stream_index < 0) {
        return -1;
    }
    if (s->seek_binary_failed > 10) {
        //av_log(NULL, AV_LOG_ERROR, "do not repeat av_seek_frame_binary, for failed before \n");
        //return -1;
    }
    av_dlog(s, "read_seek: %d %"PRId64"\n", stream_index, target_ts);
    ts_max =
        ts_min = AV_NOPTS_VALUE;
    pos_limit = -1; //gcc falsely says it may be uninitialized
    st = s->streams[stream_index];
    if (st->index_entries) {
        AVIndexEntry *e;
        index = av_index_search_timestamp(st, target_ts, flags | AVSEEK_FLAG_BACKWARD); //FIXME whole func must be checked for non-keyframe entries in index case, especially read_timestamp()
        index = FFMAX(index, 0);
        e = &st->index_entries[index];
        if (e->timestamp <= target_ts || e->pos == e->min_distance) {
            pos_min = e->pos;
            ts_min = e->timestamp;
            av_dlog(s, "using cached pos_min=0x%"PRIx64" dts_min=%"PRId64"\n",
                    pos_min, ts_min);
        } else {
            assert(index == 0);
        }
        index = av_index_search_timestamp(st, target_ts, flags & ~AVSEEK_FLAG_BACKWARD);
        assert(index < st->nb_index_entries);
        if (index >= 0) {
            e = &st->index_entries[index];
            assert(e->timestamp >= target_ts);
            pos_max = e->pos;
            ts_max = e->timestamp;
            pos_limit = pos_max - e->min_distance;
            av_dlog(s, "using cached pos_max=0x%"PRIx64" pos_limit=0x%"PRIx64" dts_max=%"PRId64"\n",
                    pos_max, pos_limit, ts_max);
        }
    }

    // update tsmin tsmax value
    if (s->pb && s->pb->is_slowmedia) {
        if (ts_max == AV_NOPTS_VALUE) {
            if (st->max_pts > 0) {
                ts_max = st->max_pts;
            }
            if (st->max_pos > 0) {
                pos_max = st->max_pos;
            }
        }
    }
    pos = av_gen_search(s, stream_index, target_ts, pos_min, pos_max, pos_limit, ts_min, ts_max, flags, &ts, avif->read_timestamp);
    if (pos < 0) {
        s->seek_binary_failed ++;
        return -1;
    }
    if (s->duration > (AV_TIME_BASE * 2048)) {
        ts_limit = s->duration >> 10;
    }
    if (ts_limit < AV_TIME_BASE) {
        ts_limit = AV_TIME_BASE << 2;    //experiment value:for ape, 2s(AV_TIME_BASE <<1) maybe not enough
    }
    if (s->pb && s->pb->is_slowmedia) {
        /*when network, ignore big difference*/
    } else if (!memcmp(s->iformat->name, "mpegts", 6)) {
        /*when mpegts, ignore big difference*/
    } else {
        if (ts > target_ts && (ts - target_ts) > ts_limit) {
            return -2;
        }
        if (ts < target_ts && (target_ts - ts) > ts_limit) {
            return -3;
        }
    }
    s->seek_binary_failed = 0;
    /* do the seek */
    if ((ret = avio_seek(s->pb, pos, SEEK_SET)) < 0) {
        return ret;
    }
    av_update_cur_dts(s, st, ts);
    return 0;
}

int64_t av_gen_search(AVFormatContext *s, int stream_index, int64_t target_ts, int64_t pos_min, int64_t pos_max, int64_t pos_limit, int64_t ts_min, int64_t ts_max, int flags, int64_t *ts_ret, int64_t (*read_timestamp)(struct AVFormatContext *, int , int64_t *, int64_t))
{
    int64_t pos, ts;
    int64_t start_pos, filesize;
    int no_change;
    int retry = 0;
    int max_retry_search = 20;
    int no_exact_seek = 0;
    int64_t seek_finish_range = -1;

    char value[PROPERTY_VALUE_MAX] = {0};
    int64_t estimate_size = DURATION_MAX_READ_SIZE;
    int ret = -1;

    if (s->pb->local_playback == 1) {
        estimate_size = DURATION_MAX_READ_LOCALPLAY_SIZE;
    }

    //read from prop
    if (property_get("media.amplayer.estimate_size", value, NULL) > 0) {
        estimate_size = (int64_t)atoi(value);
    }

    if (s->pb && s->pb->is_slowmedia) {
        max_retry_search = am_getconfig_float_def("media.libplayer.maxseekretry", 3);
        seek_finish_range = (int64_t)am_getconfig_float_def("libplayer.seek.finish_range", 3 * 90000); // default 10s
        no_exact_seek = 1;
    }
    av_dlog(s, "gen_seek: %d %"PRId64"\n", stream_index, target_ts);
    av_log(NULL, AV_LOG_INFO, "[%s]: ---------------------------------\n", __func__);
    av_log(NULL, AV_LOG_INFO, "stream_index:%d target_ts: %"PRId64" \n", stream_index, target_ts);
    av_log(NULL, AV_LOG_INFO, "pos_min:%"PRId64" ts_min: %"PRId64" \n", pos_min, ts_min);
    av_log(NULL, AV_LOG_INFO, "pox_max:%"PRId64" ts_max: %"PRId64" \n", pos_max, ts_max);
    av_log(NULL, AV_LOG_INFO, "pos_limit: %"PRId64" data_offset:%d \n", pos_limit, s->data_offset);
    av_log(NULL, AV_LOG_INFO, "[%s]: ---------------------------------\n", __func__);

RETRY_MIN:
    if (ts_min == AV_NOPTS_VALUE) {
        pos_min = s->data_offset;
        ts_min = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
        if (ts_min == AV_NOPTS_VALUE) {
            av_log(NULL, AV_LOG_ERROR, "av_gen_search failed, first pts not found\n");
            return -1;
        }
        av_log(NULL, AV_LOG_INFO, "estimate pos_min:%"PRId64" ts_min: %"PRId64" \n", pos_min, ts_min);
    }
    if (ts_max == AV_NOPTS_VALUE && s->seek_timestamp_max != 0) {
        ts_max  = s->seek_timestamp_max;
        pos_max = s->seek_pos_max;
        pos_limit = pos_max;
    }

RETRY_MAX:
    if (ts_max == AV_NOPTS_VALUE || ts_max < target_ts) {
        int64_t step = 1024;
        filesize = avio_size(s->pb);
        if (filesize > (s->valid_offset + 0x1600000)) {
            filesize = s->valid_offset;
        }

        if (filesize > estimate_size) {
            pos_max = filesize - estimate_size - 1;
        } else {
            pos_max = filesize - 1;
        }

        do {
            //if(url_interrupt_cb())
            //break;
            pos_max -= step;
            ts_max = read_timestamp(s, stream_index, &pos_max, pos_max + step);
            step += step;
            if (ts_max == AV_NOPTS_VALUE && (s->pb != NULL && s->pb->eof_reached == 1)) {
                ts_max = AV_NOPTS_VALUE;
                av_log(NULL, AV_LOG_ERROR, "av_gen_search read_timestamp eof, pos_max:%"PRId64", filesize:%"PRId64", curpos:%"PRId64"\n",
                       pos_max, filesize, avio_tell(s->pb));
                ret = AVERROR_EOF;
                break;
            }
        } while (ts_max == AV_NOPTS_VALUE && pos_max >= step && step < 0x6400000/*100M*/);

        if (ts_max == AV_NOPTS_VALUE) {
            av_log(NULL, AV_LOG_ERROR, "av_gen_search failed, max pts not found\n");
            if (ret == AVERROR_EOF) {
                //try again, by bytes
                if (target_ts > 0 && s->duration > 0 && s->valid_offset > 0) {
                    pos_max = (int64_t)(((target_ts * 1000) / s->duration) * s->valid_offset);
                    av_log(NULL, AV_LOG_ERROR, "av_gen_search retry from pos_max:%"PRId64"\n", pos_max);
                    step = 1024;
                    do {
                        //if(url_interrupt_cb())
                        //break;
                        pos_max -= step;
                        ts_max = read_timestamp(s, stream_index, &pos_max, pos_max + step);
                        step += step;
                        if (ts_max == AV_NOPTS_VALUE && (s->pb != NULL && s->pb->eof_reached == 1)) {
                            av_log(NULL, AV_LOG_ERROR, "av_gen_search try read_timestamp eof\n");
                            ret = AVERROR_EOF;
                            break;
                        }
                    } while (ret != AVERROR_EOF && ts_max == AV_NOPTS_VALUE && pos_max >= step && step < 0x6400000/*100M*/);

                    if (ts_max == AV_NOPTS_VALUE && ret == AVERROR_EOF) {
                        return ret;
                    }
                } else {
                    return ret;
                }
            } else {
                return -1;
            }
        }

        for (;;) {
            //while(ts_max > target_ts){
            int64_t tmp_pos = pos_max + 1;
            int64_t tmp_ts = read_timestamp(s, stream_index, &tmp_pos, INT64_MAX);
            if (tmp_ts == AV_NOPTS_VALUE) {
                break;
            }
            ts_max = tmp_ts;
            pos_max = tmp_pos;
            if (tmp_pos >= filesize || tmp_ts >= target_ts) {
                break;
            }
        }
        pos_limit = pos_max;
        av_log(NULL, AV_LOG_INFO, "Estimate pox_max:%"PRId64" ts_max: %"PRId64" \n", pos_max, ts_max);
    }

    if (target_ts > ts_max) {
        av_log(NULL, AV_LOG_ERROR, "target_ts(%llx) > ts_max(%llx)\n", target_ts , ts_max);
        if (flags & AVSEEK_FLAG_BACKWARD) {
            target_ts = ts_max;
        } else {
            return -2;
        }
    } else if (ts_min > ts_max) {
        return -1;
    } else if (ts_min == ts_max) {
        pos_limit = pos_min;
    }
    s->seek_timestamp_max = ts_max;
    s->seek_pos_max = pos_max;
    pos_limit = pos_max;
    if (seek_finish_range != -1) {
        int match_flag = 0;
        int64_t match_pos = -1;
        int64_t match_ts = -1;
        int match_type = -1;  // 0 max 1 min
        if (llabs(target_ts - ts_max) < seek_finish_range) {
            av_log(NULL, AV_LOG_INFO, "No need to seek, equal to ts_max \n");
            match_ts = ts_max;
            match_pos = pos_max;
            match_flag = 1;
            match_type = 0;
        } else if (llabs(target_ts - ts_min) < seek_finish_range) {
            av_log(NULL, AV_LOG_INFO, "No need to seek, equal to ts_min.\n");
            match_ts = ts_min;
            match_pos = pos_min;
            match_flag = 1;
            match_type = 1;
        }

        if (match_flag) {
            ts = read_timestamp(s, stream_index, &match_pos, INT64_MAX);
            if (llabs(target_ts - ts) < seek_finish_range) {
                *ts_ret = ts;
                av_log(NULL, AV_LOG_INFO, "ts match, %lld %lld %lld \n", ts_min, ts_max, ts);
                return match_pos;
            }
            av_log(NULL, AV_LOG_INFO, "ts not match, %lld %lld %lld \n", ts_min, ts_max, ts);
            if (match_type == 0) {
                ts_max = AV_NOPTS_VALUE;
                goto RETRY_MAX;
            }
            if (match_type == 1) {
                ts_min = AV_NOPTS_VALUE;
                goto RETRY_MIN;
            }
        }

    }

    no_change = 0;
    while (pos_min < pos_limit && pos_limit <= pos_max && ts_min < ts_max) {
        int64_t approximate_keyframe_distance = pos_max - pos_limit;
        if (approximate_keyframe_distance == 0) {
            approximate_keyframe_distance = 188 * 10;
            if (approximate_keyframe_distance > (pos_max - pos_min) / 3) {
                approximate_keyframe_distance = (pos_max - pos_min) / 3;
            }
        }

        if (no_exact_seek) {
            // range check
            if (seek_finish_range != -1 && llabs(target_ts - ts) < seek_finish_range) {
                av_log(s, AV_LOG_INFO, "seek end when,retry =%d,no_change=%d\n", retry, no_change);
                pos_min = pos_max = pos;
                ts_min = ts_max = ts;
                break;
            }
        }

        if (++retry > max_retry_search) {
            av_log(s, AV_LOG_INFO, "seek end when,retry =%d,no_change=%d\n", --retry, no_change);
            break;
        }

        av_log(s, AV_LOG_INFO, "pos_min=0x%"PRIx64" pos_max=0x%"PRIx64" dts_min=%"PRId64" dts_max=%"PRId64"\n",
               pos_min, pos_max, ts_min, ts_max);
        assert(pos_limit <= pos_max);

        if (no_change == 0) {
            // interpolate position (better than dichotomy)
            pos = av_rescale(target_ts - ts_min, pos_max - pos_min, ts_max - ts_min)
                  + pos_min - approximate_keyframe_distance;
        } else if (no_change == 1) {
            // bisection, if interpolation failed to change min or max pos last time
            ///pos = (pos_min + pos_limit)>>1;
            pos = pos_limit - approximate_keyframe_distance;
        } else {
            /* linear search if bisection failed, can only happen if there
               are very few or no keyframes between min/max */
            pos = pos_min;
        }
        if (pos <= pos_min) {
            pos = pos_min + 1;
        } else if (pos > pos_limit) {
            pos = pos_limit;
        }
        start_pos = pos;

        ts = read_timestamp(s, stream_index, &pos, INT64_MAX); //may pass pos_limit instead of -1
        if (pos == pos_max) {
            no_change++;
        } else {
            no_change = 0;
        }
        av_log(s, AV_LOG_INFO, "target_ts-ts=0x%"PRIx64"\n", target_ts - ts);
        av_dlog(s, "%"PRId64" %"PRId64" %"PRId64" / %"PRId64" %"PRId64" %"PRId64" target:%"PRId64" limit:%"PRId64" start:%"PRId64" noc:%d\n",
                pos_min, pos, pos_max, ts_min, ts, ts_max, target_ts,
                pos_limit, start_pos, no_change);
        if (ts == AV_NOPTS_VALUE) {
            av_log(s, AV_LOG_ERROR, "read_timestamp() failed in the middle\n");
            return -1;
        }
        assert(ts != AV_NOPTS_VALUE);
        if (target_ts <= ts) {
            pos_limit = start_pos - 1;
            pos_max = pos;
            ts_max = ts;
        }
        if (target_ts >= ts) {
            pos_min = pos;
            ts_min = ts;
        }
    }

    pos = (flags & AVSEEK_FLAG_BACKWARD) ? pos_min : pos_max;
    ts  = (flags & AVSEEK_FLAG_BACKWARD) ?  ts_min :  ts_max;
    pos_min = pos;
    ts_min = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
    pos_min++;
    ts_max = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
    av_dlog(s, "pos=0x%"PRIx64" %"PRId64"<=%"PRId64"<=%"PRId64"\n",
            pos, ts_min, target_ts, ts_max);
    *ts_ret = ts;
    return pos;
}

static int av_seek_frame_byte(AVFormatContext *s, int stream_index, int64_t pos, int flags)
{
    int64_t pos_min, pos_max;
#if 0
    AVStream *st;
    if (stream_index < 0) {
        return -1;
    }
    st = s->streams[stream_index];
#endif
    pos_min = s->data_offset;
    pos_max = avio_size(s->pb) - 1;
    if (pos < pos_min) {
        pos = pos_min;
    } else if (pos > pos_max) {
        pos = pos_max;
    }
    avio_seek(s->pb, pos, SEEK_SET);
#if 0
    av_update_cur_dts(s, st, ts);
#endif
    return 0;
}

static int av_seek_frame_generic(AVFormatContext *s,
                                 int stream_index, int64_t timestamp, int flags)
{
    int index;
    int64_t ret;
    AVStream *st;
    AVIndexEntry *ie;
    st = s->streams[stream_index];
    index = av_index_search_timestamp(st, timestamp, flags);
    if (index < 0 && st->nb_index_entries && timestamp < st->index_entries[0].timestamp) {
        return -1;
    }
    if (index < 0 || index == st->nb_index_entries - 1) {
        int i;
        AVPacket pkt;
        if (st->nb_index_entries) {
            assert(st->index_entries);
            ie = &st->index_entries[st->nb_index_entries - 1];
            if ((ret = avio_seek(s->pb, ie->pos, SEEK_SET)) < 0) {
                return ret;
            }
            av_update_cur_dts(s, st, ie->timestamp);
        } else {
            if ((ret = avio_seek(s->pb, s->data_offset, SEEK_SET)) < 0) {
                return ret;
            }
        }
        for (i = 0;; i++) {
            int ret;
            do {
                ret = av_read_frame(s, &pkt);
            } while (ret == AVERROR(EAGAIN));
            if (ret < 0) {
                break;
            }
            av_free_packet(&pkt);
            if (stream_index == pkt.stream_index) {
                if ((pkt.flags & AV_PKT_FLAG_KEY) && pkt.dts > timestamp) {
                    break;
                }
            }
        }
        index = av_index_search_timestamp(st, timestamp, flags);
    }
    if (index < 0) {
        return -1;
    }
    ff_read_frame_flush(s);
    if (s->iformat->read_seek) {
        if (s->iformat->read_seek(s, stream_index, timestamp, flags) >= 0) {
            return 0;
        }
    }
    ie = &st->index_entries[index];
    if ((ret = avio_seek(s->pb, ie->pos, SEEK_SET)) < 0) {
        return ret;
    }
    av_update_cur_dts(s, st, ie->timestamp);
    return 0;
}

int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    int ret;
    AVStream *st;
    ff_read_frame_flush(s);
#if 1
    if (s->pb && s->pb->is_slowmedia) {
        if (!memcmp(s->iformat->name, "mpegts", 6)) {
            ret = url_fseektotime(s->pb, timestamp / (1000 * 1000), flags);
            if (ret >= 0) { /*seek successed.*/
                return ret;
            }
        }
    }
#endif
    if (flags & AVSEEK_FLAG_BYTE) {
        return av_seek_frame_byte(s, stream_index, timestamp, flags);
    }
    if (stream_index < 0) {
        stream_index = av_find_default_stream_index(s);
        if (stream_index < 0) {
            return -1;
        }
    }
    st = s->streams[stream_index];
    /* timestamp for default must be expressed in AV_TIME_BASE units */
    timestamp = av_rescale(timestamp, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
    /* first, we try the format specific seek */
    if (s->iformat->read_seek) {
        ret = s->iformat->read_seek(s, stream_index, timestamp, flags);
    } else {
        ret = -1;
    }
    if (ret >= 0) {
        return 0;
    }
    if (s->iformat->read_timestamp && !(s->iformat->flags & AVFMT_NOBINSEARCH)) {
        return av_seek_frame_binary(s, stream_index, timestamp, flags);
    } else if (!(s->iformat->flags & AVFMT_NOGENSEARCH)) {
        return av_seek_frame_generic(s, stream_index, timestamp, flags);
    } else {
        return ret;
    }
}

int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
    if (min_ts > ts || max_ts < ts) {
        return -1;
    }
    ff_read_frame_flush(s);
    if (s->iformat->read_seek2) {
        return s->iformat->read_seek2(s, stream_index, min_ts, ts, max_ts, flags);
    }
    if (s->iformat->read_timestamp) {
        //try to seek via read_timestamp()
    }
    //Fallback to old API if new is not implemented but old is
    //Note the old has somewat different sematics
    if (s->iformat->read_seek || 1) {
        return av_seek_frame(s, stream_index, ts, flags | (ts - min_ts > (uint64_t)(max_ts - ts) ? AVSEEK_FLAG_BACKWARD : 0));
    }
    // try some generic seek like av_seek_frame_generic() but with new ts semantics
}

/*******************************************************/

/**
 * Return TRUE if the stream has accurate duration in any stream.
 *
 * @return TRUE if the stream has accurate duration for at least one component.
 */
static int av_has_duration(AVFormatContext *ic)
{
    int i;
    AVStream *st;
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->duration != AV_NOPTS_VALUE) {
            return 1;
        }
    }
    return 0;
}

/**
 * Estimate the stream timings from the one of each components.
 *
 * Also computes the global bitrate if possible.
 */
static void av_update_stream_timings(AVFormatContext *ic)
{
    int64_t start_time, start_time1, start_time_text, end_time, end_time1;
    int64_t duration, duration1;
    int i;
    AVStream *st;
    int bit_rate = 0;
    start_time = INT64_MAX;
    start_time_text = INT64_MAX;
    end_time = INT64_MIN;
    duration = INT64_MIN;
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        bit_rate += st->codec->bit_rate;
        /* added by Z.C. to set start time */
        if (st->start_time == 0) {
            if (st->nb_index_entries && st->index_entries[0].timestamp > 0) {
                st->start_time = st->index_entries[0].timestamp;
                av_log(NULL, AV_LOG_INFO, "[%s:%d] set stream %d start_time to first pts 0x%llx\n",
                       __FUNCTION__, __LINE__, i, st->start_time);
            }
        }
        if (st->start_time != AV_NOPTS_VALUE && st->time_base.den) {
            start_time1 = av_rescale_q(st->start_time, st->time_base, AV_TIME_BASE_Q);
            if (st->codec->codec_id == CODEC_ID_DVB_TELETEXT) {
                if (start_time1 < start_time_text) {
                    start_time_text = start_time1;
                }
            } else if (start_time1 < start_time) {
                start_time = start_time1;
            }
            if (st->duration != AV_NOPTS_VALUE) {
                end_time1 = start_time1
                            + av_rescale_q(st->duration, st->time_base, AV_TIME_BASE_Q);
                if (end_time1 > end_time) {
                    end_time = end_time1;
                }
            }
        }
        if (st->duration != AV_NOPTS_VALUE && st->start_time != AV_NOPTS_VALUE) {
            duration1 = av_rescale_q(st->duration, st->time_base, AV_TIME_BASE_Q);
            if (duration1 > duration) {
                duration = duration1;
            }
        }
    }
    if (start_time == INT64_MAX || (start_time > start_time_text && start_time - start_time_text < AV_TIME_BASE)) {
        start_time = start_time_text;
    }
    if (start_time != INT64_MAX) {
        ic->start_time = start_time;
        if (end_time != INT64_MIN && duration == INT64_MIN) {
            if (end_time - start_time > duration) {
                duration = end_time - start_time;
            }
        }
    }
    if (duration != INT64_MIN) {
        ic->duration = duration;
        if (ic->file_size > 0 && ic->bit_rate <= 0) {
            /*if bitrate have set,don't change it by file size,because the file size maybe not real media data for m3u demux*/
            /* compute the bitrate */
            ic->bit_rate = (double)ic->file_size * 8.0 * AV_TIME_BASE /
                           (double)ic->duration;
            if (bit_rate > ic->bit_rate || (ic->bit_rate - bit_rate) > 1000000000) {
                ic->bit_rate = bit_rate ;
            }
        }
#define IS_GTHD_VBR(a,b) ((5*a > 6*b) ? 1 : 0) // if (real bitrate > avg bitrate 20%) we think it is VBR, the offset for seek use the time ratio.
#define IS_LTHD_VBR(a,b) ((5*a < 4*b) ? 1 : 0)
        if (ic->file_size > 0 && ic->pb && !ic->pb->is_slowmedia) {
            unsigned int avg_bitrate = (double)ic->file_size * 8.0 * AV_TIME_BASE / (double)ic->duration;
            if (IS_GTHD_VBR(ic->bit_rate, avg_bitrate) || IS_LTHD_VBR(ic->bit_rate, avg_bitrate)) {
                ic->is_vbr = 1;
            } else {
                ic->is_vbr = -1;
            }
        }
    }
}

static void fill_all_stream_timings(AVFormatContext *ic)
{
    int i;
    AVStream *st;
    av_update_stream_timings(ic);
    av_log(NULL, AV_LOG_INFO, "[%s:%d] ic->start_time=0x%llx ic->duration=%llx\n", __FUNCTION__, __LINE__, ic->start_time, ic->duration);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->start_time == AV_NOPTS_VALUE) {
            if (ic->start_time != AV_NOPTS_VALUE) {
                st->start_time = av_rescale_q(ic->start_time, AV_TIME_BASE_Q, st->time_base);
            }
            if (ic->duration != AV_NOPTS_VALUE) {
                st->duration = av_rescale_q(ic->duration, AV_TIME_BASE_Q, st->time_base);
            }
        }
    }
}

static void av_estimate_timings_from_bit_rate(AVFormatContext *ic)
{
    int64_t filesize, duration;
    int bit_rate, i;
    AVStream *st;
    int64_t fulltimesecs;
    int bit_rate_err = 0;
    /* if bit_rate is already set, we believe it */
    if (ic->bit_rate <= 0) {
        bit_rate = 0;
        for (i = 0; i < ic->nb_streams; i++) {
            st = ic->streams[i];
            if ((st->codec->bit_rate > 0)) {
                bit_rate += st->codec->bit_rate;
            } else {
                bit_rate_err++;
            }
            if (bit_rate_err != 0 &&
                ic->nb_streams != 1 &&
                bit_rate_err >= ic->nb_streams / 2
               ) {
                bit_rate = 0;
            }
        }
        ic->bit_rate = bit_rate;
    }
    /* if duration is already set, we believe it */
    if (ic->duration == AV_NOPTS_VALUE &&
        ic->bit_rate != 0 &&
        ic->file_size != 0) {
        filesize = ic->file_size;
        if (filesize > 0) {
            for (i = 0; i < ic->nb_streams; i++) {
                st = ic->streams[i];
                duration = av_rescale(8 * filesize, st->time_base.den, ic->bit_rate * (int64_t)st->time_base.num);
                if (st->duration == AV_NOPTS_VALUE) {
                    st->duration = duration;
                }
            }
        }
    }
    if (ic->duration == AV_NOPTS_VALUE &&
        ic->bit_rate == 0 &&
        ic->nb_chapters > 0) {
        AVChapter *ch = ic->chapters[ic->nb_chapters - 1];
        fulltimesecs = (int64_t)(ch->end * av_q2d(ch->time_base));
        av_log(NULL, AV_LOG_INFO, "chapters fulltime secs [%lld] \n", fulltimesecs, st->time_base.num, st->time_base.den);
        duration = av_rescale(fulltimesecs, st->time_base.den, (int64_t)st->time_base.num);
        if (st->duration == AV_NOPTS_VALUE) {
            st->duration = duration ;
        }
    }
}


static int64_t seek_last_valid_pkt(AVFormatContext *ic)
{
    int64_t filesize, offset;
    int64_t start_offset, end_offset;
    int ret;
    unsigned char *buf1;
    unsigned char *buf2;
    buf1 = av_malloc(CHECK_FULL_ZERO_SIZE);
    if (!buf1) {
        return -2;
    }
    buf2 = av_malloc(CHECK_FULL_ZERO_SIZE);
    if (!buf2) {
        av_free(buf1);
        return -3;
    }
    memset(buf1, 0, CHECK_FULL_ZERO_SIZE);
    memset(buf2, 0, CHECK_FULL_ZERO_SIZE);
    filesize = ic->file_size;
    start_offset = 0;
    end_offset = filesize;
    for (;;) {
        //av_log(NULL, AV_LOG_INFO, "[%s:%d]start=0x%llx end=0x%llx end-start=0x%llx\n",__FUNCTION__,__LINE__,start_offset,end_offset,end_offset - start_offset);
        if (start_offset >= end_offset) {
            break;
        }
        if ((end_offset - start_offset) < (CHECK_FULL_ZERO_SIZE << 1)) {
            av_log(NULL, AV_LOG_INFO, "[%s:%d]last block!offset=0x%llx\n", __FUNCTION__, __LINE__, start_offset);
            av_free(buf1);
            av_free(buf2);
            return start_offset;
        }
        offset = (start_offset + end_offset) >> 1;
        //av_log(NULL, AV_LOG_INFO, "[%s:%d]offset=0x%llx\n",__FUNCTION__,__LINE__,offset);
        avio_seek(ic->pb, offset, SEEK_SET);
        do {
            ret = avio_read(ic->pb, buf1, CHECK_FULL_ZERO_SIZE);
        } while (ret == AVERROR(EAGAIN));
        if (ret < 0) {
            av_log(NULL, AV_LOG_INFO, "[%s:%d]av_read_packet failed ret=%d\n", __FUNCTION__, __LINE__, ret);
            break;
        }
        if (memcmp(buf1, buf2, CHECK_FULL_ZERO_SIZE) == 0) { //cmp,buf1=buf2=0
            //av_log(NULL, AV_LOG_INFO, "[%s:%d]first block buf1=0\n",__FUNCTION__,__LINE__);
            end_offset = offset;            //head -full zero
            continue;
        } else { //buf1<>0
            //av_log(NULL, AV_LOG_INFO, "[%s:%d]first block buf1<>0\n",__FUNCTION__,__LINE__);
            memset(buf1, 0, CHECK_FULL_ZERO_SIZE);
            avio_seek(ic->pb, offset + CHECK_FULL_ZERO_SIZE, SEEK_SET);
            do {
                ret = avio_read(ic->pb, buf1, CHECK_FULL_ZERO_SIZE);
            } while (ret == AVERROR(EAGAIN));
            if (ret < 0) {
                av_log(NULL, AV_LOG_INFO, "[%s:%d]av_read_packet failed ret=%d\n", __FUNCTION__, __LINE__, ret);
                if (ret == AVERROR_EOF) {
                    av_free(buf1);
                    av_free(buf2);
                    return filesize;
                }
                break;
            }
            if (memcmp(buf1, buf2, CHECK_FULL_ZERO_SIZE) == 0) {
                av_log(NULL, AV_LOG_INFO, "[%s:%d]find valid packet!offset=0x%llx\n", __FUNCTION__, __LINE__, offset);
                av_free(buf1);
                av_free(buf2);
                return offset;
            } else {
                //av_log(NULL, AV_LOG_INFO, "[%s:%d]second block buf1<>0\n",__FUNCTION__,__LINE__);
                start_offset = offset;
                continue;
            }
        }
    }
    av_free(buf1);
    av_free(buf2);
    return -1;
}
static int64_t check_last_blk_valid(AVFormatContext *ic)
{
    unsigned char *buf1;
    unsigned char *buf2;
    int check_size;
    int read_size;
    int64_t filesize, offset;
    int64_t start_offset, end_offset;
    int64_t ret = -1;
    if ((ic->file_size <= 0 || !ic->pb || ic->pb->is_streamed || ic->pb->is_slowmedia) &&
        !strncmp(ic->filename, "rtsp:", strlen("rtsp:"))) {
        return ic->file_size;
    }
    buf1 = av_mallocz(CHECK_FULL_ZERO_SIZE);
    if (!buf1) {
        return AVERROR(ENOMEM);
    }
    buf2 = av_mallocz(CHECK_FULL_ZERO_SIZE);
    if (!buf2) {
        av_free(buf1);
        return AVERROR(ENOMEM);
    }
    filesize = ic->file_size;
    if (filesize < CHECK_FULL_ZERO_SIZE) {
        check_size = filesize >> 3;
    } else {
        check_size = CHECK_FULL_ZERO_SIZE;
    }
    offset = filesize - check_size;
    if (offset < 0) {
        offset = 0;
    }
    avio_seek(ic->pb, offset, SEEK_SET);
    read_size = 0;
    read_size = avio_read(ic->pb, buf1, check_size);
    if (read_size <= 0) {
        av_log(ic, AV_LOG_ERROR, "[%s]get buffer failed, ret=%d\n", __FUNCTION__, read_size);
        ret = 2;
        goto end;
    } else if (memcmp(buf1, buf2, check_size) == 0) { //cmp,buf1=buf2=0
        av_log(ic, AV_LOG_ERROR, "[%s]last block is full ZERO\n", __FUNCTION__);
        ret = 0;
        goto end;
    } else {
        av_log(ic, AV_LOG_ERROR, "[%s]last block is valid data!\n", __FUNCTION__);
        ret = filesize;
    }
end:
    av_free(buf1);
    av_free(buf2);
    av_log(ic, AV_LOG_INFO, "[%s]last valid block is [0x%llx] file_size=0x%llx\n", __FUNCTION__, ret, filesize);
    return ret;
}

static int64_t find_last_chapter_end(AVFormatContext *ic, int64_t old_offset, int64_t start_time, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    int64_t offset;
    int64_t pts = AV_NOPTS_VALUE;
    int read_size, i, ret, retry = 0;
    offset = old_offset;
    do {
        offset -= DURATION_MAX_READ_SIZE << retry;
        if (offset < 0) {
            offset = 0;
            break;
        }
        //av_log(NULL, AV_LOG_INFO, "[%s:%d]offset=0x%llx\n",__FUNCTION__,__LINE__,offset);
        avio_seek(ic->pb, offset, SEEK_SET);
        read_size = 0;
        for (;;) {
            if (read_size >= DURATION_MAX_READ_SIZE << (FFMAX(retry - 1, 0))) {
                break;
            }
            do {
                ret = av_read_packet(ic, pkt);
            } while (ret == AVERROR(EAGAIN));
            if (ret != 0) {
                av_log(NULL, AV_LOG_INFO, "[%s:%d]av_read_packet failed, ret=%d\n", __FUNCTION__, __LINE__, ret);
                break;
            }
            //av_log(NULL, AV_LOG_INFO, "[%s:%d] read a packet, pkt->pts=0x%llx\n",__FUNCTION__, __LINE__,pkt->pts);
            read_size += pkt->size;
            if (pkt->pts != AV_NOPTS_VALUE && pkt->stream_index == stream_index) {
                av_log(NULL, AV_LOG_INFO, "start_time=0x%llx pts=0x%llx\n", start_time, pts);
                if (pkt->pts > start_time && (pkt->pts > pts || pts == AV_NOPTS_VALUE)) {
                    pts = pkt->pts;
                    //av_log(NULL, AV_LOG_INFO, "pts============0x%llx\n",pts);
                } else if (pkt->pts < start_time) {
                    break;
                }
            }
            av_free_packet(pkt);
        }
    } while (pts == AV_NOPTS_VALUE && old_offset > (DURATION_MAX_READ_SIZE << retry) && ++retry < RETRY_CHECK_MAX);
    avio_seek(ic->pb, old_offset, SEEK_SET);
    //av_log(NULL, AV_LOG_INFO, "[%s]return pts=0x%llx\n",__FUNCTION__, pts);
    return pts;
}

static void av_estimate_timeings_sum_chapters(AVFormatContext *ic, int64_t old_offset)
{
    AVPacket pkt1, *pkt = &pkt1;
    AVStream *st;
    int read_size, i, ret;
    int64_t end_time[MAX_STREAMS], start_time[MAX_STREAMS];
    int64_t cur_offset, valid_offset, duration;
    int64_t last_pts[MAX_STREAMS], pts_discontinue[MAX_STREAMS], first_pts[MAX_STREAMS];
    int retry = 0;
#define DISCONTINUE_PTS_VALUE  (0xffffffff)
    ic->cur_st = NULL;
    /* flush packet queue */
    flush_packet_queue(ic);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        last_pts[i] = AV_NOPTS_VALUE;
        pts_discontinue[i] = 0;
        end_time[i]  = AV_NOPTS_VALUE;
        if (st->start_time != AV_NOPTS_VALUE) {
            start_time[i] = st->start_time;
            first_pts[i] = st->start_time;
        } else if (st->first_dts != AV_NOPTS_VALUE) {
            start_time[i] = st->first_dts;
            first_pts[i] = st->first_dts;
        } else {
            av_log(st->codec, AV_LOG_WARNING, "start time is not set in av_estimate_timeings_sum_chapters\n");
        }
        if (st->parser) {
            av_parser_close(st->parser);
            st->parser = NULL;
            av_free_packet(&st->cur_pkt);
        }
    }
    /* estimate the end time (duration) */
    /* XXX: may need to support wrapping */
    cur_offset = url_ftell(ic->pb);
    avio_seek(ic->pb, cur_offset, SEEK_SET);
    valid_offset = ic->valid_offset;
    read_size = 0;
    for (;;) {
        if (read_size >= valid_offset) {
            break;
        }
        do {
            ret = av_read_packet(ic, pkt);
        } while (ret == AVERROR(EAGAIN));
        if (ret != 0) {
            av_log(NULL, AV_LOG_INFO, "[%s:%d]av_read_packet failed, ret=%d\n", __FUNCTION__, __LINE__, ret);
            break;
        }
        read_size += pkt->size;
        if (pkt->pts != AV_NOPTS_VALUE) {
            st = ic->streams[pkt->stream_index];
            if (last_pts[pkt->stream_index] != AV_NOPTS_VALUE &&
                last_pts[pkt->stream_index] < DISCONTINUE_PTS_VALUE &&
                pkt->pts < last_pts[pkt->stream_index]) {
                pts_discontinue[pkt->stream_index] += last_pts[pkt->stream_index] - first_pts[pkt->stream_index];
                first_pts[pkt->stream_index] = pkt->pts;
            }
            if (pkt->pts != last_pts[pkt->stream_index]) {
                last_pts[pkt->stream_index] = pkt->pts;
            }
            end_time[pkt->stream_index] = pkt->pts;
        }
        av_free_packet(pkt);
    }
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        duration = end_time[i] - first_pts[i];
        if (pts_discontinue[i] != AV_NOPTS_VALUE) {
            duration += pts_discontinue[i];
        }
        if (duration < 0) {
            duration += 1LL << st->pts_wrap_bits;
        }
        if (duration > 0) {
            if (st->duration == AV_NOPTS_VALUE ||
                st->duration < duration) {
                st->duration = duration;
            }
        }
    }
    fill_all_stream_timings(ic);
    avio_seek(ic->pb, old_offset, SEEK_SET);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        st->cur_dts = st->first_dts;
        st->last_IP_pts = AV_NOPTS_VALUE;
    }
}
/* only usable for vob */
static void av_estimate_timeings_chapters(AVFormatContext *ic, int64_t old_offset)
{
    AVPacket pkt1, *pkt = &pkt1;
    AVStream *st;
    int read_size, i, ret;
    int64_t end_time, start_time[MAX_STREAMS], max_time[MAX_STREAMS];
    int64_t valid_offset, offset, last_offset, duration;
    int64_t last_pts[MAX_STREAMS], pts_discontinue[MAX_STREAMS];
    int retry = 0;
    unsigned int cur_nb_streams; // number of streams currently found
#define DISCONTINUE_PTS_VALUE  (0xffffffff)
    ic->cur_st = NULL;
    /* flush packet queue */
    flush_packet_queue(ic);
    for (i = 0; i < ic->nb_streams; i++) {
        last_pts[i] = AV_NOPTS_VALUE;
        pts_discontinue[i] = AV_NOPTS_VALUE;
        max_time[i] = 0;
        st = ic->streams[i];
        if (st->start_time != AV_NOPTS_VALUE) {
            start_time[i] = st->start_time;
            max_time[i] = start_time[i];
        } else if (st->first_dts != AV_NOPTS_VALUE) {
            start_time[i] = st->first_dts;
            max_time[i] = start_time[i];
        } else {
            av_log(st->codec, AV_LOG_WARNING, "start time is not set in av_estimate_timeings_chapters\n");
        }
        if (st->parser) {
            av_parser_close(st->parser);
            st->parser = NULL;
            av_free_packet(&st->cur_pkt);
        }
    }
    /* estimate the end time (duration) */
    /* XXX: may need to support wrapping */
    valid_offset = ic->valid_offset;
    end_time = AV_NOPTS_VALUE;
    cur_nb_streams = ic->nb_streams;
    do {
        offset = valid_offset - (DURATION_MAX_READ_SIZE << retry);
        if (offset < 0) {
            offset = 0;
        }
        avio_seek(ic->pb, offset, SEEK_SET);
        read_size = 0;
        for (;;) {
            if (read_size >= DURATION_MAX_READ_SIZE << (FFMAX(retry - 1, 0))) {
                break;
            }
            do {
                ret = av_read_packet(ic, pkt);
            } while (ret == AVERROR(EAGAIN));
            if (ret != 0) {
                av_log(NULL, AV_LOG_INFO, "[%s:%d]av_read_packet failed, ret=%d\n", __FUNCTION__, __LINE__, ret);
                break;
            }
            //av_log(NULL, AV_LOG_INFO, "[%s:%d] read a packet, pkt->pts=0x%llx\n",__FUNCTION__, __LINE__,pkt->pts);
            // For VOB, new streams may be found after reading a new packet.
            // If this happens, we need to init for the new streams.
            if (ic->nb_streams > cur_nb_streams) {
                for (i = (int)cur_nb_streams; i < (int)ic->nb_streams; i++) {
                    last_pts[i] = AV_NOPTS_VALUE;
                    pts_discontinue[i] = AV_NOPTS_VALUE;
                    st = ic->streams[i];
                    if (st->start_time != AV_NOPTS_VALUE) {
                        start_time[i] = st->start_time;
                        max_time[i] = start_time[i];
                    } else if (st->first_dts != AV_NOPTS_VALUE) {
                        start_time[i] = st->first_dts;
                        max_time[i] = start_time[i];
                    } else {
                        start_time[i] = 0;
                        max_time[i] = 0;
                    }
                }
                cur_nb_streams = ic->nb_streams;
            }
            read_size += pkt->size;
            st = ic->streams[pkt->stream_index];
            if (pkt->pts != AV_NOPTS_VALUE) {
                if (last_pts[pkt->stream_index] != AV_NOPTS_VALUE &&
                    last_pts[pkt->stream_index] < DISCONTINUE_PTS_VALUE &&
                    pkt->pts < last_pts[pkt->stream_index]) {
                    pts_discontinue[pkt->stream_index] = last_pts[pkt->stream_index];
                    av_log(NULL, AV_LOG_INFO, "pts=0x%llx discontinue_pts=0x%llx\n", pkt->pts, last_pts[pkt->stream_index]);
                }
                if (pkt->pts != last_pts[pkt->stream_index]) {
                    last_pts[pkt->stream_index] = pkt->pts;
                }
                if (start_time[pkt->stream_index] != AV_NOPTS_VALUE) {
                    if (pkt->pts < start_time[pkt->stream_index] &&
                        pkt->pts < DISCONTINUE_PTS_VALUE &&
                        pts_discontinue[pkt->stream_index] == AV_NOPTS_VALUE) {
                        last_offset = url_ftell(ic->pb);
                        pts_discontinue[pkt->stream_index] = find_last_chapter_end(ic, last_offset, start_time[pkt->stream_index], pkt->stream_index);
                    }
                    if (pkt->pts > max_time[pkt->stream_index]) {
                        max_time[pkt->stream_index] = pkt->pts;
                    }
                    end_time = pkt->pts;
                    duration = end_time - start_time[pkt->stream_index];
                    //av_log(NULL, AV_LOG_INFO, "end=0x%llx start=0x%llx dur=0x%llx\n",end_time, start_time[pkt->stream_index], duration);
                    if (pts_discontinue[pkt->stream_index] != AV_NOPTS_VALUE) {
                        duration = pts_discontinue[pkt->stream_index] - start_time[pkt->stream_index];
                        //av_log(NULL, AV_LOG_INFO, "discontinue_pts=0x%llx duration=0x%llx\n",pts_discontinue[pkt->stream_index], duration);
                    } else if (end_time < start_time[pkt->stream_index]) {
                        duration = max_time[pkt->stream_index] - start_time[pkt->stream_index];
                        av_log(NULL, AV_LOG_INFO, "end time < start time and cannot check discontinue_pts use max pts - start pt\n");
                    }
                    if (duration < 0) {
                        duration += 1LL << st->pts_wrap_bits;
                        av_log(NULL, AV_LOG_INFO, "duration=0x%llx\n", duration);
                    }
                    if (duration > 0) {
                        if (st->duration == AV_NOPTS_VALUE ||
                            st->duration < duration) {
                            st->duration = duration;
                        }
                        //av_log(NULL, AV_LOG_INFO, "st->dur=0x%llx\n",duration);
                    }
                }
            }
            av_free_packet(pkt);
        }
    } while (end_time == AV_NOPTS_VALUE
             && valid_offset > (DURATION_MAX_READ_SIZE << retry)
             && ++retry <= DURATION_MAX_RETRY);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        duration = st->duration;
    }
    fill_all_stream_timings(ic);
    avio_seek(ic->pb, old_offset, SEEK_SET);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        st->cur_dts = st->first_dts;
        st->last_IP_pts = AV_NOPTS_VALUE;
    }
}

/* only usable for MPEG-PS streams */
static void av_estimate_timings_from_pts(AVFormatContext *ic, int64_t old_offset)
{
    AVPacket pkt1, *pkt = &pkt1;
    AVStream *st;
    int read_size, i, ret;
    int64_t end_time;
    int64_t start_time = AV_NOPTS_VALUE;
    int64_t valid_offset, offset, duration;
    int retry = 0;
    int first_dts = 0;
    int64_t estimate_size = DURATION_ESTIMATE_MAX_READ_SIZE;
    char value[PROPERTY_VALUE_MAX] = {0};
    ic->cur_st = NULL;
    //read from prop

    if (ic->pb->local_playback == 1) {
        estimate_size = DURATION_MAX_READ_LOCALPLAY_SIZE;
    }

    if (property_get("media.amplayer.estimate_size", value, NULL) > 0) {
        estimate_size = (int64_t)atoi(value);
    }
    av_log(NULL, AV_LOG_WARNING, "Estimate Size:%d (Byte)\n", estimate_size);
    /* flush packet queue */
    flush_packet_queue(ic);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->first_dts != AV_NOPTS_VALUE) {
            first_dts = st->first_dts;
        }
        if (st->start_time == AV_NOPTS_VALUE && st->first_dts == AV_NOPTS_VALUE) {
            av_log(st->codec, AV_LOG_WARNING, "stream[%d] start time is not set in av_estimate_timings_from_pts\n", i);
        } else {
            if (st->start_time == AV_NOPTS_VALUE) {
                start_time = st->first_dts;
            } else {
                start_time = st->start_time;
            }
        }
        if (st->parser) {
            av_parser_close(st->parser);
            st->parser = NULL;
            av_free_packet(&st->cur_pkt);
        }
    }
    //if stream can not get first dts,use first_dts finded in other stream.
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->first_dts == AV_NOPTS_VALUE) {
            st->first_dts = first_dts;
        }
    }
    /* estimate the end time (duration) */
    /* XXX: may need to support wrapping */
    valid_offset = ic->valid_offset;
    end_time = AV_NOPTS_VALUE;
    do {
        offset = valid_offset - (estimate_size << retry);
        if (offset < 0) {
            offset = 0;
        }
        //av_log(NULL,AV_LOG_INFO, "[%s:%d]offset=%llx valid_offset=%llx \n", __FUNCTION__, __LINE__, offset, valid_offset);
        avio_seek(ic->pb, offset, SEEK_SET);
        read_size = 0;
        for (;;) {
            if (read_size >= estimate_size << (FFMAX(retry - 1, 0))) {
                break;
            }
            do {
                ret = av_read_packet(ic, pkt);
                if (url_interrupt_cb()) {
                    TRACE();
                    break;
                }
            } while (ret == AVERROR(EAGAIN));
            if (ret == AVERROR(ENOSR)) {
                retry = DURATION_MAX_RETRY;
                av_log(NULL, AV_LOG_ERROR, "may be live stream, seek return range error\n");
                break;
            } else if (ret != 0) {
                break;
            }
            //av_log(NULL, AV_LOG_INFO, "[%s:%d] read a [%d]packet, pkt->pts=0x%llx\n",__FUNCTION__, __LINE__,pkt->stream_index,pkt->pts);
            read_size += pkt->size;
            st = ic->streams[pkt->stream_index];
            if (pkt->pts != AV_NOPTS_VALUE &&
                (st->start_time != AV_NOPTS_VALUE ||
                 st->first_dts  != AV_NOPTS_VALUE ||
                 start_time != AV_NOPTS_VALUE)) {
                duration = end_time = pkt->pts;
                //av_log(NULL, AV_LOG_INFO, "[%s:%d] duration=0x%llx start_time=%llx first_dts=%llx\n",__FUNCTION__, __LINE__,duration, st->start_time, st->first_dts);
                if (st->start_time != AV_NOPTS_VALUE) {
                    duration -= st->start_time;
                } else if (st->first_dts  != AV_NOPTS_VALUE) {
                    duration -= st->first_dts;
                } else {
                    duration -= start_time;
                }
                //av_log(NULL, AV_LOG_INFO, "[%s:%d] duration=0x%llx\n",__FUNCTION__, __LINE__,duration);
                if (duration < 0) {
                    duration += 1LL << st->pts_wrap_bits;
                }
                if (duration > 0) {
                    if (st->duration == AV_NOPTS_VALUE || st->info->last_duration <= 0 ||
                        (st->duration < duration && FFABS(duration - st->info->last_duration) < 60LL * st->time_base.den / st->time_base.num)) {
                        st->duration = duration;
                    }
                    if (FFABS(duration - st->info->last_duration) > 600LL * st->time_base.den / st->time_base.num && st->info->last_duration > 0) {
                        //av_log(NULL, AV_LOG_INFO, "[%s:%d] diff too much duration=0x%llx  last_duration:%llx index:%d \n",__FUNCTION__, __LINE__,duration,st->info->last_duration,pkt->stream_index);
                        st->duration = st->info->last_duration;
                        continue;
                    }
                    st->info->last_duration = duration;
                    st->max_pos = offset + read_size;
                    st->max_pts = duration;
                }
            }
            av_free_packet(pkt);
        }
        // av_log(NULL, AV_LOG_INFO, "[%s:%d] endtime=0x%llx retry=%d\n",__FUNCTION__, __LINE__,end_time , retry);
    } while (end_time == AV_NOPTS_VALUE
             && valid_offset > (estimate_size << retry)
             && ++retry <= DURATION_ESTIMATE_MAX_RETRY);
    av_log(NULL, AV_LOG_INFO, "[%s:%d] ic->duration=0x%llx\n", __FUNCTION__, __LINE__, ic->duration);
    fill_all_stream_timings(ic);
    avio_seek(ic->pb, old_offset, SEEK_SET);
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        st->cur_dts = st->first_dts;
        st->last_IP_pts = AV_NOPTS_VALUE;
        st->reference_dts = AV_NOPTS_VALUE;
    }
}

extern  int  adts_bitrate_parse(AVFormatContext *s, int *bitrate, int64_t old_offset);
static int av_estimate_timings(AVFormatContext *ic, int64_t old_offset)
{
    int64_t file_size;
    int64_t cur_offset, valid_offset;
    int64_t ret = 0;
    int bitrate = 0;
    int i = 0;
    AVStream *st = NULL;
    if (!ic->pb) {
        return 0;
    }
    /* get the file size, if possible */
    if (ic->iformat->flags & AVFMT_NOFILE) {
        file_size = 0;
    } else {
        file_size = avio_size(ic->pb);
        if (file_size < 0) {
            file_size = 0;
        }
    }
    ic->file_size = file_size;
    ic->valid_offset = file_size ? file_size : 0x7fffffffffffffff;
#if 1
    if (ic->pb->local_playback == 1) {
        /* find valid_offset*/
        cur_offset = url_ftell(ic->pb);
        ret = check_last_blk_valid(ic);
        if (ret ==  ic->file_size) {
            valid_offset = ic->file_size;
        } else if (ret == 0) {
            valid_offset = seek_last_valid_pkt(ic);
        } else {
            av_log(NULL, AV_LOG_ERROR, "[%s]error, return\n", __FUNCTION__);
            return ret;
        }
        if ((valid_offset > 2) && (ic->valid_offset != 0x7fffffffffffffff)) {
            ic->valid_offset = valid_offset;
            ic->valid_offset_done = 1;
            if (ic->valid_offset + CHECK_FULL_ZERO_SIZE <= ic->file_size) {
                ic->valid_offset += CHECK_FULL_ZERO_SIZE;
            }
        }
        avio_seek(ic->pb, cur_offset, SEEK_SET);
    }
#endif
    av_log(NULL, AV_LOG_INFO, "[%s:%d]file_size=%lld valid_offset=%llx\n", __FUNCTION__, __LINE__, ic->file_size, ic->valid_offset);

    char *tmp = strstr(ic->filename, "duration=");
    float value;
    if (tmp) {
        int use_url_duration = 1;
        if (am_getconfig_float("media.amffmpeg.url_duration", &value) == 0) {
            use_url_duration = (int)value;
        }
        if (use_url_duration == 0) {
            tmp = NULL;
        }
    }
    if (tmp) {
        av_log(NULL, AV_LOG_INFO, "Skip calc duration \n");
        return ret;
    }
    if ((!strcmp(ic->iformat->name, "mpeg") ||
         !strcmp(ic->iformat->name, "mpegts")) &&
        file_size > 0 && ic->pb->seekable /*&& !ic->pb->is_slowmedia*/ && !ic->pb->is_streamed) {
        /* get accurate estimate from the PTSes */
        if (!strcmp(ic->iformat->name, "mpegts")) {
            av_estimate_timings_from_pts(ic, old_offset);
        } else {
            av_estimate_timeings_chapters(ic, old_offset);
        }
    } else if (av_has_duration(ic)) {
        /* at least one component has timings - we use them for all
           the components */
        fill_all_stream_timings(ic);
    } else {
        av_log(ic, AV_LOG_WARNING, "Estimating duration from bitrate, this may be inaccurate\n");
        /* less precise: use bitrate info */
        if (!strcmp(ic->iformat->name, "aac")) {
            ret = adts_bitrate_parse(ic, &bitrate, old_offset);
            if (ret > 0) {
                for (i = 0; i < ic->nb_streams; i++) {
                    st = ic->streams[i];
                    st->codec->bit_rate = bitrate;
                }
                ic->bit_rate = bitrate;
                av_log(NULL, AV_LOG_INFO, "VBR AAC: read all frames to ensure correct bitrate %d \n", ic->bit_rate);
            }
        }
        av_estimate_timings_from_bit_rate(ic);
    }
    av_update_stream_timings(ic);
#if 0
    {
        int i;
        AVStream av_unused *st;
        for (i = 0; i < ic->nb_streams; i++) {
            st = ic->streams[i];
            printf("%d: start_time: %0.3f duration: %0.3f\n",
                   i, (double)st->start_time / AV_TIME_BASE,
                   (double)st->duration / AV_TIME_BASE);
        }
        printf("stream: start_time: %0.3f duration: %0.3f bitrate=%d kb/s\n",
               (double)ic->start_time / AV_TIME_BASE,
               (double)ic->duration / AV_TIME_BASE,
               ic->bit_rate / 1000);
    }
#endif
    return 0;
}

static int has_codec_parameters(AVCodecContext *enc)
{
    int val;
    switch (enc->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        val = enc->sample_rate && enc->channels && enc->sample_fmt != AV_SAMPLE_FMT_NONE;
        if (!enc->frame_size &&
            (enc->codec_id == CODEC_ID_VORBIS ||
             enc->codec_id == CODEC_ID_AAC ||
             enc->codec_id == CODEC_ID_MP1 ||
             enc->codec_id == CODEC_ID_MP2 ||
             enc->codec_id == CODEC_ID_MP3 ||
             enc->codec_id == CODEC_ID_SPEEX ||
             enc->codec_id == CODEC_ID_CELT)) {
            return 0;
        }
        break;
    case AVMEDIA_TYPE_VIDEO:
        val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
        break;
    default:
        val = 1;
        break;
    }
    return enc->codec_id != CODEC_ID_NONE && val != 0;
}

//fastmode refer to different mode
#define SOFTDEMUX_PARSE_MODE  0
#define PARSE_MODE_BASE 8
#define ASF_PARSE_MODE  8
#define SPEED_PARSE_MODE 9
#define WFD_PARSE_MODE  10
#define FLV_PARSE_MODE  11
#define TS_HASPMT_PLUS  128

static int has_codec_parameters_ex(AVCodecContext *enc, int fastmode)
{
    int val;
    if (!fastmode) {
        switch (enc->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            // bugfix:127506
            // since ac3 decoder has been removed under Lincence Control
            // ac3 format not check sample format parameter
            if (enc->codec_id == CODEC_ID_AC3) {
                val = enc->sample_rate && enc->channels;
            } else {
                val = enc->sample_rate && enc->channels && enc->sample_fmt != AV_SAMPLE_FMT_NONE;
            }
            if (!enc->frame_size &&
                (enc->codec_id == CODEC_ID_VORBIS ||
                 enc->codec_id == CODEC_ID_AAC ||
                 enc->codec_id == CODEC_ID_MP1 ||
                 enc->codec_id == CODEC_ID_MP2 ||
                 enc->codec_id == CODEC_ID_MP3 ||
                 enc->codec_id == CODEC_ID_SPEEX ||
                 enc->codec_id == CODEC_ID_CELT)) {
                return 0;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
            break;
        default:
            val = 1;
            break;
        }
    } else {
        switch (enc->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            if (fastmode &&
                (enc->codec_id == CODEC_ID_AAC) ||
                (enc->codec_id == CODEC_ID_MP3) ||
                (enc->codec_id == CODEC_ID_AC3) ||
                (enc->codec_id == CODEC_ID_DTS)) {
                if (FLV_PARSE_MODE == fastmode) {
                    val = enc->extradata_size;
                } else {
                    val = 1;
                }
            } else {
                val = enc->sample_rate && enc->channels;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            if (fastmode == 8) {
                val = (enc->durcount > 15 ? 1 : 0) && enc->width;
            } else {
                val = enc->width;
            }
            break;
        default:
            val = 1;
            break;
        }
    }
    if (WFD_PARSE_MODE == fastmode) {
        return val != 0;
    } else {
        if (fastmode > (PARSE_MODE_BASE + TS_HASPMT_PLUS)) { // only ts stream have pmt, can use the skip mode. add by le.yang@amlogic.com
            if (enc->codec_type == AVMEDIA_TYPE_DATA ||
                enc->codec_type == AVMEDIA_TYPE_ATTACHMENT) {
                return 1;
            }
        }
        return enc->codec_id != CODEC_ID_NONE && val != 0;
    }
}


static int has_decode_delay_been_guessed(AVStream *st)
{
    /* trying decode 8k would cost huge memory, invoke oom-killer */
    return (st->codec->codec_id != CODEC_ID_H264 && st->codec->codec_id != CODEC_ID_HEVC) ||
           (st->codec_info_nb_frames >= 6 + st->codec->has_b_frames) ||
           (st->codec->width > 4096 || st->codec->height > 2304);
}

static int try_decode_frame(AVStream *st, AVPacket *avpkt)
{
    int16_t *samples;
    AVCodec *codec;
    int got_picture, data_size, ret = 0;
    AVFrame picture;
    // no hevc SW decoder, just parser now
    if (st->codec->codec_id == CODEC_ID_HEVC && (st->codec->width <= 0 || st->codec->bit_depth <= 0)) {
        struct hevc_info info;
        const uint8_t *ptr;
        uint8_t *buffer = NULL;
        int tsize, bsize = 0;
        // no sps in es data but extradata, parse bit_depth from extradata.
        if (st->codec->extradata_size > 0) {
            int header_len = 0;
            int dsize = st->codec->extradata_size;
            bsize = st->codec->extradata_size + 100;
            buffer = (uint8_t *)av_malloc(bsize);
            memset(buffer, 0x00, bsize);
            const uint8_t *p = st->codec->extradata;
            if ((p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) || (p[0] == 0 && p[1] == 0 && p[2] == 1)) {
                memcpy(buffer, st->codec->extradata, st->codec->extradata_size);
            } else {
                char nal_start_code[] = {0x0, 0x0, 0x0, 0x1};
                int i, j, num_arrays, nal_len_size, nalsize;
                if (dsize < 23) {
                    //av_log(NULL, AV_LOG_INFO, "size too short\n");
                    av_free(buffer);
                    return 0;
                }
                p += 21;
                nal_len_size = *(p++) & 3 + 1;
                num_arrays   = *(p++);
                dsize -= 23;
                for (i = 0; i < num_arrays; i++)
                {
                    if (dsize < 3) {
                        //av_log(NULL, AV_LOG_INFO, "size too short\n");
                        av_free(buffer);
                        return 0;
                    }
                    int type = *(p++) & 0x3f;
                    int cnt  = (*p << 8) | (*(p + 1));
                    p += 2;
                    dsize -= 3;
                    for (j = 0; j < cnt; j++)
                    {
                        nalsize = (*p << 8) | (*(p + 1));
                        memcpy(&(buffer[header_len]), nal_start_code, 4);
                        header_len += 4;
                        if (dsize < nalsize) {
                            //av_log(NULL, AV_LOG_INFO, "size too short\n");
                            av_free(buffer);
                            return 0;
                        }
                        memcpy(&(buffer[header_len]), p + 2, nalsize);
                        header_len += nalsize;
                        p += (nalsize + 2);
                        dsize -= nalsize;
                    }
                }
            }
            ptr = buffer;
            tsize = header_len;
        }
        ret = HEVC_decode_SPS(ptr, tsize, &info);
        if (ret) {
            ptr = avpkt->data;
            tsize = avpkt->size;
            ret = HEVC_decode_SPS(ptr, tsize, &info);
        }
        if (!ret) {
            st->codec->width = info.mwidth;
            st->codec->height = info.mheight;
            st->codec->bit_depth = info.bit_depth;
            if (info.long_term_ref_pics_present_flag == 1 && info.num_long_term_ref_pics_sps > 0) {
                st->codec->long_term_ref_pic = 1;
            }
        }
        if (bsize > 0) {
            av_free(buffer);
        }
        return 0;
    }
    if (!st->codec->codec) {
        codec = avcodec_find_decoder(st->codec->codec_id);
        if (!codec) {
            return -1;
        }
        ret = avcodec_open(st->codec, codec);
        if (ret < 0) {
            return ret;
        }
    }
    if (!has_codec_parameters(st->codec) || !has_decode_delay_been_guessed(st)) {
        switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            avcodec_get_frame_defaults(&picture);
            ret = avcodec_decode_video2(st->codec, &picture,
                                        &got_picture, avpkt);
            break;
        case AVMEDIA_TYPE_AUDIO:
            data_size = FFMAX(avpkt->size, AVCODEC_MAX_AUDIO_FRAME_SIZE);
            samples = av_malloc(data_size);
            if (!samples) {
                goto fail;
            }
            ret = avcodec_decode_audio3(st->codec, samples,
                                        &data_size, avpkt);
            av_free(samples);
            break;
        default:
            break;
        }
    }
fail:
    return ret;
}

unsigned int ff_codec_get_tag(const AVCodecTag *tags, enum CodecID id)
{
    while (tags->id != CODEC_ID_NONE) {
        if (tags->id == id) {
            return tags->tag;
        }
        tags++;
    }
    return 0;
}

enum CodecID ff_codec_get_id(const AVCodecTag *tags, unsigned int tag)
{
    int i;
    for (i = 0; tags[i].id != CODEC_ID_NONE; i++) {
        if (tag == tags[i].tag) {
            return tags[i].id;
        }
    }
    for (i = 0; tags[i].id != CODEC_ID_NONE; i++) {
        if (ff_toupper4(tag) == ff_toupper4(tags[i].tag)) {
            return tags[i].id;
        }
    }
    return CODEC_ID_NONE;
}

unsigned int av_codec_get_tag(const AVCodecTag *const *tags, enum CodecID id)
{
    int i;
    for (i = 0; tags && tags[i]; i++) {
        int tag = ff_codec_get_tag(tags[i], id);
        if (tag) {
            return tag;
        }
    }
    return 0;
}

enum CodecID av_codec_get_id(const AVCodecTag *const *tags, unsigned int tag)
{
    int i;
    for (i = 0; tags && tags[i]; i++) {
        enum CodecID id = ff_codec_get_id(tags[i], tag);
        if (id != CODEC_ID_NONE) {
            return id;
        }
    }
    return CODEC_ID_NONE;
}

static void compute_chapters_end(AVFormatContext *s)
{
    unsigned int i, j;
    int64_t max_time = s->duration + ((s->start_time == AV_NOPTS_VALUE) ? 0 : s->start_time);
    for (i = 0; i < s->nb_chapters; i++)
        if (s->chapters[i]->end == AV_NOPTS_VALUE) {
            AVChapter *ch = s->chapters[i];
            int64_t   end = max_time ? av_rescale_q(max_time, AV_TIME_BASE_Q, ch->time_base)
                            : INT64_MAX;
            for (j = 0; j < s->nb_chapters; j++) {
                AVChapter *ch1 = s->chapters[j];
                int64_t next_start = av_rescale_q(ch1->start, ch1->time_base, ch->time_base);
                if (j != i && next_start > ch->start && next_start < end) {
                    end = next_start;
                }
            }
            ch->end = (end == INT64_MAX) ? ch->start : end;
        }
}

static int get_std_framerate(int i)
{
    if (i < 60 * 12) {
        return i * 1001;
    } else        return ((const int[]) {
        24, 30, 60, 12, 15
    })[i - 60 * 12] * 1000 * 12;
}

/*
 * Is the time base unreliable.
 * This is a heuristic to balance between quick acceptance of the values in
 * the headers vs. some extra checks.
 * Old DivX and Xvid often have nonsense timebases like 1fps or 2fps.
 * MPEG-2 commonly misuses field repeat flags to store different framerates.
 * And there are "variable" fps files this needs to detect as well.
 */
static int tb_unreliable(AVCodecContext *c)
{
    if (c->time_base.den >= 101L * c->time_base.num
        || c->time_base.den <    5L * c->time_base.num
        /*       || c->codec_tag == AV_RL32("DIVX")
               || c->codec_tag == AV_RL32("XVID")*/
        || c->codec_id == CODEC_ID_MPEG2VIDEO
        || c->codec_id == CODEC_ID_H264
       ) {
        return 1;
    }
    return 0;
}

#define SPEED_PARSE_MODE 2
#define ASF_PARSE_MODE  8
#define PRO_BUFFER_SIZE 102400

int av_find_stream_info(AVFormatContext *ic)
{
    int i, count, ret, read_size, j, k;
    AVStream *st;
    AVPacket pkt1, *pkt;
    int64_t file_size = 0;
    int bit_rate = 0;
    int64_t old_offset = -1;
    int fast_switch = 1;/*fast for mpegts.*/
    int dts_count = 0;
    float value;
    int64_t streamtype = -1;
    int retry_number;
    retry_number = am_getconfig_float_def("media.libplayer.maxproberetry", 20);
#define HAVE_INFO_LOCAL_MAX_WAIT_PROBE_TIME_US (8*1000*1000)
#define HAVE_INFO_NET_MAX_WAIT_PROBE_TIME_US (30*1000*1000) /**/
#define DEFAULT_PROBE_SIZE 6500000
    int64_t waittimeout_us = av_gettime();
    int local_playback = 0;
    if (ic->pb && ic->pb->local_playback) {
        waittimeout_us += HAVE_INFO_LOCAL_MAX_WAIT_PROBE_TIME_US;
        ic->probesize = 12000000;//local play can probe bigger size
        local_playback = 1 ;
    } else {
        waittimeout_us += HAVE_INFO_NET_MAX_WAIT_PROBE_TIME_US;
    }
    if (am_getconfig_float("media.libplayer.fastswitch", &value) == 0) {
        fast_switch = (int)value;
    }
    if (fast_switch) {
        if (strcmp(ic->iformat->name, "mpegts")
            && strcmp(ic->iformat->name, "rtsp")
            && !ic->is_dash_demuxer
            && strcmp(ic->iformat->name, "dash")
            && strcmp(ic->iformat->name, "mhls")
            && strcmp(ic->iformat->name, "rtp")) {
            /*not ts. always do full parser.*/
            fast_switch = 0;
        } else if (ic->pb && ic->pb->local_playback) {
            fast_switch = 0;/*disable fast switch for local playing.*/
        } else {
            /*others do with fastswitch*/
        }

        if (ic->is_hls_demuxer == 1 && !strcmp(ic->iformat->name, "mpegts")) {
            fast_switch = 2;
        }

    };

    if (!strcmp(ic->iformat->name, "flv") && ic->pb && (1 == ic->pb->local_playback)) {
        fast_switch = 0;
    }

    if (!strcmp(ic->iformat->name, "rtp")) {
        ic->probesize = 1024 * 256;
    }

    av_log(NULL, AV_LOG_INFO, "retry_number = %d\n", retry_number);
    av_log(NULL, AV_LOG_INFO, "[%s]iformat->name[%s]fast_switch=%d streamtype=%lld\n", \
           __FUNCTION__, ic->iformat->name, fast_switch, streamtype);
    /* make all the stream  valid at the beginning*/
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        st->stream_valid = 1;
    }
    if (ic->pb != NULL) {
        old_offset = avio_tell(ic->pb);
    }
    if (!strcmp(ic->iformat->name, "DRMdemux") || !strcmp(ic->iformat->name, "Demux_no_prot")) {
        for (i = 0; i < ic->nb_streams; i++) {
            st = ic->streams[i];
            if (st->codec->bit_rate > 0) {
                bit_rate += st->codec->bit_rate;
                if (st->duration != AV_NOPTS_VALUE) {
                    file_size += (st->duration / AV_TIME_BASE * st->codec->bit_rate) >> 3;
                }
            }
        }
        if (file_size > 0) {
            ic->file_size = file_size;
        }
        if (bit_rate > 0) {
            ic->bit_rate = bit_rate;
        }
        av_log(NULL, AV_LOG_INFO, "[av_find_stream_info]DRMdemux&Demux_no_prot, do not check stream info ,return directly\n");
        return 0;
    }
    av_log(NULL, AV_LOG_INFO, "[%s:%d]fast_switch=%d, seekkeyframe=%x,\n", __FUNCTION__, __LINE__, fast_switch, (am_getconfig_bool("media.amplayer.seekkeyframe")));
    for (i = 0; i < ic->nb_streams; i++) {
        AVCodec *codec;
        st = ic->streams[i];
        /* if (st->codec->codec_id == CODEC_ID_AAC) {

             st->codec->sample_rate = 0;
             st->codec->frame_size = 0;
             st->codec->channels = 0;
         }*/
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO ||
            st->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            /*            if(!st->time_base.num)
                            st->time_base= */
            if (!st->codec->time_base.num) {
                st->codec->time_base = st->time_base;
            }
        }
        //only for the split stuff
        if (!st->parser && !(ic->flags & AVFMT_FLAG_NOPARSE)) {
            st->parser = av_parser_init(st->codec->codec_id);
            if (st->need_parsing == AVSTREAM_PARSE_HEADERS && st->parser) {
                st->parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
            }
        }
        assert(!st->codec->codec);
        codec = avcodec_find_decoder(st->codec->codec_id);
        /* Force decoding of at least one frame of codec data
         * this makes sure the codec initializes the channel configuration
         * and does not trust the values from the container.
         */
        if (codec && codec->capabilities & CODEC_CAP_CHANNEL_CONF) {
            st->codec->channels = 0;
        }
        /* Ensure that subtitle_header is properly set. */
        if ((st->codec->codec_type == AVMEDIA_TYPE_SUBTITLE ||
             st->codec->codec_id == CODEC_ID_VC1)/*VC1 need codec init for parser.*/
            && codec && !st->codec->codec) {
            avcodec_open(st->codec, codec);
        }
        //try to just open decoders, in case this is enough to get parameters
        //only when  get enough data then opencodec
        //for fast start.
        if (!has_codec_parameters_ex(st->codec, fast_switch)) {
            if (codec && !st->codec->codec) {
                avcodec_open(st->codec, codec);
            }
        }
    }
    for (i = 0; i < ic->nb_streams; i++) {
        ic->streams[i]->info->last_dts = AV_NOPTS_VALUE;
        ic->streams[i]->info->fps_first_dts = AV_NOPTS_VALUE;
        ic->streams[i]->info->fps_last_dts  = AV_NOPTS_VALUE;
    }
    count = 0;
    read_size = 0;
    int stream_parser_count = 0;
    int continue_parse_count = 0;
    for (;;) {
        if (url_interrupt_cb()) {
            ret = AVERROR_EXIT;
            av_log(ic, AV_LOG_DEBUG, "interrupted\n");
            break;
        }
        //if(ic->pb->is_streamed&&!strcmp(ic->iformat->name, "mpegts")&&stream_parser_count ==ic->nb_streams){
        //    av_log(NULL,AV_LOG_WARNING,"Do fast parser.\n");
        //    break;
        // }
        //bug on some mpegts file..,netflix
        /* check if one codec still needs to be handled */
        for (i = 0; i < ic->nb_streams; i++) {
            int fps_analyze_framecount = 20;
            st = ic->streams[i];
            int parse_mode = fast_switch;
            if (ic->pb && ic->pb->is_streamed == 1 && !strcmp(ic->iformat->name, "mpegts")) {
                if (ic->iformat != NULL && (ic->iformat->flags & AVFMT_TS_HASPMT)) {
                    parse_mode = PARSE_MODE_BASE + TS_HASPMT_PLUS + fast_switch;
                } else {
                    parse_mode = PARSE_MODE_BASE + fast_switch;
                }
            }
            if (!strcmp(ic->iformat->name, "asf")) {
                parse_mode = ASF_PARSE_MODE ;
                av_log(NULL, AV_LOG_INFO, "parse_mode=%d\n", parse_mode);
            }
            if ((parse_mode == WFD_PARSE_MODE) && (st->request_probe >= 0)) {
                parse_mode = SPEED_PARSE_MODE;
            }
            //av_log(NULL, AV_LOG_INFO, "parse_mode=%d\n",parse_mode);
            st->codec->durcount = st->info->duration_count;
            av_log(ic, AV_LOG_DEBUG, "[%s:%d]on pos=%lld,stream.index=%d\n", __FUNCTION__, __LINE__, avio_tell(ic->pb), i);
            if (!has_codec_parameters_ex(st->codec, parse_mode) && st->codec->codec_id != 0) {
                break;
            } else {
                stream_parser_count = i + 1;
            }
            av_log(ic, AV_LOG_DEBUG, "[%s:%d]on pos=%lld,stream.index=%d\n", __FUNCTION__, __LINE__, avio_tell(ic->pb), i);
            if (ic->pb && ic->pb->fastdetectedinfo) {
                continue;
            }
            /* if the timebase is coarse (like the usual millisecond precision
               of mkv), we need to analyze more frames to reliably arrive at
               the correct fps */
            if (av_q2d(st->time_base) > 0.0005) {
                fps_analyze_framecount *= 2;
            }
            if (ic->fps_probe_size >= 0) {
                fps_analyze_framecount = ic->fps_probe_size;
            }
            /* variable fps and no guess at the real fps */
            if (!fast_switch && tb_unreliable(st->codec) && !(st->r_frame_rate.num && st->avg_frame_rate.num)
                && st->info->duration_count < fps_analyze_framecount
                && st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                break;
            }
            av_log(ic, AV_LOG_DEBUG, "[%s:%d]on pos=%lld,stream.index=%d\n", __FUNCTION__, __LINE__, avio_tell(ic->pb), i);
            if (strcmp(ic->iformat->name, "mpegts") && st->parser && st->parser->parser->split && !st->codec->extradata) { //not ts.ts needn't extradata.
                break;
            }
            av_log(ic, AV_LOG_DEBUG, "[%s:%d]on pos=%lld,stream.index=%d\n", __FUNCTION__, __LINE__, avio_tell(ic->pb), i);
            if (!fast_switch &&  st->first_dts == AV_NOPTS_VALUE && (st->codec->codec_type == CODEC_TYPE_AUDIO || st->codec->codec_type == CODEC_TYPE_VIDEO)) {
                break;
            }
            av_log(ic, AV_LOG_DEBUG, "[%s:%d]on pos=%lld,stream.index=%d\n", __FUNCTION__, __LINE__, avio_tell(ic->pb), i);
        }
        int need_continue_parse = 0;
        // prevent parsing wrong codec type.
        for (int j = 0; j < ic->nb_streams; j++) {
            st = ic->streams[j];
            if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO || st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                need_continue_parse++;
            }
        }
        continue_parse_count++;
        if (i == ic->nb_streams) {
            //local probe stop feature, probe size > 65m av found stop
            int probe_stop = 0;
            if ((ic->pb && ic->pb->local_playback && (read_size > DEFAULT_PROBE_SIZE))) {
                for (j = 0; j < ic->nb_streams; j++) {
                    st = ic->streams[j];
                    if (((ic->nb_streams == 1) && (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)) ||
                        (need_continue_parse >= 2)) {
                        av_log(NULL, AV_LOG_INFO, "probe_stop 1, find stream info end\n");
                        probe_stop = 1;
                    }
                }
            }
            /* NOTE: if the format has no header, then we need to read
               some packets to get most of the streams, so we cannot
               stop here */
            if (!(ic->ctx_flags & AVFMTCTX_NOHEADER) || probe_stop ||
                (((fast_switch && ic->nb_streams >= 2) || ((2 & fast_switch) && 1 == ic->nb_streams))
                 && ((need_continue_parse >= 2 || continue_parse_count > 10) || (ic->is_hls_demuxer == 1)))) {
                /*
                for fast_mode
                if not found a video & audio ,need_continue_parse
                try more continue_parse_count,
                */
                /* if we found the info for all the codecs, we can stop */
                ret = count;
                av_log(ic, AV_LOG_INFO, "All info found at pos=%lld\n", avio_tell(ic->pb));
                break;
            }
            if (ic->nb_streams >= 2 && (av_gettime() > waittimeout_us)) {
                ret = count;
                av_log(ic, AV_LOG_INFO, "All info found when time out at pos=%lld\n", avio_tell(ic->pb));
                break;
            }
        }
        /* we did not get all the codec info, but we read too much data */
        if (read_size >= ic->probesize) {

            if (!strcmp(ic->iformat->name, "rtp")) {
                ret = count;
                av_log(ic, AV_LOG_ERROR, "Probe buffer size limit %d reached, fast_switch:%d,read_size:%d,stream %d\n",
                       fast_switch, ic->probesize, read_size, ic->nb_streams);
                break;
            }

            // Online case, Since probsize was set to 10240, may not parse enough param
            // add condition check
            // -1 : initial status , track may not exist
            //  0 : track exists but no param found
            //  1 : at least one track param found
            int found_one_audio_param = -1;
            int found_one_video_param = -1;
            ret = count;
            //lagre than probesize net use fast switch get parameters
            if (!local_playback) {
                fast_switch  |= 1;
            }

            av_log(ic, AV_LOG_ERROR, "Probe buffer size limit %d reached, fast_switch:%d,read_size:%d,stream %d\n",
                   fast_switch, ic->probesize, read_size, ic->nb_streams);
            /*try to ensure an audio&video stream par found*/
            for (k = 0; k < ic->nb_streams; k++) {
                st = ic->streams[k];
                if (has_codec_parameters_ex(st->codec, fast_switch)) {
                    if ((st->codec->codec_type == AVMEDIA_TYPE_AUDIO)) {
                        found_one_audio_param = 1;
                    }
                    if ((st->codec->codec_type == AVMEDIA_TYPE_VIDEO)) {
                        found_one_video_param = 1;
                    }

                } else {
                    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO && found_one_audio_param != 1) {
                        found_one_audio_param = 0;
                    }
                    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO && found_one_video_param != 1) {
                        found_one_video_param = 0;
                    }
                }

            }

            if (found_one_audio_param == 0 || found_one_video_param == 0) {
                int default_max_size = am_getconfig_int_def("ffmpeg.online.maxprobesize", DEFAULT_PROBE_SIZE);
                int max_size = default_max_size;
                if (ic->file_size > 0 && ic->file_size < ic->probesize) {
                    break;
                }
                av_log(NULL, AV_LOG_INFO, "No param found. promote probesize. probesize:%u \n", ic->probesize);
                if (ic->pb && ic->pb->is_slowmedia && ic->probesize < max_size - PRO_BUFFER_SIZE) {
                    ic->probesize = read_size + PRO_BUFFER_SIZE;
                }
            }

            /*probe size no change break;*/
            if (read_size >= ic->probesize) {
                break;
            }
        }
        /* NOTE: a new stream can be added there if no header in file
           (AVFMTCTX_NOHEADER) */
        ret = av_read_frame_internal(ic, &pkt1);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            /* EOF or error */
            ret = -1; /* we could not have all the codec parameters before EOF */
            for (i = 0; i < ic->nb_streams; i++) {
                st = ic->streams[i];
                if (!has_codec_parameters_ex(st->codec, fast_switch)) {
                    char buf[256];
                    avcodec_string(buf, sizeof(buf), st->codec, 0);
                    av_log(ic, AV_LOG_WARNING, "Could not find codec parameters (%s)\n", buf);
                } else {
                    ret = 0;
                }
            }
            break;
        }
        if (ret == AVERROR(EAGAIN)) {
            continue;
        }
        pkt = add_to_pktbuf(&ic->packet_buffer, &pkt1, &ic->packet_buffer_end);
        if ((ret = av_dup_packet(pkt)) < 0) {
            goto find_stream_info_err;
        }
        read_size += pkt->size;
        st = ic->streams[pkt->stream_index];

        if (!strcmp(ic->iformat->name, "rtp") && st->codec_info_nb_frames > 0) {
            continue;
        }

        if (st->codec_info_nb_frames > 1) {
            int64_t t;
            if (st->time_base.den > 0 && (t = av_rescale_q(st->info->codec_info_duration, st->time_base, AV_TIME_BASE_Q)) >= ic->max_analyze_duration) {
                av_log(ic, AV_LOG_WARNING, "max_analyze_duration %d reached at %"PRId64"\n", ic->max_analyze_duration, t);
                break;
            }
            st->info->codec_info_duration += pkt->duration;
        }
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) { // just video
            if (pkt->dts == AV_NOPTS_VALUE) { // for some invalid dts case
                dts_count++;
            } else {
                int64_t last = st->info->last_dts;
                int64_t duration = pkt->dts - last;
                if (pkt->dts != AV_NOPTS_VALUE && last != AV_NOPTS_VALUE && duration > 0) {
                    double dur = duration * av_q2d(st->time_base);
                    //                if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                    //                    av_log(NULL, AV_LOG_ERROR, "%f\n", dur);
                    if (st->info->duration_count < 2) {
                        memset(st->info->duration_error, 0, sizeof(st->info->duration_error));
                    }
                    for (i = 1; i < FF_ARRAY_ELEMS(st->info->duration_error); i++) {
                        int framerate = get_std_framerate(i);
                        int ticks = lrintf(dur * framerate / (1001 * 12));
                        double error = dur - ticks * 1001 * 12 / (double)framerate;
                        st->info->duration_error[i] += error * error;
                    }
                    duration /= ++dts_count;
                    st->info->duration_count += dts_count;
                    // ignore the first 4 values, they might have some random jitter
                    if (st->info->duration_count > 3) {
                        st->info->duration_gcd = av_gcd(st->info->duration_gcd, duration);
                    }
                }
                st->info->last_dts = pkt->dts;
                dts_count = 0;
            }
        }

        if ((pkt->dts != AV_NOPTS_VALUE) &&
            (st->codec_info_nb_frames == 0) &&
            (ic->pb != NULL) &&
            !ic->pb->is_slowmedia) {
            // avoid always using the 3rd packet's pts for st->start_time in following logic
            // set the first dts value for discontinue checking
            st->info->fps_first_dts = pkt->dts;
        }

        if (pkt->dts != AV_NOPTS_VALUE && st->codec_info_nb_frames > 1 && ic->pb != NULL && !ic->pb->is_slowmedia) {
            /* check for non-increasing dts */
            if (st->info->fps_last_dts != AV_NOPTS_VALUE &&
                st->info->fps_last_dts >= pkt->dts) {
                av_log(ic, AV_LOG_DEBUG,
                       "Non-increasing DTS in stream %d: packet %d with DTS "
                       "%"PRId64", packet %d with DTS %"PRId64"\n",
                       st->index, st->info->fps_last_dts_idx,
                       st->info->fps_last_dts, st->codec_info_nb_frames,
                       pkt->dts);
                st->info->fps_first_dts =
                    st->info->fps_last_dts  = AV_NOPTS_VALUE;
            }
            /* Check for a discontinuity in dts. If the difference in dts
             * is more than 1000 times the average packet duration in the
             * sequence, we treat it as a discontinuity. */
            if (st->info->fps_last_dts != AV_NOPTS_VALUE &&
                st->info->fps_last_dts_idx > st->info->fps_first_dts_idx &&
                (pkt->dts - st->info->fps_last_dts) / 1000 >
                (st->info->fps_last_dts     - st->info->fps_first_dts) /
                (st->info->fps_last_dts_idx - st->info->fps_first_dts_idx)) {
                av_log(ic, AV_LOG_WARNING,
                       "DTS discontinuity in stream %d: packet %d with DTS "
                       "%"PRId64", packet %d with DTS %"PRId64"\n",
                       st->index, st->info->fps_last_dts_idx,
                       st->info->fps_last_dts, st->codec_info_nb_frames,
                       pkt->dts);
                st->info->fps_first_dts =
                    st->info->fps_last_dts  = AV_NOPTS_VALUE;
            }
            /* update stored dts values */
            if (st->info->fps_first_dts == AV_NOPTS_VALUE) {
                st->info->fps_first_dts     = pkt->dts;
                st->info->fps_first_dts_idx = st->codec_info_nb_frames;
                st->start_time = pkt->pts;
                //st->first_dts = pkt->dts;
            }
            st->info->fps_last_dts     = pkt->dts;
            st->info->fps_last_dts_idx = st->codec_info_nb_frames;
        }
        if (st->parser && st->parser->parser->split && !st->codec->extradata) {
            int i = st->parser->parser->split(st->codec, pkt->data, pkt->size);
            if (i && (i <= pkt->size))
            {
                st->codec->extradata_size = i;
                st->codec->extradata = av_malloc(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
                memcpy(st->codec->extradata, pkt->data, st->codec->extradata_size);
                memset(st->codec->extradata + i, 0, FF_INPUT_BUFFER_PADDING_SIZE);
            }
        }
        /* if still no information, we try to open the codec and to
           decompress the frame. We try to avoid that in most cases as
           it takes longer and uses more memory. For MPEG-4, we need to
           decompress for QuickTime. */
        if ((st->codec->codec_id != CODEC_ID_CAVS) && (!has_codec_parameters_ex(st->codec, fast_switch) || (!fast_switch && !has_decode_delay_been_guessed(st) && !has_codec_parameters_ex(st->codec, 0)))) {
            try_decode_frame(st, pkt);
        }
        st->codec_info_nb_frames++;
        count++;
    }
    // close codecs which were opened in try_decode_frame()
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->codec->codec) {
            avcodec_close(st->codec);
        }
        st->stream_valid = 1;
        av_log(NULL, AV_LOG_INFO, "[%s:%d] st %d, para: %d, codec_info_nb_frames: %d,\n", __FUNCTION__, __LINE__, i, has_codec_parameters_ex(st->codec, fast_switch)
               , st->codec_info_nb_frames);
        if (!has_codec_parameters_ex(st->codec, fast_switch) && (!st->codec_info_nb_frames)
            && (st->codec->codec_type == AVMEDIA_TYPE_AUDIO)) {
            if (st->codec->codec_id != CODEC_ID_DTS && st->codec->codec_id != CODEC_ID_DRA) {
                av_log(NULL, AV_LOG_INFO, "[%s:%d] stream %d is unvalid! \n", __FUNCTION__, __LINE__, i);
                st->stream_valid = 0;
                if (!strcmp(ic->iformat->name, "rtp")) {
                    av_log(NULL, AV_LOG_INFO, "[%s:%d] rtp %d is valid! \n", __FUNCTION__, __LINE__, i);
                    st->stream_valid = 1;
                }
            }
        }
        if (st->codec_info_nb_frames == 0 && !strcmp(ic->iformat->name, "rtp") && st->codec->codec_type == AVMEDIA_TYPE_DATA) {
            st->codec->codec_id = CODEC_ID_MP2;
            st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
            av_log(NULL, AV_LOG_INFO, "[%s:%d] force use mp2 as default audio\n", __FUNCTION__, __LINE__);
        }
    }
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->codec_info_nb_frames > 2 && !st->avg_frame_rate.num && st->info->codec_info_duration)
            av_reduce(&st->avg_frame_rate.num, &st->avg_frame_rate.den,
                      (st->codec_info_nb_frames - 2) * (int64_t)st->time_base.den,
                      st->info->codec_info_duration * (int64_t)st->time_base.num, 60000);
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (st->codec->codec_id == CODEC_ID_RAWVIDEO && !st->codec->codec_tag && !st->codec->bits_per_coded_sample) {
                uint32_t tag = avcodec_pix_fmt_to_codec_tag(st->codec->pix_fmt);
                if (ff_find_pix_fmt(ff_raw_pix_fmt_tags, tag) == st->codec->pix_fmt) {
                    st->codec->codec_tag = tag;
                }
            }
            // the check for tb_unreliable() is not completely correct, since this is not about handling
            // a unreliable/inexact time base, but a time base that is finer than necessary, as e.g.
            // ipmovie.c produces.
            if (tb_unreliable(st->codec) && st->info->duration_count > 15 && st->info->duration_gcd > FFMAX(1, st->time_base.den / (500LL * st->time_base.num)) && !st->r_frame_rate.num) {
                av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den, st->time_base.den, st->time_base.num * st->info->duration_gcd, INT_MAX);
            }
            if (st->info->duration_count && !st->r_frame_rate.num
                && tb_unreliable(st->codec) /*&&
               //FIXME we should not special-case MPEG-2, but this needs testing with non-MPEG-2 ...
               st->time_base.num*duration_sum[i]/st->info->duration_count*101LL > st->time_base.den*/) {
                int num = 0;
                double best_error = 2 * av_q2d(st->time_base);
                best_error = best_error * best_error * st->info->duration_count * 1000 * 12 * 30;
                for (j = 1; j < FF_ARRAY_ELEMS(st->info->duration_error); j++) {
                    double error = st->info->duration_error[j] * get_std_framerate(j);
                    //                    if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                    //                        av_log(NULL, AV_LOG_ERROR, "%f %f\n", get_std_framerate(j) / 12.0/1001, error);
                    if (error < best_error) {
                        best_error = error;
                        num = get_std_framerate(j);
                    }
                }
                // do not increase frame rate by more than 1 % in order to match a standard rate.
                if (num && (!st->r_frame_rate.num || (double)num / (12 * 1001) < 1.01 * av_q2d(st->r_frame_rate))) {
                    av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den, num, 12 * 1001, INT_MAX);
                }
            }
            if (!st->r_frame_rate.num) {
                if (st->codec->time_base.den * (int64_t)st->time_base.num
                    <= st->codec->time_base.num * st->codec->ticks_per_frame * (int64_t)st->time_base.den) {
                    st->r_frame_rate.num = st->codec->time_base.den;
                    st->r_frame_rate.den = st->codec->time_base.num * st->codec->ticks_per_frame;
                } else {
                    st->r_frame_rate.num = st->time_base.den;
                    st->r_frame_rate.den = st->time_base.num;
                }
            }
        } else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (!st->codec->bits_per_coded_sample) {
                st->codec->bits_per_coded_sample = av_get_bits_per_sample(st->codec->codec_id);
            }
            // set stream disposition based on audio service type
            switch (st->codec->audio_service_type) {
            case AV_AUDIO_SERVICE_TYPE_EFFECTS:
                st->disposition = AV_DISPOSITION_CLEAN_EFFECTS;
                break;
            case AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED:
                st->disposition = AV_DISPOSITION_VISUAL_IMPAIRED;
                break;
            case AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED:
                st->disposition = AV_DISPOSITION_HEARING_IMPAIRED;
                break;
            case AV_AUDIO_SERVICE_TYPE_COMMENTARY:
                st->disposition = AV_DISPOSITION_COMMENT;
                break;
            case AV_AUDIO_SERVICE_TYPE_KARAOKE:
                st->disposition = AV_DISPOSITION_KARAOKE;
                break;
            }
        }
    }

    ret = av_estimate_timings(ic, old_offset);
    if (ret < 0) {
        goto find_stream_info_err;
    }
    compute_chapters_end(ic);
#if 0
    /* correct DTS for B-frame streams with no timestamps */
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (b - frames) {
                ppktl = &ic->packet_buffer;
                while (ppkt1) {
                    if (ppkt1->stream_index != i) {
                        continue;
                    }
                    if (ppkt1->pkt->dts < 0) {
                        break;
                    }
                    if (ppkt1->pkt->pts != AV_NOPTS_VALUE) {
                        break;
                    }
                    ppkt1->pkt->dts -= delta;
                    ppkt1 = ppkt1->next;
                }
                if (ppkt1) {
                    continue;
                }
                st->cur_dts -= delta;
            }
        }
    }
#endif
    av_log(ic, AV_LOG_INFO, "FindStream info end at pos=%lld\n", avio_tell(ic->pb));
find_stream_info_err:
    for (i = 0; i < ic->nb_streams; i++) {
        av_freep(&ic->streams[i]->info);
    }
    av_log(NULL, AV_LOG_INFO, "[%s]return\n", __FUNCTION__);
    return ret;
}

static AVProgram *find_program_from_stream(AVFormatContext *ic, int s)
{
    int i, j;
    for (i = 0; i < ic->nb_programs; i++)
        for (j = 0; j < ic->programs[i]->nb_stream_indexes; j++)
            if (ic->programs[i]->stream_index[j] == s) {
                return ic->programs[i];
            }
    return NULL;
}

int av_find_best_stream(AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        AVCodec **decoder_ret,
                        int flags)
{
    int i, nb_streams = ic->nb_streams;
    int ret = AVERROR_STREAM_NOT_FOUND, best_count = -1;
    unsigned *program = NULL;
    AVCodec *decoder = NULL, *best_decoder = NULL;
    if (related_stream >= 0 && wanted_stream_nb < 0) {
        AVProgram *p = find_program_from_stream(ic, related_stream);
        if (p) {
            program = p->stream_index;
            nb_streams = p->nb_stream_indexes;
        }
    }
    for (i = 0; i < nb_streams; i++) {
        int real_stream_index = program ? program[i] : i;
        AVStream *st = ic->streams[real_stream_index];
        AVCodecContext *avctx = st->codec;
        if (avctx->codec_type != type) {
            continue;
        }
        if (wanted_stream_nb >= 0 && real_stream_index != wanted_stream_nb) {
            continue;
        }
        if (st->disposition & (AV_DISPOSITION_HEARING_IMPAIRED | AV_DISPOSITION_VISUAL_IMPAIRED)) {
            continue;
        }
        if (decoder_ret) {
            decoder = avcodec_find_decoder(st->codec->codec_id);
            if (!decoder) {
                if (ret < 0) {
                    ret = AVERROR_DECODER_NOT_FOUND;
                }
                continue;
            }
        }
        if (best_count >= st->codec_info_nb_frames) {
            continue;
        }
        best_count = st->codec_info_nb_frames;
        ret = real_stream_index;
        best_decoder = decoder;
        if (program && i == nb_streams - 1 && ret < 0) {
            program = NULL;
            nb_streams = ic->nb_streams;
            i = 0; /* no related stream found, try again with everything */
        }
    }
    if (decoder_ret) {
        *decoder_ret = best_decoder;
    }
    return ret;
}

/*******************************************************/

int av_read_play(AVFormatContext *s)
{
    if (s->iformat->read_play) {
        return s->iformat->read_play(s);
    }
    if (s->pb) {
        return avio_pause(s->pb, 0);
    }
    return AVERROR(ENOSYS);
}

int av_read_pause(AVFormatContext *s)
{
    if (s->iformat->read_pause) {
        return s->iformat->read_pause(s);
    }
    if (s->pb) {
        return avio_pause(s->pb, 1);
    }
    return AVERROR(ENOSYS);
}
int av_set_private_parameter(AVFormatContext * s, int para, int type, int64_t value, int64_t value1)
{
    if (s->iformat->set_parameter) {
        return s->iformat->set_parameter(s, para, type, value, value1);
    }
    return AVERROR(ENOSYS);
}



void av_close_input_stream(AVFormatContext *s)
{
    flush_packet_queue(s);
    if (s->iformat) {
        av_log(NULL, AV_LOG_INFO, "[%s]format=%s\n", __FUNCTION__, s->iformat->name);
    }
    if (s->iformat->read_close) {
        av_log(NULL, AV_LOG_INFO, "format %s read_close\n", s->iformat->name);
        s->iformat->read_close(s);
    }
    avformat_free_context(s);
}
void av_free_stream(AVFormatContext *s, AVStream *st)
{
    if (st->parser) {
        av_parser_close(st->parser);
        av_free_packet(&st->cur_pkt);
    }
    av_dict_free(&st->metadata);
    av_free(st->index_entries);
    av_free(st->codec->extradata);
    av_free(st->codec->extradata1);
    av_free(st->codec->subtitle_header);
    av_free(st->codec);
    av_free(st->priv_data);
    av_free(st->info);
    av_free(st);
    s->nb_streams -= 1;
}

void avformat_free_context(AVFormatContext *s)
{
    int i;
    AVStream *st;
    if (s->cover_data) {
        av_free(s->cover_data);
    }
    av_opt_free(s);
    if (s->iformat && s->iformat->priv_class && s->priv_data) {
        av_opt_free(s->priv_data);
    }
    for (i = 0; i < s->nb_streams; i++) {
        /* free all data in a stream component */
        st = s->streams[i];
        if (st->parser) {
            av_parser_close(st->parser);
            av_free_packet(&st->cur_pkt);
        }
        if (st->codec && st->codec->codec) {
            avcodec_close(st->codec);
        }
        av_dict_free(&st->metadata);
        av_free(st->index_entries);
        av_free(st->codec->extradata);
        av_free(st->codec->extradata1);
        av_free(st->codec->subtitle_header);
        av_free(st->codec);
        av_free(st->priv_data);
        av_free(st->info);
        av_free(st);
    }
    for (i = s->nb_programs - 1; i >= 0; i--) {
        av_dict_free(&s->programs[i]->metadata);
        av_freep(&s->programs[i]->stream_index);
        av_freep(&s->programs[i]);
    }
    av_freep(&s->programs);
    av_freep(&s->priv_data);
    while (s->nb_chapters--) {
        av_dict_free(&s->chapters[s->nb_chapters]->metadata);
        av_free(s->chapters[s->nb_chapters]);
    }
    av_freep(&s->chapters);
    av_dict_free(&s->metadata);
    av_freep(&s->streams);
    av_free(s->headers);
    av_free(s);
}

void av_close_input_file(AVFormatContext *s)
{
    av_log(NULL, AV_LOG_ERROR, "av_close_input_file--%d\n", __LINE__);
    AVIOContext *pb = (s->iformat->flags & AVFMT_NOFILE) || (s->flags & AVFMT_FLAG_CUSTOM_IO) ?
                      NULL : s->pb;
    av_log(NULL, AV_LOG_ERROR, "av_close_input_file--%d\n", __LINE__);
    if (strncmp(s->iformat->name, "mhls", 4) == 0) {
        if (s->pb) {
            av_log(NULL, AV_LOG_INFO, "mhls free s->pb=0x%x\n", s->pb);
            pb = s->pb;
        }
    }
    av_close_input_stream(s);
    if (pb) {
        avio_close(pb);
    }
    tcppool_refresh_link_and_check(1);
}

AVStream *av_new_stream(AVFormatContext *s, int id)
{
    AVStream *st;
    int i;
    AVStream **streams;
    if (s->nb_streams >= INT_MAX / sizeof(*streams)) {
        return NULL;
    }
    streams = av_realloc(s->streams, (s->nb_streams + 1) * sizeof(*streams));
    if (!streams) {
        return NULL;
    }
    s->streams = streams;
    st = av_mallocz(sizeof(AVStream));
    if (!st) {
        return NULL;
    }
    if (!(st->info = av_mallocz(sizeof(*st->info)))) {
        av_free(st);
        return NULL;
    }
    st->codec = avcodec_alloc_context();
    if (s->iformat) {
        /* no default bitrate if decoding */
        st->codec->bit_rate = 0;
    }
    st->index = s->nb_streams;
    st->id = id;
    st->start_time = AV_NOPTS_VALUE;
    st->duration = AV_NOPTS_VALUE;
    /* we set the current DTS to 0 so that formats without any timestamps
       but durations get some timestamps, formats with some unknown
       timestamps have their first few packets buffered and the
       timestamps corrected before they are returned to the user */
    st->cur_dts = 0;
    st->first_dts = AV_NOPTS_VALUE;
    st->probe_packets = MAX_PROBE_PACKETS;
    /* default pts setting is MPEG-like */
    av_set_pts_info(st, 33, 1, 90000);
    st->last_IP_pts = AV_NOPTS_VALUE;
    for (i = 0; i < MAX_REORDER_DELAY + 1; i++) {
        st->pts_buffer[i] = AV_NOPTS_VALUE;
    }
    st->reference_dts = AV_NOPTS_VALUE;
    st->sample_aspect_ratio = (AVRational) {
        0, 1
    };
    /* set max pos - pts default */
    st->max_pos = 0;
    st->max_pts = AV_NOPTS_VALUE;
    st->min_pos = 0;
    st->min_pts = AV_NOPTS_VALUE;
    s->streams[s->nb_streams++] = st;
    return st;
}

AVProgram *av_new_program(AVFormatContext *ac, int id)
{
    AVProgram *program = NULL;
    int i;
    av_dlog(ac, "new_program: id=0x%04x\n", id);
    for (i = 0; i < ac->nb_programs; i++)
        if (ac->programs[i]->id == id) {
            program = ac->programs[i];
        }
    if (!program) {
        program = av_mallocz(sizeof(AVProgram));
        if (!program) {
            return NULL;
        }
        dynarray_add(&ac->programs, &ac->nb_programs, program);
        program->discard = AVDISCARD_NONE;
    }
    program->id = id;
    return program;
}

AVChapter *ff_new_chapter(AVFormatContext *s, int id, AVRational time_base, int64_t start, int64_t end, const char *title)
{
    AVChapter *chapter = NULL;
    int i;
    for (i = 0; i < s->nb_chapters; i++)
        if (s->chapters[i]->id == id) {
            chapter = s->chapters[i];
        }
    if (!chapter) {
        chapter = av_mallocz(sizeof(AVChapter));
        if (!chapter) {
            return NULL;
        }
        dynarray_add(&s->chapters, &s->nb_chapters, chapter);
    }
    av_dict_set(&chapter->metadata, "title", title, 0);
    chapter->id    = id;
    chapter->time_base = time_base;
    chapter->start = start;
    chapter->end   = end;
    return chapter;
}

/************************************************************/
/* output media file */

#if FF_API_FORMAT_PARAMETERS
int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap)
{
    if (s->oformat->priv_data_size > 0) {
        s->priv_data = av_mallocz(s->oformat->priv_data_size);
        if (!s->priv_data) {
            return AVERROR(ENOMEM);
        }
        if (s->oformat->priv_class) {
            *(const AVClass **)s->priv_data = s->oformat->priv_class;
            av_opt_set_defaults(s->priv_data);
        }
    } else {
        s->priv_data = NULL;
    }
    return 0;
}
#endif

int avformat_alloc_output_context2(AVFormatContext **avctx, AVOutputFormat *oformat,
                                   const char *format, const char *filename)
{
    AVFormatContext *s = avformat_alloc_context();
    int ret = 0;
    *avctx = NULL;
    if (!s) {
        goto nomem;
    }
    if (!oformat) {
        if (format) {
            oformat = av_guess_format(format, NULL, NULL);
            if (!oformat) {
                av_log(s, AV_LOG_ERROR, "Requested output format '%s' is not a suitable output format\n", format);
                ret = AVERROR(EINVAL);
                goto error;
            }
        } else {
            oformat = av_guess_format(NULL, filename, NULL);
            if (!oformat) {
                ret = AVERROR(EINVAL);
                av_log(s, AV_LOG_ERROR, "Unable to find a suitable output format for '%s'\n",
                       filename);
                goto error;
            }
        }
    }
    s->oformat = oformat;
    if (s->oformat->priv_data_size > 0) {
        s->priv_data = av_mallocz(s->oformat->priv_data_size);
        if (!s->priv_data) {
            goto nomem;
        }
        if (s->oformat->priv_class) {
            *(const AVClass **)s->priv_data = s->oformat->priv_class;
            av_opt_set_defaults(s->priv_data);
        }
    } else {
        s->priv_data = NULL;
    }
    if (filename) {
        av_strlcpy(s->filename, filename, sizeof(s->filename));
    }
    *avctx = s;
    return 0;
nomem:
    av_log(s, AV_LOG_ERROR, "Out of memory\n");
    ret = AVERROR(ENOMEM);
error:
    avformat_free_context(s);
    return ret;
}

#if FF_API_ALLOC_OUTPUT_CONTEXT
AVFormatContext *avformat_alloc_output_context(const char *format,
        AVOutputFormat *oformat, const char *filename)
{
    AVFormatContext *avctx;
    int ret = avformat_alloc_output_context2(&avctx, oformat, format, filename);
    return ret < 0 ? NULL : avctx;
}
#endif

static int validate_codec_tag(AVFormatContext *s, AVStream *st)
{
    const AVCodecTag *avctag;
    int n;
    enum CodecID id = CODEC_ID_NONE;
    unsigned int tag = 0;
    /**
     * Check that tag + id is in the table
     * If neither is in the table -> OK
     * If tag is in the table with another id -> FAIL
     * If id is in the table with another tag -> FAIL unless strict < normal
     */
    for (n = 0; s->oformat->codec_tag[n]; n++) {
        avctag = s->oformat->codec_tag[n];
        while (avctag->id != CODEC_ID_NONE) {
            if (ff_toupper4(avctag->tag) == ff_toupper4(st->codec->codec_tag)) {
                id = avctag->id;
                if (id == st->codec->codec_id) {
                    return 1;
                }
            }
            if (avctag->id == st->codec->codec_id) {
                tag = avctag->tag;
            }
            avctag++;
        }
    }
    if (id != CODEC_ID_NONE) {
        return 0;
    }
    if (tag && (st->codec->strict_std_compliance >= FF_COMPLIANCE_NORMAL)) {
        return 0;
    }
    return 1;
}

#if FF_API_FORMAT_PARAMETERS
int av_write_header(AVFormatContext *s)
{
    return avformat_write_header(s, NULL);
}
#endif

int avformat_write_header(AVFormatContext *s, AVDictionary **options)
{
    int ret = 0, i;
    AVStream *st;
    AVDictionary *tmp = NULL;
    if (options) {
        av_dict_copy(&tmp, *options, 0);
    }
    if ((ret = av_opt_set_dict(s, &tmp)) < 0) {
        goto fail;
    }
    // some sanity checks
    if (s->nb_streams == 0 && !(s->oformat->flags & AVFMT_NOSTREAMS)) {
        av_log(s, AV_LOG_ERROR, "no streams\n");
        ret = AVERROR(EINVAL);
        goto fail;
    }
    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            if (st->codec->sample_rate <= 0) {
                av_log(s, AV_LOG_ERROR, "sample rate not set\n");
                ret = AVERROR(EINVAL);
                goto fail;
            }
            if (!st->codec->block_align)
                st->codec->block_align = st->codec->channels *
                                         av_get_bits_per_sample(st->codec->codec_id) >> 3;
            break;
        case AVMEDIA_TYPE_VIDEO:
            if (st->codec->time_base.num <= 0 || st->codec->time_base.den <= 0) { //FIXME audio too?
                av_log(s, AV_LOG_ERROR, "time base not set\n");
                ret = AVERROR(EINVAL);
                goto fail;
            }
            if ((st->codec->width <= 0 || st->codec->height <= 0) && !(s->oformat->flags & AVFMT_NODIMENSIONS)) {
                av_log(s, AV_LOG_ERROR, "dimensions not set\n");
                ret = AVERROR(EINVAL);
                goto fail;
            }
            if (av_cmp_q(st->sample_aspect_ratio, st->codec->sample_aspect_ratio)) {
                av_log(s, AV_LOG_ERROR, "Aspect ratio mismatch between encoder and muxer layer\n");
                ret = AVERROR(EINVAL);
                goto fail;
            }
            break;
        }
        if (s->oformat->codec_tag) {
            if (st->codec->codec_tag && st->codec->codec_id == CODEC_ID_RAWVIDEO && av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id) == 0 && !validate_codec_tag(s, st)) {
                //the current rawvideo encoding system ends up setting the wrong codec_tag for avi, we override it here
                st->codec->codec_tag = 0;
            }
            if (st->codec->codec_tag) {
                if (!validate_codec_tag(s, st)) {
                    char tagbuf[32];
                    av_get_codec_tag_string(tagbuf, sizeof(tagbuf), st->codec->codec_tag);
                    av_log(s, AV_LOG_ERROR,
                           "Tag %s/0x%08x incompatible with output codec id '%d'\n",
                           tagbuf, st->codec->codec_tag, st->codec->codec_id);
                    ret = AVERROR_INVALIDDATA;
                    goto fail;
                }
            } else {
                st->codec->codec_tag = av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id);
            }
        }
        if (s->oformat->flags & AVFMT_GLOBALHEADER &&
            !(st->codec->flags & CODEC_FLAG_GLOBAL_HEADER)) {
            av_log(s, AV_LOG_WARNING, "Codec for stream %d does not use global headers but container format requires global headers\n", i);
        }
    }
    if (!s->priv_data && s->oformat->priv_data_size > 0) {
        s->priv_data = av_mallocz(s->oformat->priv_data_size);
        if (!s->priv_data) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        if (s->oformat->priv_class) {
            *(const AVClass **)s->priv_data = s->oformat->priv_class;
            av_opt_set_defaults(s->priv_data);
            if ((ret = av_opt_set_dict(s->priv_data, &tmp)) < 0) {
                goto fail;
            }
        }
    }
    /* set muxer identification string */
    if (s->nb_streams && !(s->streams[0]->codec->flags & CODEC_FLAG_BITEXACT)) {
        av_dict_set(&s->metadata, "encoder", LIBAVFORMAT_IDENT, 0);
    }
    if (s->oformat->write_header) {
        ret = s->oformat->write_header(s);
        if (ret < 0) {
            goto fail;
        }
    }
    /* init PTS generation */
    for (i = 0; i < s->nb_streams; i++) {
        int64_t den = AV_NOPTS_VALUE;
        st = s->streams[i];
        switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            den = (int64_t)st->time_base.num * st->codec->sample_rate;
            break;
        case AVMEDIA_TYPE_VIDEO:
            den = (int64_t)st->time_base.num * st->codec->time_base.den;
            break;
        default:
            break;
        }
        if (den != AV_NOPTS_VALUE) {
            if (den <= 0) {
                ret = AVERROR_INVALIDDATA;
                goto fail;
            }
            av_frac_init(&st->pts, 0, 0, den);
        }
    }
    if (options) {
        av_dict_free(options);
        *options = tmp;
    }
    return 0;
fail:
    av_dict_free(&tmp);
    return ret;
}

//FIXME merge with compute_pkt_fields
static int compute_pkt_fields2(AVFormatContext *s, AVStream *st, AVPacket *pkt)
{
    int delay = FFMAX(st->codec->has_b_frames, !!st->codec->max_b_frames);
    int num, den, frame_size, i;
    av_dlog(s, "compute_pkt_fields2: pts:%"PRId64" dts:%"PRId64" cur_dts:%"PRId64" b:%d size:%d st:%d\n",
            pkt->pts, pkt->dts, st->cur_dts, delay, pkt->size, pkt->stream_index);
    /*    if(pkt->pts == AV_NOPTS_VALUE && pkt->dts == AV_NOPTS_VALUE)
            return AVERROR(EINVAL);*/
    /* duration field */
    if (pkt->duration == 0) {
        compute_frame_duration(&num, &den, st, NULL, pkt);
        if (den && num) {
            pkt->duration = av_rescale(1, num * (int64_t)st->time_base.den * st->codec->ticks_per_frame, den * (int64_t)st->time_base.num);
        }
    }
    if (pkt->pts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && delay == 0) {
        pkt->pts = pkt->dts;
    }
    //XXX/FIXME this is a temporary hack until all encoders output pts
    if ((pkt->pts == 0 || pkt->pts == AV_NOPTS_VALUE) && pkt->dts == AV_NOPTS_VALUE && !delay) {
        pkt->dts =
            //        pkt->pts= st->cur_dts;
            pkt->pts = st->pts.val;
    }
    //calculate dts from pts
    if (pkt->pts != AV_NOPTS_VALUE && pkt->dts == AV_NOPTS_VALUE && delay <= MAX_REORDER_DELAY) {
        st->pts_buffer[0] = pkt->pts;
        for (i = 1; i < delay + 1 && st->pts_buffer[i] == AV_NOPTS_VALUE; i++) {
            st->pts_buffer[i] = pkt->pts + (i - delay - 1) * pkt->duration;
        }
        for (i = 0; i < delay && st->pts_buffer[i] > st->pts_buffer[i + 1]; i++) {
            FFSWAP(int64_t, st->pts_buffer[i], st->pts_buffer[i + 1]);
        }
        pkt->dts = st->pts_buffer[0];
    }
    if (st->cur_dts && st->cur_dts != AV_NOPTS_VALUE && ((!(s->oformat->flags & AVFMT_TS_NONSTRICT) && st->cur_dts >= pkt->dts) || st->cur_dts > pkt->dts)) {
        av_log(s, AV_LOG_ERROR,
               "Application provided invalid, non monotonically increasing dts to muxer in stream %d: %"PRId64" >= %"PRId64"\n",
               st->index, st->cur_dts, pkt->dts);
        return AVERROR(EINVAL);
    }
    if (pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->pts < pkt->dts) {
        av_log(s, AV_LOG_ERROR, "pts < dts in stream %d\n", st->index);
        return AVERROR(EINVAL);
    }
    //    av_log(s, AV_LOG_DEBUG, "av_write_frame: pts2:%"PRId64" dts2:%"PRId64"\n", pkt->pts, pkt->dts);
    st->cur_dts = pkt->dts;
    st->pts.val = pkt->dts;
    /* update pts */
    switch (st->codec->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        frame_size = get_audio_frame_size(st->codec, pkt->size);
        /* HACK/FIXME, we skip the initial 0 size packets as they are most
           likely equal to the encoder delay, but it would be better if we
           had the real timestamps from the encoder */
        if (frame_size >= 0 && (pkt->size || st->pts.num != st->pts.den >> 1 || st->pts.val)) {
            av_frac_add(&st->pts, (int64_t)st->time_base.den * frame_size);
        }
        break;
    case AVMEDIA_TYPE_VIDEO:
        av_frac_add(&st->pts, (int64_t)st->time_base.den * st->codec->time_base.num);
        break;
    default:
        break;
    }
    return 0;
}

int av_write_frame(AVFormatContext *s, AVPacket *pkt)
{
    int ret = compute_pkt_fields2(s, s->streams[pkt->stream_index], pkt);
    if (ret < 0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS)) {
        return ret;
    }
    ret = s->oformat->write_packet(s, pkt);
    if (!ret) {
        ret = url_ferror(s->pb);
    }
    return ret;
}

void ff_interleave_add_packet(AVFormatContext *s, AVPacket *pkt,
                              int (*compare)(AVFormatContext *, AVPacket *, AVPacket *))
{
    AVPacketList **next_point, *this_pktl;
    this_pktl = av_mallocz(sizeof(AVPacketList));
    this_pktl->pkt = *pkt;
    pkt->destruct = NULL;            // do not free original but only the copy
    av_dup_packet(&this_pktl->pkt);  // duplicate the packet if it uses non-alloced memory
    if (s->streams[pkt->stream_index]->last_in_packet_buffer) {
        next_point = &(s->streams[pkt->stream_index]->last_in_packet_buffer->next);
    } else {
        next_point = &s->packet_buffer;
    }
    if (*next_point) {
        if (compare(s, &s->packet_buffer_end->pkt, pkt)) {
            while (!compare(s, &(*next_point)->pkt, pkt)) {
                next_point = &(*next_point)->next;
            }
            goto next_non_null;
        } else {
            next_point = &(s->packet_buffer_end->next);
        }
    }
    assert(!*next_point);
    s->packet_buffer_end = this_pktl;
next_non_null:
    this_pktl->next = *next_point;
    s->streams[pkt->stream_index]->last_in_packet_buffer =
        *next_point = this_pktl;
}

static int ff_interleave_compare_dts(AVFormatContext *s, AVPacket *next, AVPacket *pkt)
{
    AVStream *st = s->streams[ pkt ->stream_index];
    AVStream *st2 = s->streams[ next->stream_index];
    int comp = av_compare_ts(next->dts, st2->time_base, pkt->dts,
                             st->time_base);
    if (comp == 0) {
        return pkt->stream_index < next->stream_index;
    }
    return comp > 0;
}

int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out, AVPacket *pkt, int flush)
{
    AVPacketList *pktl;
    int stream_count = 0;
    int i;
    if (pkt) {
        ff_interleave_add_packet(s, pkt, ff_interleave_compare_dts);
    }
    for (i = 0; i < s->nb_streams; i++) {
        stream_count += !!s->streams[i]->last_in_packet_buffer;
    }
    if (stream_count && (s->nb_streams == stream_count || flush)) {
        pktl = s->packet_buffer;
        *out = pktl->pkt;
        s->packet_buffer = pktl->next;
        if (!s->packet_buffer) {
            s->packet_buffer_end = NULL;
        }
        if (s->streams[out->stream_index]->last_in_packet_buffer == pktl) {
            s->streams[out->stream_index]->last_in_packet_buffer = NULL;
        }
        av_freep(&pktl);
        return 1;
    } else {
        av_init_packet(out);
        return 0;
    }
}

/**
 * Interleave an AVPacket correctly so it can be muxed.
 * @param out the interleaved packet will be output here
 * @param in the input packet
 * @param flush 1 if no further packets are available as input and all
 *              remaining packets should be output
 * @return 1 if a packet was output, 0 if no packet could be output,
 *         < 0 if an error occurred
 */
static int av_interleave_packet(AVFormatContext *s, AVPacket *out, AVPacket *in, int flush)
{
    if (s->oformat->interleave_packet) {
        return s->oformat->interleave_packet(s, out, in, flush);
    } else {
        return av_interleave_packet_per_dts(s, out, in, flush);
    }
}

int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt)
{
    AVStream *st = s->streams[ pkt->stream_index];
    int ret;
    //FIXME/XXX/HACK drop zero sized packets
    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO && pkt->size == 0) {
        return 0;
    }
    av_dlog(s, "av_interleaved_write_frame size:%d dts:%"PRId64" pts:%"PRId64"\n",
            pkt->size, pkt->dts, pkt->pts);
    if ((ret = compute_pkt_fields2(s, st, pkt)) < 0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS)) {
        return ret;
    }
    if (pkt->dts == AV_NOPTS_VALUE && !(s->oformat->flags & AVFMT_NOTIMESTAMPS)) {
        return AVERROR(EINVAL);
    }
    for (;;) {
        AVPacket opkt;
        int ret = av_interleave_packet(s, &opkt, pkt, 0);
        if (ret <= 0) { //FIXME cleanup needed for ret<0 ?
            return ret;
        }
        ret = s->oformat->write_packet(s, &opkt);
        av_free_packet(&opkt);
        pkt = NULL;
        if (ret < 0) {
            return ret;
        }
        if (url_ferror(s->pb)) {
            return url_ferror(s->pb);
        }
    }
}

int av_write_trailer(AVFormatContext *s)
{
    int ret, i;
    for (;;) {
        AVPacket pkt;
        ret = av_interleave_packet(s, &pkt, NULL, 1);
        if (ret < 0) { //FIXME cleanup needed for ret<0 ?
            goto fail;
        }
        if (!ret) {
            break;
        }
        ret = s->oformat->write_packet(s, &pkt);
        av_free_packet(&pkt);
        if (ret < 0) {
            goto fail;
        }
        if (url_ferror(s->pb)) {
            goto fail;
        }
    }
    if (s->oformat->write_trailer) {
        ret = s->oformat->write_trailer(s);
    }
fail:
    if (ret == 0) {
        ret = url_ferror(s->pb);
    }
    for (i = 0; i < s->nb_streams; i++) {
        av_freep(&s->streams[i]->priv_data);
        av_freep(&s->streams[i]->index_entries);
    }
    if (s->iformat && s->iformat->priv_class) {
        av_opt_free(s->priv_data);
    }
    av_freep(&s->priv_data);
    return ret;
}

void ff_program_add_stream_index(AVFormatContext *ac, int progid, unsigned int idx)
{
    int i, j;
    AVProgram *program = NULL;
    void *tmp;
    if (idx >= ac->nb_streams) {
        av_log(ac, AV_LOG_ERROR, "stream index %d is not valid\n", idx);
        return;
    }
    for (i = 0; i < ac->nb_programs; i++) {
        if (ac->programs[i]->id != progid) {
            continue;
        }
        program = ac->programs[i];
        for (j = 0; j < program->nb_stream_indexes; j++)
            if (program->stream_index[j] == idx) {
                return;
            }
        tmp = av_realloc(program->stream_index, sizeof(unsigned int) * (program->nb_stream_indexes + 1));
        if (!tmp) {
            return;
        }
        program->stream_index = tmp;
        program->stream_index[program->nb_stream_indexes++] = idx;
        return;
    }
}

static void print_fps(double d, const char *postfix)
{
    uint64_t v = lrintf(d * 100);
    if (v % 100) {
        av_log(NULL, AV_LOG_INFO, ", %3.2f %s", d, postfix);
    } else if (v % (100 * 1000)) {
        av_log(NULL, AV_LOG_INFO, ", %1.0f %s", d, postfix);
    } else {
        av_log(NULL, AV_LOG_INFO, ", %1.0fk %s", d / 1000, postfix);
    }
}

static void dump_metadata(void *ctx, AVDictionary *m, const char *indent)
{
    if (m && !(m->count == 1 && av_dict_get(m, "language", NULL, 0))) {
        AVDictionaryEntry *tag = NULL;
        av_log(ctx, AV_LOG_INFO, "%sMetadata:\n", indent);
        while ((tag = av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            if (strcmp("language", tag->key)) {
                char tmp[256];
                int i;
                av_strlcpy(tmp, tag->value, sizeof(tmp));
                for (i = 0; i < strlen(tmp); i++) if (tmp[i] == 0xd) {
                        tmp[i] = ' ';
                    }
                av_log(ctx, AV_LOG_INFO, "%s  %-16s: %s\n", indent, tag->key, tmp);
            }
        }
    }
}

/* "user interface" functions */
static void dump_stream_format(AVFormatContext *ic, int i, int index, int is_output)
{
    char buf[256];
    int flags = (is_output ? ic->oformat->flags : ic->iformat->flags);
    AVStream *st = ic->streams[i];
    int g = av_gcd(st->time_base.num, st->time_base.den);
    AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);
    avcodec_string(buf, sizeof(buf), st->codec, is_output);
    av_log(NULL, AV_LOG_INFO, "    Stream #%d.%d", index, i);
    /* the pid is an important information, so we display it */
    /* XXX: add a generic system */
    if (flags & AVFMT_SHOW_IDS) {
        av_log(NULL, AV_LOG_INFO, "[0x%x]", st->id);
    }
    if (lang) {
        av_log(NULL, AV_LOG_INFO, "(%s)", lang->value);
    }
    av_log(NULL, AV_LOG_DEBUG, ", %d, %d/%d", st->codec_info_nb_frames, st->time_base.num / g, st->time_base.den / g);
    av_log(NULL, AV_LOG_INFO, ": %s", buf);
    if (st->sample_aspect_ratio.num && // default
        av_cmp_q(st->sample_aspect_ratio, st->codec->sample_aspect_ratio)) {
        AVRational display_aspect_ratio;
        av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  st->codec->width * st->sample_aspect_ratio.num,
                  st->codec->height * st->sample_aspect_ratio.den,
                  1024 * 1024);
        av_log(NULL, AV_LOG_INFO, ", PAR %d:%d DAR %d:%d",
               st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
               display_aspect_ratio.num, display_aspect_ratio.den);
    }
    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (st->avg_frame_rate.den && st->avg_frame_rate.num) {
            print_fps(av_q2d(st->avg_frame_rate), "fps");
        }
        if (st->r_frame_rate.den && st->r_frame_rate.num) {
            print_fps(av_q2d(st->r_frame_rate), "tbr");
        }
        if (st->time_base.den && st->time_base.num) {
            print_fps(1 / av_q2d(st->time_base), "tbn");
        }
        if (st->codec->time_base.den && st->codec->time_base.num) {
            print_fps(1 / av_q2d(st->codec->time_base), "tbc");
        }
    }
    if (st->disposition & AV_DISPOSITION_DEFAULT) {
        av_log(NULL, AV_LOG_INFO, " (default)");
    }
    if (st->disposition & AV_DISPOSITION_DUB) {
        av_log(NULL, AV_LOG_INFO, " (dub)");
    }
    if (st->disposition & AV_DISPOSITION_ORIGINAL) {
        av_log(NULL, AV_LOG_INFO, " (original)");
    }
    if (st->disposition & AV_DISPOSITION_COMMENT) {
        av_log(NULL, AV_LOG_INFO, " (comment)");
    }
    if (st->disposition & AV_DISPOSITION_LYRICS) {
        av_log(NULL, AV_LOG_INFO, " (lyrics)");
    }
    if (st->disposition & AV_DISPOSITION_KARAOKE) {
        av_log(NULL, AV_LOG_INFO, " (karaoke)");
    }
    if (st->disposition & AV_DISPOSITION_FORCED) {
        av_log(NULL, AV_LOG_INFO, " (forced)");
    }
    if (st->disposition & AV_DISPOSITION_HEARING_IMPAIRED) {
        av_log(NULL, AV_LOG_INFO, " (hearing impaired)");
    }
    if (st->disposition & AV_DISPOSITION_VISUAL_IMPAIRED) {
        av_log(NULL, AV_LOG_INFO, " (visual impaired)");
    }
    if (st->disposition & AV_DISPOSITION_CLEAN_EFFECTS) {
        av_log(NULL, AV_LOG_INFO, " (clean effects)");
    }
    av_log(NULL, AV_LOG_INFO, "\n");
    dump_metadata(NULL, st->metadata, "    ");
}

#if FF_API_DUMP_FORMAT
void dump_format(AVFormatContext *ic,
                 int index,
                 const char *url,
                 int is_output)
{
    av_dump_format(ic, index, url, is_output);
}
#endif

void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output)
{
    int i;
    uint8_t *printed = av_mallocz(ic->nb_streams);
    if (ic->nb_streams && !printed) {
        return;
    }
    av_log(NULL, AV_LOG_INFO, "%s #%d, %s, %s '%s':\n",
           is_output ? "Output" : "Input",
           index,
           is_output ? ic->oformat->name : ic->iformat->name,
           is_output ? "to" : "from", url);
    dump_metadata(NULL, ic->metadata, "  ");
    if (!is_output) {
        av_log(NULL, AV_LOG_INFO, "  Duration: ");
        if (ic->duration != AV_NOPTS_VALUE) {
            int hours, mins, secs, us;
            secs = ic->duration / AV_TIME_BASE;
            us = ic->duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            av_log(NULL, AV_LOG_INFO, "%02d:%02d:%02d.%02d", hours, mins, secs,
                   (100 * us) / AV_TIME_BASE);
        } else {
            av_log(NULL, AV_LOG_INFO, "N/A");
        }
        if (ic->start_time != AV_NOPTS_VALUE) {
            int secs, us;
            av_log(NULL, AV_LOG_INFO, ", start: ");
            secs = ic->start_time / AV_TIME_BASE;
            us = abs(ic->start_time % AV_TIME_BASE);
            av_log(NULL, AV_LOG_INFO, "%d.%06d",
                   secs, (int)av_rescale(us, 1000000, AV_TIME_BASE));
        }
        av_log(NULL, AV_LOG_INFO, ", bitrate: ");
        if (ic->bit_rate) {
            av_log(NULL, AV_LOG_INFO, "%d kb/s", ic->bit_rate / 1000);
        } else {
            av_log(NULL, AV_LOG_INFO, "N/A");
        }
        av_log(NULL, AV_LOG_INFO, "\n");
    }
    for (i = 0; i < ic->nb_chapters; i++) {
        AVChapter *ch = ic->chapters[i];
        av_log(NULL, AV_LOG_INFO, "    Chapter #%d.%d: ", index, i);
        av_log(NULL, AV_LOG_INFO, "start %f, ", ch->start * av_q2d(ch->time_base));
        av_log(NULL, AV_LOG_INFO, "end %f\n",   ch->end   * av_q2d(ch->time_base));
        dump_metadata(NULL, ch->metadata, "    ");
    }
    if (ic->nb_programs) {
        int j, k, total = 0;
        for (j = 0; j < ic->nb_programs; j++) {
            AVDictionaryEntry *name = av_dict_get(ic->programs[j]->metadata,
                                                  "name", NULL, 0);
            av_log(NULL, AV_LOG_INFO, "  Program %d %s\n", ic->programs[j]->id,
                   name ? name->value : "");
            dump_metadata(NULL, ic->programs[j]->metadata, "    ");
            for (k = 0; k < ic->programs[j]->nb_stream_indexes; k++) {
                dump_stream_format(ic, ic->programs[j]->stream_index[k], index, is_output);
                printed[ic->programs[j]->stream_index[k]] = 1;
            }
            total += ic->programs[j]->nb_stream_indexes;
        }
        if (total < ic->nb_streams) {
            av_log(NULL, AV_LOG_INFO, "  No Program\n");
        }
    }
    for (i = 0; i < ic->nb_streams; i++)
        if (!printed[i]) {
            dump_stream_format(ic, i, index, is_output);
        }
    av_free(printed);
}

int64_t av_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

uint64_t ff_ntp_time(void)
{
    return (av_gettime() / 1000) * 1000 + NTP_OFFSET_US;
}

#if FF_API_PARSE_DATE
#include "libavutil/parseutils.h"

int64_t parse_date(const char *timestr, int duration)
{
    int64_t timeval;
    av_parse_time(&timeval, timestr, duration);
    return timeval;
}
#endif

#if FF_API_FIND_INFO_TAG
#include "libavutil/parseutils.h"

int find_info_tag(char *arg, int arg_size, const char *tag1, const char *info)
{
    return av_find_info_tag(arg, arg_size, tag1, info);
}
#endif

int av_get_frame_filename(char *buf, int buf_size,
                          const char *path, int number)
{
    const char *p;
    char *q, buf1[20], c;
    int nd, len, percentd_found;
    q = buf;
    p = path;
    percentd_found = 0;
    for (;;) {
        c = *p++;
        if (c == '\0') {
            break;
        }
        if (c == '%') {
            do {
                nd = 0;
                while (isdigit(*p)) {
                    nd = nd * 10 + *p++ - '0';
                }
                c = *p++;
            } while (isdigit(c));
            switch (c) {
            case '%':
                goto addchar;
            case 'd':
                if (percentd_found) {
                    goto fail;
                }
                percentd_found = 1;
                snprintf(buf1, sizeof(buf1), "%0*d", nd, number);
                len = strlen(buf1);
                if ((q - buf + len) > buf_size - 1) {
                    goto fail;
                }
                memcpy(q, buf1, len);
                q += len;
                break;
            default:
                goto fail;
            }
        } else {
addchar:
            if ((q - buf) < buf_size - 1) {
                *q++ = c;
            }
        }
    }
    if (!percentd_found) {
        goto fail;
    }
    *q = '\0';
    return 0;
fail:
    *q = '\0';
    return -1;
}

static void hex_dump_internal(void *avcl, FILE *f, int level, uint8_t *buf, int size)
{
    int len, i, j, c;
    char sbuf[1024 * 4];
    int off = 0;
    int printed = 0;
#undef fprintf
#define PRINT(...) do { if (!f) {\
            printed= snprintf(sbuf+off,1024*4-off, __VA_ARGS__); \
            if (printed>0) off+=printed;\
        }else fprintf(f, __VA_ARGS__); } while(0)
    for (i = 0; i < size; i += 16) {
        len = size - i;
        if (len > 16) {
            len = 16;
        }
        PRINT("%08x: ", i);
        for (j = 0; j < 16; j++) {
            if (j < len) {
                PRINT(" %02x", buf[i + j]);
            } else {
                PRINT("   ");
            }
        }
        PRINT(" ");
        for (j = 0; j < len; j++) {
            c = buf[i + j];
            if (c < ' ' || c > '~') {
                c = '.';
            }
            PRINT("%c", c);
        }
        PRINT("\n");
    }
    if (!f && off > 0 && off < 1024 * 4) {
        sbuf[off] = '\0';
        av_log(avcl, level, "%s\n", sbuf);
    }
#undef PRINT
}

void av_hex_dump(FILE *f, uint8_t *buf, int size)
{
    hex_dump_internal(NULL, f, 0, buf, size);
}

void av_hex_dump_log(void *avcl, int level, uint8_t *buf, int size)
{
    hex_dump_internal(avcl, NULL, level, buf, size);
}

static void pkt_dump_internal(void *avcl, FILE *f, int level, AVPacket *pkt, int dump_payload, AVRational time_base)
{
#undef fprintf
#define PRINT(...) do { if (!f) av_log(avcl, level, __VA_ARGS__); else fprintf(f, __VA_ARGS__); } while(0)
    PRINT("stream #%d:\n", pkt->stream_index);
    PRINT("  keyframe=%d\n", ((pkt->flags & AV_PKT_FLAG_KEY) != 0));
    PRINT("  duration=%0.3f\n", pkt->duration * av_q2d(time_base));
    /* DTS is _always_ valid after av_read_frame() */
    PRINT("  dts=");
    if (pkt->dts == AV_NOPTS_VALUE) {
        PRINT("N/A");
    } else {
        PRINT("%0.3f", pkt->dts * av_q2d(time_base));
    }
    /* PTS may not be known if B-frames are present. */
    PRINT("  pts=");
    if (pkt->pts == AV_NOPTS_VALUE) {
        PRINT("N/A");
    } else {
        PRINT("%0.3f", pkt->pts * av_q2d(time_base));
    }
    PRINT("\n");
    PRINT("  size=%d\n", pkt->size);
#undef PRINT
    if (dump_payload) {
        av_hex_dump(f, pkt->data, pkt->size);
    }
}

#if FF_API_PKT_DUMP
void av_pkt_dump(FILE *f, AVPacket *pkt, int dump_payload)
{
    AVRational tb = { 1, AV_TIME_BASE };
    pkt_dump_internal(NULL, f, 0, pkt, dump_payload, tb);
}
#endif

void av_pkt_dump2(FILE *f, AVPacket *pkt, int dump_payload, AVStream *st)
{
    pkt_dump_internal(NULL, f, 0, pkt, dump_payload, st->time_base);
}

#if FF_API_PKT_DUMP
void av_pkt_dump_log(void *avcl, int level, AVPacket *pkt, int dump_payload)
{
    AVRational tb = { 1, AV_TIME_BASE };
    pkt_dump_internal(avcl, NULL, level, pkt, dump_payload, tb);
}
#endif

void av_pkt_dump_log2(void *avcl, int level, AVPacket *pkt, int dump_payload,
                      AVStream *st)
{
    pkt_dump_internal(avcl, NULL, level, pkt, dump_payload, st->time_base);
}

void av_url_split(char *proto, int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname, int hostname_size,
                  int *port_ptr,
                  char *path, int path_size,
                  const char *url)
{
    const char *p, *ls, *at, *col, *brk;
    if (port_ptr) {
        *port_ptr = -1;
    }
    if (proto_size > 0) {
        proto[0] = 0;
    }
    if (authorization_size > 0) {
        authorization[0] = 0;
    }
    if (hostname_size > 0) {
        hostname[0] = 0;
    }
    if (path_size > 0) {
        path[0] = 0;
    }
    /* parse protocol */
    if ((p = strchr(url, ':'))) {
        av_strlcpy(proto, url, FFMIN(proto_size, p + 1 - url));
        p++; /* skip ':' */
        if (*p == '/') {
            p++;
        }
        if (*p == '/') {
            p++;
        }
    } else {
        /* no protocol means plain filename */
        av_strlcpy(path, url, path_size);
        return;
    }
    /* separate path from hostname */
    ls = strchr(p, '/');
    if (!ls) {
        ls = strchr(p, '?');
    }
    if (ls) {
        av_strlcpy(path, ls, path_size);
    } else {
        ls = &p[strlen(p)];    // XXX
    }
    /* the rest is hostname, use that to parse auth/port */
    if (ls != p) {
        /* authorization (user[:pass]@hostname) */
        if ((at = strchr(p, '@')) && at < ls) {
            av_strlcpy(authorization, p,
                       FFMIN(authorization_size, at + 1 - p));
            p = at + 1; /* skip '@' */
        }
        if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
            /* [host]:port */
            av_strlcpy(hostname, p + 1,
                       FFMIN(hostname_size, brk - p));
            if (brk[1] == ':' && port_ptr) {
                *port_ptr = atoi(brk + 2);
            }
        } else if ((col = strchr(p, ':')) && col < ls) {
            av_strlcpy(hostname, p,
                       FFMIN(col + 1 - p, hostname_size));
            if (port_ptr) {
                *port_ptr = atoi(col + 1);
            }
        } else
            av_strlcpy(hostname, p,
                       FFMIN(ls + 1 - p, hostname_size));
    }
}

char *ff_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase)
{
    int i;
    static const char hex_table_uc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'A', 'B',
                                           'C', 'D', 'E', 'F'
                                         };
    static const char hex_table_lc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'a', 'b',
                                           'c', 'd', 'e', 'f'
                                         };
    const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;
    for (i = 0; i < s; i++) {
        buff[i * 2]     = hex_table[src[i] >> 4];
        buff[i * 2 + 1] = hex_table[src[i] & 0xF];
    }
    return buff;
}

int ff_hex_to_data(uint8_t *data, const char *p)
{
    int c, len, v;
    len = 0;
    v = 1;
    for (;;) {
        p += strspn(p, SPACE_CHARS);
        if (*p == '\0') {
            break;
        }
        c = toupper((unsigned char) * p++);
        if (c >= '0' && c <= '9') {
            c = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else {
            break;
        }
        v = (v << 4) | c;
        if (v & 0x100) {
            if (data) {
                data[len] = v;
            }
            len++;
            v = 1;
        }
    }
    return len;
}

void av_set_pts_info(AVStream *s, int pts_wrap_bits,
                     unsigned int pts_num, unsigned int pts_den)
{
    AVRational new_tb;
    if (av_reduce(&new_tb.num, &new_tb.den, pts_num, pts_den, INT_MAX)) {
        if (new_tb.num != pts_num) {
            av_log(NULL, AV_LOG_DEBUG, "st:%d removing common factor %d from timebase\n", s->index, pts_num / new_tb.num);
        }
    } else {
        av_log(NULL, AV_LOG_WARNING, "st:%d has too large timebase, reducing\n", s->index);
    }
    if (new_tb.num <= 0 || new_tb.den <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Ignoring attempt to set invalid timebase for st:%d\n", s->index);
        return;
    }
    s->time_base = new_tb;
    s->pts_wrap_bits = pts_wrap_bits;
}

int ff_url_join(char *str, int size, const char *proto,
                const char *authorization, const char *hostname,
                int port, const char *fmt, ...)
{
#if CONFIG_NETWORK
    struct addrinfo hints, *ai;
#endif
    str[0] = '\0';
    if (proto) {
        av_strlcatf(str, size, "%s://", proto);
    }
    if (authorization && authorization[0]) {
        av_strlcatf(str, size, "%s@", authorization);
    }
#if CONFIG_NETWORK && defined(AF_INET6)
    /* Determine if hostname is a numerical IPv6 address,
     * properly escape it within [] in that case. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    if (!getaddrinfo(hostname, NULL, &hints, &ai)) {
        if (ai->ai_family == AF_INET6) {
            av_strlcat(str, "[", size);
            av_strlcat(str, hostname, size);
            av_strlcat(str, "]", size);
        } else {
            av_strlcat(str, hostname, size);
        }
        freeaddrinfo(ai);
    } else
#endif
        /* Not an IPv6 address, just output the plain string. */
        av_strlcat(str, hostname, size);
    if (port >= 0) {
        av_strlcatf(str, size, ":%d", port);
    }
    if (fmt) {
        va_list vl;
        int len = strlen(str);
        va_start(vl, fmt);
        vsnprintf(str + len, size > len ? size - len : 0, fmt, vl);
        va_end(vl);
    }
    return strlen(str);
}

int ff_write_chained(AVFormatContext *dst, int dst_stream, AVPacket *pkt,
                     AVFormatContext *src)
{
    AVPacket local_pkt;
    local_pkt = *pkt;
    local_pkt.stream_index = dst_stream;
    if (pkt->pts != AV_NOPTS_VALUE)
        local_pkt.pts = av_rescale_q(pkt->pts,
                                     src->streams[pkt->stream_index]->time_base,
                                     dst->streams[dst_stream]->time_base);
    if (pkt->dts != AV_NOPTS_VALUE)
        local_pkt.dts = av_rescale_q(pkt->dts,
                                     src->streams[pkt->stream_index]->time_base,
                                     dst->streams[dst_stream]->time_base);
    return av_write_frame(dst, &local_pkt);
}

void ff_parse_key_value(const char *str, ff_parse_key_val_cb callback_get_buf,
                        void *context)
{
    const char *ptr = str;
    /* Parse key=value pairs. */
    for (;;) {
        const char *key;
        char *dest = NULL, *dest_end;
        int key_len, dest_len = 0;
        /* Skip whitespace and potential commas. */
        while (*ptr && (isspace(*ptr) || *ptr == ',')) {
            ptr++;
        }
        if (!*ptr) {
            break;
        }
        key = ptr;
        if (!(ptr = strchr(key, '='))) {
            break;
        }
        ptr++;
        key_len = ptr - key;
        callback_get_buf(context, key, key_len, &dest, &dest_len);
        dest_end = dest + dest_len - 1;
        if (*ptr == '\"') {
            ptr++;
            while (*ptr && *ptr != '\"') {
                if (*ptr == '\\') {
                    if (!ptr[1]) {
                        break;
                    }
                    if (dest && dest < dest_end) {
                        *dest++ = ptr[1];
                    }
                    ptr += 2;
                } else {
                    if (dest && dest < dest_end) {
                        *dest++ = *ptr;
                    }
                    ptr++;
                }
            }
            if (*ptr == '\"') {
                ptr++;
            }
        } else {
            for (; *ptr && !(isspace(*ptr) || *ptr == ','); ptr++)
                if (dest && dest < dest_end) {
                    *dest++ = *ptr;
                }
        }
        if (dest) {
            *dest = 0;
        }
    }
}

int ff_find_stream_index(AVFormatContext *s, int id)
{
    int i;
    for (i = 0; i < s->nb_streams; i++) {
        if (s->streams[i]->id == id) {
            return i;
        }
    }
    return -1;
}

void ff_make_absolute_url(char *buf, int size, const char *base,
                          const char *rel)
{
    char *sep;
    char *protol_prefix;
    char *option_start;
    /* Absolute path, relative to the current server */
    if (base && strstr(base, "://") && rel[0] == '/') {
        if (base != buf) {
            av_strlcpy(buf, base, size);
        }
        sep = strstr(buf, "://");
        if (sep) {
            sep += 3;
            sep = strchr(sep, '/');
            if (sep) {
                *sep = '\0';
            }
        }
        av_strlcat(buf, rel, size);
        return;
    }
    //av_log(NULL, AV_LOG_DEBUG,"[%s:%d],buf:%s\r\n,base:%s\r\n,rel:%s\n",__FUNCTION__,__LINE__,buf,base,rel);
    protol_prefix = strstr(rel, "://");
    option_start = strstr(rel, "?");
    /* If rel actually is an absolute url, just copy it */
    if (!base  || rel[0] == '/' || (option_start == NULL  && protol_prefix) || (option_start  && protol_prefix != NULL && protol_prefix < option_start)) {
        /* refurl  have  http://,ftp://,and don't have "?"
          refurl  have  http://,ftp://,and  have "?", so we must ensure it is not a option, link  refurl=filename?authurl=http://xxxxxx
        */
        av_strlcpy(buf, rel, size);
        return;
    }
    //av_log(NULL, AV_LOG_DEBUG,"[%s:%d],buf:%s\r\n,base:%s\r\n,rel:%s\n",__FUNCTION__,__LINE__,buf,base,rel);
    if (base != buf) {
        av_strlcpy(buf, base, size);
    }
    /* Remove the file name from the base url */
    option_start = strchr(buf, '?'); /*cut the ? tail, we don't need auth&parameters info..*/
    if (option_start) {
        option_start[0] = '\0';
    }
    sep = strrchr(buf, '/');
    if (sep) {
        sep[1] = '\0';
    } else {
        buf[0] = '\0';
    }
    while (av_strstart(rel, "../", NULL) && sep) {
        /* Remove the path delimiter at the end */
        sep[0] = '\0';
        sep = strrchr(buf, '/');
        /* If the next directory name to pop off is "..", break here */
        if (!strcmp(sep ? &sep[1] : buf, "..")) {
            /* Readd the slash we just removed */
            av_strlcat(buf, "/", size);
            break;
        }
        /* Cut off the directory name */
        if (sep) {
            sep[1] = '\0';
        } else {
            buf[0] = '\0';
        }
        rel += 3;
    }
    av_strlcat(buf, rel, size);
    //av_log(NULL, AV_LOG_DEBUG,"[%s:%d],buf:%s\r\n,base:%s\r\n,rel:%s\n",__FUNCTION__,__LINE__,buf,base,rel);
}
