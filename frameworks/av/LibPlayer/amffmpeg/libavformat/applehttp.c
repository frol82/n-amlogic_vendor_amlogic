/*
 * Apple HTTP Live Streaming demuxer
 * Copyright (c) 2010 Martin Storsjo
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

/**
 * @file
 * Apple HTTP Live Streaming demuxer
 * http://tools.ietf.org/html/draft-pantos-http-live-streaming
 */

#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "internal.h"
#include <unistd.h>
#include "avio_internal.h"
#include "url.h"

#define INITIAL_BUFFER_SIZE 32768

/*
 * An apple http stream consists of a playlist with media segment files,
 * played sequentially. There may be several playlists with the same
 * video content, in different bandwidth variants, that are played in
 * parallel (preferrably only one bandwidth variant at a time). In this case,
 * the user supplied the url to a main playlist that only lists the variant
 * playlists.
 *
 * If the main playlist doesn't point at any variants, we still create
 * one anonymous toplevel variant for this, to maintain the structure.
 */

enum KeyType {
    KEY_NONE,
    KEY_AES_128,
};

struct segment {
    int duration;
    char url[MAX_URL_SIZE];
    char key[MAX_URL_SIZE];
    enum KeyType key_type;
    uint8_t iv[16];
};

/*
 * Each variant has its own demuxer. If it currently is active,
 * it has an open AVIOContext too, and potentially an AVPacket
 * containing the next packet from this stream.
 */
struct variant {
    int bandwidth;
    char url[MAX_URL_SIZE];
    AVIOContext pb;
    uint8_t* read_buffer;
    URLContext *input;
    AVFormatContext *parent;
    int index;
    AVFormatContext *ctx;
    AVPacket pkt;
    int stream_offset;

    int finished;
    int target_duration;
    int start_seq_no;
    int n_segments;
    struct segment **segments;
    int needed, cur_needed;
    int cur_seq_no;
    int64_t last_load_time;

    char key_url[MAX_URL_SIZE];
    uint8_t key[16];
};

typedef struct AppleHTTPContext {
    int n_variants;
    struct variant **variants;
    int cur_seq_no;
    int end_of_segment;
    int first_packet;
} AppleHTTPContext;

static int read_chomp_line(AVIOContext *s, char *buf, int maxlen)
{
    int len = ff_get_line(s, buf, maxlen);
    while (len > 0 && isspace(buf[len - 1])) {
        buf[--len] = '\0';
    }
    return len;
}

static void free_segment_list(struct variant *var)
{
    int i;
    for (i = 0; i < var->n_segments; i++) {
        av_free(var->segments[i]);
    }
    av_freep(&var->segments);
    var->n_segments = 0;
}

static void free_variant_list(AppleHTTPContext *c)
{
    int i;
    for (i = 0; i < c->n_variants; i++) {
        struct variant *var = c->variants[i];
        free_segment_list(var);
        av_free_packet(&var->pkt);
        av_free(var->pb.buffer);
        if (var->input) {
            ffurl_close(var->input);
        }
        if (var->ctx) {
            var->ctx->pb = NULL;
            av_close_input_file(var->ctx);
        }
        av_free(var);
    }
    av_freep(&c->variants);
    c->n_variants = 0;
}

/*
 * Used to reset a statically allocated AVPacket to a clean slate,
 * containing no data.
 */
static void reset_packet(AVPacket *pkt)
{
    av_init_packet(pkt);
    pkt->data = NULL;
}

static struct variant *new_variant(AppleHTTPContext *c, int bandwidth,
                                   const char *url, const char *base)
{
    struct variant *var = av_mallocz(sizeof(struct variant));
    if (!var) {
        return NULL;
    }
    reset_packet(&var->pkt);
    var->bandwidth = bandwidth;
    ff_make_absolute_url(var->url, sizeof(var->url), base, url);
    dynarray_add(&c->variants, &c->n_variants, var);
    return var;
}

struct variant_info {
    char bandwidth[20];
};

static void handle_variant_args(struct variant_info *info, const char *key,
                                int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "BANDWIDTH=", key_len)) {
        *dest     =        info->bandwidth;
        *dest_len = sizeof(info->bandwidth);
    }
}

struct key_info {
    char uri[MAX_URL_SIZE];
    char method[10];
    char iv[35];
};

static void handle_key_args(struct key_info *info, const char *key,
                            int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "METHOD=", key_len)) {
        *dest     =        info->method;
        *dest_len = sizeof(info->method);
    } else if (!strncmp(key, "URI=", key_len)) {
        *dest     =        info->uri;
        *dest_len = sizeof(info->uri);
    } else if (!strncmp(key, "IV=", key_len)) {
        *dest     =        info->iv;
        *dest_len = sizeof(info->iv);
    }
}

static int parse_playlist(AppleHTTPContext *c, const char *url,
                          struct variant *var, AVIOContext *in)
{
    int ret = 0, duration = 0, is_segment = 0, is_variant = 0, bandwidth = 0;
    enum KeyType key_type = KEY_NONE;
    uint8_t iv[16] = "";
    int has_iv = 0;
    char key[MAX_URL_SIZE];
    char line[1024];
    const char *ptr;
    int close_in = 0;

    if (!in) {
        close_in = 1;
        if ((ret = avio_open(&in, url, AVIO_FLAG_READ)) < 0) {
            return ret;
        }
    }

    read_chomp_line(in, line, sizeof(line));
    if (strcmp(line, "#EXTM3U")) {
        ret = AVERROR_INVALIDDATA;
        goto fail;
    }

    if (var) {
        free_segment_list(var);
        var->finished = 0;
    }
    while (!url_feof(in)) {
        read_chomp_line(in, line, sizeof(line));
        if (av_strstart(line, "#EXT-X-STREAM-INF:", &ptr)) {
            struct variant_info info = {{0}};
            is_variant = 1;
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_variant_args,
                               &info);
            bandwidth = atoi(info.bandwidth);
        } else if (av_strstart(line, "#EXT-X-KEY:", &ptr)) {
            struct key_info info = {{0}};
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_key_args,
                               &info);
            key_type = KEY_NONE;
            has_iv = 0;
            if (!strcmp(info.method, "AES-128")) {
                key_type = KEY_AES_128;
            }
            if (!strncmp(info.iv, "0x", 2) || !strncmp(info.iv, "0X", 2)) {
                ff_hex_to_data(iv, info.iv + 2);
                has_iv = 1;
            }
            av_strlcpy(key, info.uri, sizeof(key));
        } else if (av_strstart(line, "#EXT-X-TARGETDURATION:", &ptr)) {
            if (!var) {
                var = new_variant(c, 0, url, NULL);
                if (!var) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
            }
            var->target_duration = atoi(ptr);
        } else if (av_strstart(line, "#EXT-X-MEDIA-SEQUENCE:", &ptr)) {
            if (!var) {
                var = new_variant(c, 0, url, NULL);
                if (!var) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
            }
            var->start_seq_no = atoi(ptr);
        } else if (av_strstart(line, "#EXT-X-ENDLIST", &ptr)) {
            if (var) {
                var->finished = 1;
            }
        } else if (av_strstart(line, "#EXTINF:", &ptr)) {
            is_segment = 1;
            duration   = atoi(ptr);
        } else if (av_strstart(line, "#", NULL)) {
            continue;
        } else if (line[0]) {
            if (is_variant) {
                if (!new_variant(c, bandwidth, line, url)) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
                is_variant = 0;
                bandwidth  = 0;
            }
            if (is_segment) {
                struct segment *seg;
                if (!var) {
                    var = new_variant(c, 0, url, NULL);
                    if (!var) {
                        ret = AVERROR(ENOMEM);
                        goto fail;
                    }
                }
                seg = av_malloc(sizeof(struct segment));
                if (!seg) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
                seg->duration = duration;
                seg->key_type = key_type;
                if (has_iv) {
                    memcpy(seg->iv, iv, sizeof(iv));
                } else {
                    int seq = var->start_seq_no + var->n_segments;
                    memset(seg->iv, 0, sizeof(seg->iv));
                    AV_WB32(seg->iv + 12, seq);
                }
                ff_make_absolute_url(seg->key, sizeof(seg->key), url, key);
                ff_make_absolute_url(seg->url, sizeof(seg->url), url, line);
                dynarray_add(&var->segments, &var->n_segments, seg);
                is_segment = 0;
            }
        }
    }
    if (var) {
        var->last_load_time = av_gettime();
    }

fail:
    if (close_in) {
        avio_close(in);
    }
    return ret;
}

static int open_input(struct variant *var)
{
    struct segment *seg = var->segments[var->cur_seq_no - var->start_seq_no];
    if (seg->key_type == KEY_NONE) {
        return ffurl_open(&var->input, seg->url, AVIO_FLAG_READ);
    } else if (seg->key_type == KEY_AES_128) {
        char iv[33], key[33], url[MAX_URL_SIZE];
        int ret;
        if (strcmp(seg->key, var->key_url)) {
            URLContext *uc;
            if (ffurl_open(&uc, seg->key, AVIO_FLAG_READ) == 0) {
                if (ffurl_read_complete(uc, var->key, sizeof(var->key))
                    != sizeof(var->key)) {
                    av_log(NULL, AV_LOG_ERROR, "Unable to read key file %s\n",
                           seg->key);
                }
                ffurl_close(uc);
            } else {
                av_log(NULL, AV_LOG_ERROR, "Unable to open key file %s\n",
                       seg->key);
            }
            av_strlcpy(var->key_url, seg->key, sizeof(var->key_url));
        }
        ff_data_to_hex(iv, seg->iv, sizeof(seg->iv), 0);
        ff_data_to_hex(key, var->key, sizeof(var->key), 0);
        iv[32] = key[32] = '\0';
        if (strstr(seg->url, "://")) {
            snprintf(url, sizeof(url), "crypto+%s", seg->url);
        } else {
            snprintf(url, sizeof(url), "crypto:%s", seg->url);
        }
        if ((ret = ffurl_alloc(&var->input, url, AVIO_FLAG_READ)) < 0) {
            return ret;
        }
        av_set_string3(var->input->priv_data, "key", key, 0, NULL);
        av_set_string3(var->input->priv_data, "iv", iv, 0, NULL);
        if ((ret = ffurl_connect(var->input)) < 0) {
            ffurl_close(var->input);
            var->input = NULL;
            return ret;
        }
        return 0;
    }
    return AVERROR(ENOSYS);
}

static int read_data(void *opaque, uint8_t *buf, int buf_size)
{
    struct variant *v = opaque;
    AppleHTTPContext *c = v->parent->priv_data;
    int ret, i;

restart:
    if (!v->input) {
reload:
        /* If this is a live stream and target_duration has elapsed since
         * the last playlist reload, reload the variant playlists now. */
        if (!v->finished &&
            av_gettime() - v->last_load_time >= v->target_duration * 1000000 &&
            (ret = parse_playlist(c, v->url, v, NULL)) < 0) {
            return ret;
        }
        if (v->cur_seq_no < v->start_seq_no) {
            av_log(NULL, AV_LOG_WARNING,
                   "skipping %d segments ahead, expired from playlists\n",
                   v->start_seq_no - v->cur_seq_no);
            v->cur_seq_no = v->start_seq_no;
        }
        if (v->cur_seq_no >= v->start_seq_no + v->n_segments) {
            if (v->finished) {
                return AVERROR_EOF;
            }
            while (av_gettime() - v->last_load_time <
                   v->target_duration * 1000000) {
                if (url_interrupt_cb()) {
                    return AVERROR_EXIT;
                }
                usleep(100 * 1000);
            }
            /* Enough time has elapsed since the last reload */
            goto reload;
        }

        ret = open_input(v);
        if (ret < 0) {
            return ret;
        }
    }
    ret = ffurl_read(v->input, buf, buf_size);
    if (ret > 0) {
        return ret;
    }
    if (ret < 0 && ret != AVERROR_EOF) {
        return ret;
    }
    ffurl_close(v->input);
    v->input = NULL;
    v->cur_seq_no++;

    c->end_of_segment = 1;
    c->cur_seq_no = v->cur_seq_no;

    if (v->ctx) {
        v->needed = 0;
        for (i = v->stream_offset; i < v->stream_offset + v->ctx->nb_streams;
             i++) {
            if (v->parent->streams[i]->discard < AVDISCARD_ALL) {
                v->needed = 1;
            }
        }
    }
    if (!v->needed) {
        av_log(v->parent, AV_LOG_INFO, "No longer receiving variant %d\n",
               v->index);
        return AVERROR_EOF;
    }
    goto restart;
}

static int applehttp_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    AppleHTTPContext *c = s->priv_data;
    int ret = 0, i, j, stream_offset = 0;

    if ((ret = parse_playlist(c, s->filename, NULL, s->pb)) < 0) {
        goto fail;
    }

    if (c->n_variants == 0) {
        av_log(NULL, AV_LOG_WARNING, "Empty playlist\n");
        ret = AVERROR_EOF;
        goto fail;
    }
    /* If the playlist only contained variants, parse each individual
     * variant playlist. */
    if (c->n_variants > 1 || c->variants[0]->n_segments == 0) {
        for (i = 0; i < c->n_variants; i++) {
            struct variant *v = c->variants[i];
            if ((ret = parse_playlist(c, v->url, v, NULL)) < 0) {
                goto fail;
            }
        }
    }

    if (c->variants[0]->n_segments == 0) {
        av_log(NULL, AV_LOG_WARNING, "Empty playlist\n");
        ret = AVERROR_EOF;
        goto fail;
    }

    /* If this isn't a live stream, calculate the total duration of the
     * stream. */
    if (c->variants[0]->finished) {
        int64_t duration = 0;
        for (i = 0; i < c->variants[0]->n_segments; i++) {
            duration += c->variants[0]->segments[i]->duration;
        }
        s->duration = duration * AV_TIME_BASE;
    }

    /* Open the demuxer for each variant */
    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];
        AVInputFormat *in_fmt = NULL;
        char bitrate_str[20];
        if (v->n_segments == 0) {
            continue;
        }

        if (!(v->ctx = avformat_alloc_context())) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        v->index  = i;
        v->needed = 1;
        v->parent = s;

        /* If this is a live stream with more than 3 segments, start at the
         * third last segment. */
        v->cur_seq_no = v->start_seq_no;
        if (!v->finished && v->n_segments > 3) {
            v->cur_seq_no = v->start_seq_no + v->n_segments - 3;
        }

        v->read_buffer = av_malloc(INITIAL_BUFFER_SIZE);
        ffio_init_context(&v->pb, v->read_buffer, INITIAL_BUFFER_SIZE, 0, v,
                          read_data, NULL, NULL);
        v->pb.seekable = 0;
        ret = av_probe_input_buffer(&v->pb, &in_fmt, v->segments[0]->url,
                                    NULL, 0, 0);
        if (ret < 0) {
            goto fail;
        }
        v->ctx->pb       = &v->pb;
        ret = avformat_open_input(&v->ctx, v->segments[0]->url, in_fmt, NULL);
        if (ret < 0) {
            goto fail;
        }
        v->stream_offset = stream_offset;
        snprintf(bitrate_str, sizeof(bitrate_str), "%d", v->bandwidth);
        /* Create new AVStreams for each stream in this variant */
        for (j = 0; j < v->ctx->nb_streams; j++) {
            AVStream *st = av_new_stream(s, i);
            if (!st) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            avcodec_copy_context(st->codec, v->ctx->streams[j]->codec);
            if (v->bandwidth)
                av_dict_set(&st->metadata, "variant_bitrate", bitrate_str,
                            0);
        }
        stream_offset += v->ctx->nb_streams;
    }

    c->first_packet = 1;

    return 0;
fail:
    free_variant_list(c);
    return ret;
}

static int recheck_discard_flags(AVFormatContext *s, int first)
{
    AppleHTTPContext *c = s->priv_data;
    int i, changed = 0;

    /* Check if any new streams are needed */
    for (i = 0; i < c->n_variants; i++) {
        c->variants[i]->cur_needed = 0;
    };

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        struct variant *var = c->variants[s->streams[i]->id];
        if (st->discard < AVDISCARD_ALL) {
            var->cur_needed = 1;
        }
    }
    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];
        if (v->cur_needed && !v->needed) {
            v->needed = 1;
            changed = 1;
            v->cur_seq_no = c->cur_seq_no;
            v->pb.eof_reached = 0;
            av_log(s, AV_LOG_INFO, "Now receiving variant %d\n", i);
        } else if (first && !v->cur_needed && v->needed) {
            if (v->input) {
                ffurl_close(v->input);
            }
            v->input = NULL;
            v->needed = 0;
            changed = 1;
            av_log(s, AV_LOG_INFO, "No longer receiving variant %d\n", i);
        }
    }
    return changed;
}

static int applehttp_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AppleHTTPContext *c = s->priv_data;
    int ret, i, minvariant = -1;

    if (c->first_packet) {
        recheck_discard_flags(s, 1);
        c->first_packet = 0;
    }

start:
    c->end_of_segment = 0;
    for (i = 0; i < c->n_variants; i++) {
        struct variant *var = c->variants[i];
        /* Make sure we've got one buffered packet from each open variant
         * stream */
        if (var->needed && !var->pkt.data) {
            ret = av_read_frame(var->ctx, &var->pkt);
            if (ret < 0) {
                if (!url_feof(&var->pb)) {
                    return ret;
                }
                reset_packet(&var->pkt);
            }
        }
        /* Check if this stream has the packet with the lowest dts */
        if (var->pkt.data) {
            if (minvariant < 0 ||
                var->pkt.dts < c->variants[minvariant]->pkt.dts) {
                minvariant = i;
            }
        }
    }
    if (c->end_of_segment) {
        if (recheck_discard_flags(s, 0)) {
            goto start;
        }
    }
    /* If we got a packet, return it */
    if (minvariant >= 0) {
        *pkt = c->variants[minvariant]->pkt;
        pkt->stream_index += c->variants[minvariant]->stream_offset;
        reset_packet(&c->variants[minvariant]->pkt);
        return 0;
    }
    return AVERROR_EOF;
}

static int applehttp_close(AVFormatContext *s)
{
    AppleHTTPContext *c = s->priv_data;

    free_variant_list(c);
    return 0;
}

static int applehttp_read_seek(AVFormatContext *s, int stream_index,
                               int64_t timestamp, int flags)
{
    AppleHTTPContext *c = s->priv_data;
    int i, j, ret;

    if ((flags & AVSEEK_FLAG_BYTE) || !c->variants[0]->finished) {
        return AVERROR(ENOSYS);
    }

    timestamp = av_rescale_rnd(timestamp, 1, stream_index >= 0 ?
                               s->streams[stream_index]->time_base.den :
                               AV_TIME_BASE, flags & AVSEEK_FLAG_BACKWARD ?
                               AV_ROUND_DOWN : AV_ROUND_UP);
    ret = AVERROR(EIO);
    for (i = 0; i < c->n_variants; i++) {
        /* Reset reading */
        struct variant *var = c->variants[i];
        int64_t pos = 0;
        if (var->input) {
            ffurl_close(var->input);
            var->input = NULL;
        }
        av_free_packet(&var->pkt);
        reset_packet(&var->pkt);
        var->pb.eof_reached = 0;

        /* Locate the segment that contains the target timestamp */
        for (j = 0; j < var->n_segments; j++) {
            if (timestamp >= pos &&
                timestamp < pos + var->segments[j]->duration) {
                var->cur_seq_no = var->start_seq_no + j;
                ret = 0;
                break;
            }
            pos += var->segments[j]->duration;
        }
    }
    return ret;
}

static int applehttp_probe(AVProbeData *p)
{
    /* Require #EXTM3U at the start, and either one of the ones below
     * somewhere for a proper match. */
    if (strncmp(p->buf, "#EXTM3U", 7)) {
        return 0;
    }
    if (strstr(p->buf, "#EXT-X-STREAM-INF:")     ||
        strstr(p->buf, "#EXT-X-TARGETDURATION:") ||
        strstr(p->buf, "#EXT-X-MEDIA-SEQUENCE:")) {
        return AVPROBE_SCORE_MAX;
    }
    return 0;
}

AVInputFormat ff_applehttp_demuxer = {
    "applehttp",
    NULL_IF_CONFIG_SMALL("Apple HTTP Live Streaming format"),
    sizeof(AppleHTTPContext),
    applehttp_probe,
    applehttp_read_header,
    applehttp_read_packet,
    applehttp_close,
    applehttp_read_seek,
};
