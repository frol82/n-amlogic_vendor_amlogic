/*
 * MOV demuxer
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2009 Baptiste Coudurier <baptiste dot coudurier at gmail dot com>
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

#include <limits.h>

//#define DEBUG
//#define MOV_EXPORT_ALL_METADATA

#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"
#include "seek.h"

#if CONFIG_ZLIB
#include <zlib.h>
#endif

/*
 * First version by Francois Revol revol@free.fr
 * Seek function by Gael Chardon gael.dev@4now.net
 *
 * Features and limitations:
 * - reads most of the QT files I have (at least the structure),
 *   Sample QuickTime files with mp3 audio can be found at: http://www.3ivx.com/showcase.html
 * - the code is quite ugly... maybe I won't do it recursive next time :-)
 *
 * Funny I didn't know about http://sourceforge.net/projects/qt-ffmpeg/
 * when coding this :) (it's a writer anyway)
 *
 * Reference documents:
 * http://www.geocities.com/xhelmboyx/quicktime/formats/qtm-layout.txt
 * Apple:
 *  http://developer.apple.com/documentation/QuickTime/QTFF/
 *  http://developer.apple.com/documentation/QuickTime/QTFF/qtff.pdf
 * QuickTime is a trademark of Apple (AFAIK :))
 */

#include "qtpalette.h"


#undef NDEBUG
#include <assert.h>

/* XXX: it's the first time I make a recursive parser I think... sorry if it's ugly :P */

/* those functions parse an atom */
/* return code:
  0: continue to parse next atom
 <0: error occurred, exit
*/
/* links atom IDs to parse functions */
typedef struct MOVParseTableEntry {
    uint32_t type;
    int (*parse)(MOVContext *ctx, AVIOContext *pb, MOVAtom atom);
} MOVParseTableEntry;

static const MOVParseTableEntry mov_default_parse_table[];
#define MAX_READ_SEEK (1024*1024*3-32*1024)//DEF_MAX_READ_SEEK-block_read_size
#define LIMIT_BUFSIZE (6.8*1024*1024)

static int mov_metadata_track_or_disc_number(MOVContext *c, AVIOContext *pb, unsigned len, const char *type)
{
    char buf[16];

    avio_rb16(pb); // unknown
    snprintf(buf, sizeof(buf), "%d", avio_rb16(pb));
    av_dict_set(&c->fc->metadata, type, buf, 0);

    avio_rb16(pb); // total tracks/discs

    return 0;
}

static const uint32_t mac_to_unicode[128] = {
    0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
    0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
    0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
    0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
    0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
    0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
    0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
    0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
    0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
    0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
    0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
    0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
    0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
    0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
    0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
    0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7,
};

static int mov_read_mac_string(MOVContext *c, AVIOContext *pb, int len,
                               char *dst, int dstlen)
{
    char *p = dst;
    char *end = dst + dstlen - 1;
    int i;

    for (i = 0; i < len; i++) {
        uint8_t t, c = avio_r8(pb);
        if (c < 0x80 && p < end) {
            *p++ = c;
        } else {
            PUT_UTF8(mac_to_unicode[c - 0x80], t, if (p < end) *p++ = t;);
        }
}
*p = 0;
return p - dst;
}

static int mov_extract_cover_pic(AVFormatContext *s, AVIOContext *pb, int type, int size, char *value)
{
    if (s->cover_data) {
        av_log(s, AV_LOG_INFO, "Extract cover picture in other atom!\n");
        return 0;
    }

    s->cover_data = av_malloc(size);
    if (!s->cover_data) {
        av_log(s, AV_LOG_INFO, "no memery, av_alloc failed!\n");
        return -1;
    }
    s->cover_data_len = size;
    avio_read(pb, s->cover_data, size);

    if (type == 13) {
        strcpy(value, "image/jpeg");    // jpeg
    } else if (type == 14) {
        strcpy(value, "image/png");    // png
    }

    return 0;
}

static int mov_read_udta_string(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
#ifdef MOV_EXPORT_ALL_METADATA
    char tmp_key[5];
#endif
    char str[1024], key2[16], language[4] = {0};
    const char *key = NULL;
    uint16_t str_size, langcode = 0;
    uint32_t cover_size = 0;
    uint32_t data_type = 0;
    int (*parse)(MOVContext*, AVIOContext*, unsigned, const char *) = NULL;

    switch (atom.type) {
    case MKTAG(0xa9, 'n', 'a', 'm'):
        key = "title";
        break;
    case MKTAG(0xa9, 'a', 'u', 't'):
    case MKTAG(0xa9, 'A', 'R', 'T'):
        key = "artist";
        break;
    case MKTAG('a', 'A', 'R', 'T'):
        key = "album_artist";
        break;
    case MKTAG(0xa9, 'w', 'r', 't'):
        key = "composer";
        break;
    case MKTAG('c', 'p', 'r', 't'):
    case MKTAG(0xa9, 'c', 'p', 'y'):
        key = "copyright";
        break;
    case MKTAG(0xa9, 'g', 'r', 'p'):
        key = "grouping";
        break;
    case MKTAG(0xa9, 'l', 'y', 'r'):
        key = "lyrics";
        break;
    case MKTAG(0xa9, 'c', 'm', 't'):
    case MKTAG(0xa9, 'i', 'n', 'f'):
        key = "comment";
        break;
    case MKTAG(0xa9, 'a', 'l', 'b'):
        key = "album";
        break;
    case MKTAG(0xa9, 'd', 'a', 'y'):
        key = "date";
        break;
    case MKTAG(0xa9, 'g', 'e', 'n'):
        key = "genre";
        break;
    case MKTAG(0xa9, 't', 'o', 'o'):
    case MKTAG(0xa9, 's', 'w', 'r'):
        key = "encoder";
        break;
    case MKTAG(0xa9, 'e', 'n', 'c'):
        key = "encoder";
        break;
    case MKTAG('d', 'e', 's', 'c'):
        key = "description";
        break;
    case MKTAG('l', 'd', 'e', 's'):
        key = "synopsis";
        break;
    case MKTAG('t', 'v', 's', 'h'):
        key = "show";
        break;
    case MKTAG('t', 'v', 'e', 'n'):
        key = "episode_id";
        break;
    case MKTAG('t', 'v', 'n', 'n'):
        key = "network";
        break;
    case MKTAG('t', 'r', 'k', 'n'):
        key = "track";
        parse = mov_metadata_track_or_disc_number;
        break;
    case MKTAG('d', 'i', 's', 'k'):
        key = "disc";
        parse = mov_metadata_track_or_disc_number;
        break;
    case MKTAG('c', 'o', 'v', 'r'):
        key = "cover_pic";
        break;
    case MKTAG(0xa9, 'x', 'y', 'z'):
        key = "GPSCoordinates";
        break;
    }

    if (c->itunes_metadata && atom.size > 8) {
        int data_size = avio_rb32(pb);
        int tag = avio_rl32(pb);
        if (tag == MKTAG('d', 'a', 't', 'a')) {
            data_type = avio_rb32(pb); // type
            avio_rb32(pb); // unknown
            str_size = data_size - 16;
            cover_size = data_size - 16;
            atom.size -= 16;
        } else {
            return 0;
        }
    } else if (atom.size > 4 && key && !c->itunes_metadata) {
        str_size = avio_rb16(pb); // string length
        langcode = avio_rb16(pb);
        ff_mov_lang_to_iso639(langcode, language);
        atom.size -= 4;
    } else {
        str_size = atom.size;
    }

#ifdef MOV_EXPORT_ALL_METADATA
    if (!key) {
        snprintf(tmp_key, 5, "%.4s", (char*)&atom.type);
        key = tmp_key;
    }
#endif

    if (!key) {
        return 0;
    }
    if (atom.size < 0) {
        return -1;
    }

    str_size = FFMIN3(sizeof(str) - 1, str_size, atom.size);

    if (parse) {
        parse(c, pb, str_size, key);
    } else {
        if (data_type == 3 || (data_type == 0 && langcode < 0x800)) { // MAC Encoded
            mov_read_mac_string(c, pb, str_size, str, sizeof(str));
        } else if (data_type == 13 || data_type == 14) {
            mov_extract_cover_pic(c->fc, pb, data_type, cover_size, str);
        } else {
            avio_read(pb, str, str_size);
            str[str_size] = 0;
        }
        // Android MP4 writer put an additional '/' at the end, discard it.
        // The CTS test seems the added '/' is not needed.
        if ((atom.type == MKTAG(0xa9, 'x', 'y', 'z')) && (str[str_size - 1] == 0x2f)) {
            str[str_size - 1] = 0;
        }
        av_dict_set(&c->fc->metadata, key, str, 0);
        if (*language && strcmp(language, "und")) {
            snprintf(key2, sizeof(key2), "%s-%s", key, language);
            av_dict_set(&c->fc->metadata, key2, str, 0);
        }
    }
    av_dlog(c->fc, "lang \"%3s\" ", language);
    av_dlog(c->fc, "tag \"%s\" value \"%s\" atom \"%.4s\" %d %"PRId64"\n",
            key, str, (char*)&atom.type, str_size, atom.size);

    return 0;
}

static int mov_read_chpl(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int64_t start;
    int i, nb_chapters, str_len, version;
    char str[256 + 1];

    if ((atom.size -= 5) < 0) {
        return 0;
    }

    version = avio_r8(pb);
    avio_rb24(pb);
    if (version) {
        avio_rb32(pb);    // ???
    }
    nb_chapters = avio_r8(pb);

    for (i = 0; i < nb_chapters; i++) {
        if (atom.size < 9) {
            return 0;
        }

        start = avio_rb64(pb);
        str_len = avio_r8(pb);

        if ((atom.size -= 9 + str_len) < 0) {
            return 0;
        }

        avio_read(pb, str, str_len);
        str[str_len] = 0;
        ff_new_chapter(c->fc, i, (AVRational) {
            1, 10000000
        }, start, AV_NOPTS_VALUE, str);
    }
    return 0;
}

static int mov_read_default(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int64_t total_size = 0;
    MOVAtom a;
    int i;
    int searchtag = 0;/*found a error tag,changed to search mod*/
    int serachenable = (atom.type == MKTAG('r', 'o', 'o', 't'));

    if (atom.size < 0) {
        atom.size = INT64_MAX;
    }
    while (total_size + 8 < atom.size && !url_feof(pb) && !url_interrupt_cb()) {
        int (*parse)(MOVContext*, AVIOContext*, MOVAtom) = NULL;
        a.size = atom.size;
        a.type = 0;
        if (atom.size >= 8) {
            a.size = avio_rb32(pb);
            a.type = avio_rl32(pb);
        }
        av_dlog(c->fc, "type: %08x '%.4s' parent:'%.4s' sz: %"PRId64" %"PRId64" %"PRId64"\n",
                a.type, (char*)&a.type, (char*)&atom.type, a.size, total_size, atom.size);
        //av_log(c->fc, AV_LOG_INFO, "type: %08x '%.4s' offset=%llx aize=%llx atome.size=%llx total_size=%llx\n", a.type, (char*)&a.type,avio_tell(pb), a.size, atom.size, total_size);
        total_size += 8;
        if (a.size == 1) { /* 64 bit extended size */
            a.size = avio_rb64(pb) - 8;
            total_size += 8;
        }
        if (a.size == 0) {
            a.size = atom.size - total_size;
            if (a.size <= 8) {
                av_log(c->fc, AV_LOG_INFO, "L%d: a.size (%x)<8, break!\n", __LINE__, a.size);
                break;
            }
        }
        a.size -= 8;
        if (!c->found_moov && (a.size > atom.size - total_size)) {
            if (!searchtag) {
                searchtag = serachenable;
                //av_log(c->fc, AV_LOG_INFO, "******set search mode************\n");
            }
            continue;
        }

        if (a.size < 0) {
            av_log(c->fc, AV_LOG_INFO, "L%d: a.size (%x)<8, break!\n", __LINE__, a.size);
            break;
        }

        a.size = FFMIN(a.size, atom.size - total_size);

        for (i = 0; mov_default_parse_table[i].type; i++)
            if (mov_default_parse_table[i].type == a.type) {
                parse = mov_default_parse_table[i].parse;
                if (searchtag) {
                    searchtag = 0;/*exit serach mode*/
                    //av_log(c->fc, AV_LOG_INFO, "******exit search mode************\n");
                }
                break;
            }

        // container is user data
        if (!parse && (atom.type == MKTAG('u', 'd', 't', 'a') ||
                       atom.type == MKTAG('i', 'l', 's', 't'))) {
            parse = mov_read_udta_string;
        }

        if (!parse) { /* skip leaf atoms data */
            if (!searchtag) {
                avio_skip(pb, a.size);
            } else {
                continue;
            }
        } else {
            int64_t start_pos = avio_tell(pb);
            int64_t left;
            int err = parse(c, pb, a);
            if (err < 0) {
                return err;
            }
            if (c->found_moov && c->found_mdat &&
                (!pb->seekable  || pb->is_slowmedia  || pb->is_streamed || start_pos + a.size == avio_size(pb))) {
                return 0;    /*can't seek,slowmedia,streamed all don't do else parser now*/
            }
            left = a.size - avio_tell(pb) + start_pos;
            if (left > 0) { /* skip garbage at atom end */
                avio_skip(pb, left);
            }
        }

        total_size += a.size;
    }

    if (total_size < atom.size && atom.size < 0x7ffff) {
        avio_skip(pb, atom.size - total_size);
    }
    if (url_interrupt_cb()) {
        av_log(NULL, AV_LOG_WARNING, "mov_read_default interrupt, exit\n");
        return AVERROR_EXIT;
    }
    return 0;
}

static int mov_read_dref(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int entries, i, j;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags
    entries = avio_rb32(pb);
    if (entries >= UINT_MAX / sizeof(*sc->drefs)) {
        return -1;
    }
    sc->drefs = av_mallocz(entries * sizeof(*sc->drefs));
    if (!sc->drefs) {
        return AVERROR(ENOMEM);
    }
    sc->drefs_count = entries;

    for (i = 0; i < sc->drefs_count; i++) {
        MOVDref *dref = &sc->drefs[i];
        uint32_t size = avio_rb32(pb);
        int64_t next = avio_tell(pb) + size - 4;

        if (size < 12) {
            return -1;
        }

        dref->type = avio_rl32(pb);
        avio_rb32(pb); // version + flags
        av_dlog(c->fc, "type %.4s size %d\n", (char*)&dref->type, size);

        if (dref->type == MKTAG('a', 'l', 'i', 's') && size > 150) {
            /* macintosh alias record */
            uint16_t volume_len, len;
            int16_t type;

            avio_skip(pb, 10);

            volume_len = avio_r8(pb);
            volume_len = FFMIN(volume_len, 27);
            avio_read(pb, dref->volume, 27);
            dref->volume[volume_len] = 0;
            av_log(c->fc, AV_LOG_DEBUG, "volume %s, len %d\n", dref->volume, volume_len);

            avio_skip(pb, 12);

            len = avio_r8(pb);
            len = FFMIN(len, 63);
            avio_read(pb, dref->filename, 63);
            dref->filename[len] = 0;
            av_log(c->fc, AV_LOG_DEBUG, "filename %s, len %d\n", dref->filename, len);

            avio_skip(pb, 16);

            /* read next level up_from_alias/down_to_target */
            dref->nlvl_from = avio_rb16(pb);
            dref->nlvl_to   = avio_rb16(pb);
            av_log(c->fc, AV_LOG_DEBUG, "nlvl from %d, nlvl to %d\n",
                   dref->nlvl_from, dref->nlvl_to);

            avio_skip(pb, 16);

            for (type = 0; type != -1 && avio_tell(pb) < next;) {
                type = avio_rb16(pb);
                len = avio_rb16(pb);
                av_log(c->fc, AV_LOG_DEBUG, "type %d, len %d\n", type, len);
                if (len & 1) {
                    len += 1;
                }
                if (type == 2) { // absolute path
                    av_free(dref->path);
                    dref->path = av_mallocz(len + 1);
                    if (!dref->path) {
                        return AVERROR(ENOMEM);
                    }
                    avio_read(pb, dref->path, len);
                    if (len > volume_len && !strncmp(dref->path, dref->volume, volume_len)) {
                        len -= volume_len;
                        memmove(dref->path, dref->path + volume_len, len);
                        dref->path[len] = 0;
                    }
                    for (j = 0; j < len; j++)
                        if (dref->path[j] == ':') {
                            dref->path[j] = '/';
                        }
                    av_log(c->fc, AV_LOG_DEBUG, "path %s\n", dref->path);
                } else if (type == 0) { // directory name
                    av_free(dref->dir);
                    dref->dir = av_malloc(len + 1);
                    if (!dref->dir) {
                        return AVERROR(ENOMEM);
                    }
                    avio_read(pb, dref->dir, len);
                    dref->dir[len] = 0;
                    for (j = 0; j < len; j++)
                        if (dref->dir[j] == ':') {
                            dref->dir[j] = '/';
                        }
                    av_log(c->fc, AV_LOG_DEBUG, "dir %s\n", dref->dir);
                } else {
                    avio_skip(pb, len);
                }
            }
        }
        avio_seek(pb, next, SEEK_SET);
    }
    return 0;
}

static int mov_read_hdlr(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint32_t type;
    uint32_t av_unused ctype;

    if (c->fc->nb_streams < 1) { // meta before first trak
        return 0;
    }

    st = c->fc->streams[c->fc->nb_streams - 1];

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    /* component type */
    ctype = avio_rl32(pb);
    type = avio_rl32(pb); /* component subtype */

    av_dlog(c->fc, "ctype= %.4s (0x%08x)\n", (char*)&ctype, ctype);
    av_dlog(c->fc, "stype= %.4s\n", (char*)&type);

    if (type == MKTAG('v', 'i', 'd', 'e')) {
        st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    } else if (type == MKTAG('s', 'o', 'u', 'n')) {
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    } else if (type == MKTAG('m', '1', 'a', ' ')) {
        st->codec->codec_id = CODEC_ID_MP2;
    } else if (type == MKTAG('s', 'u', 'b', 'p')) {
        st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;
    }

    avio_rb32(pb); /* component  manufacture */
    avio_rb32(pb); /* component flags */
    avio_rb32(pb); /* component flags mask */

    return 0;
}

int ff_mov_read_esds(AVFormatContext *fc, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int tag;

    if (fc->nb_streams < 1) {
        return 0;
    }
    st = fc->streams[fc->nb_streams - 1];

    avio_rb32(pb); /* version + flags */
    ff_mp4_read_descr(fc, pb, &tag);
    if (tag == MP4ESDescrTag) {
        avio_rb16(pb); /* ID */
        avio_r8(pb); /* priority */
    } else {
        avio_rb16(pb);    /* ID */
    }

    ff_mp4_read_descr(fc, pb, &tag);
    if (tag == MP4DecConfigDescrTag) {
        ff_mp4_read_dec_config_descr(fc, st, pb);
    }
    return 0;
}

static int mov_read_esds(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    return ff_mov_read_esds(c->fc, pb, atom);
}

static int mov_read_dac3(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int ac3info, acmod, lfeon, bsmod;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    ac3info = avio_rb24(pb);
    bsmod = (ac3info >> 14) & 0x7;
    acmod = (ac3info >> 11) & 0x7;
    lfeon = (ac3info >> 10) & 0x1;
    st->codec->channels = ((int[]) {
        2, 1, 2, 3, 3, 4, 4, 5
    })[acmod] + lfeon;
    st->codec->audio_service_type = bsmod;
    if (st->codec->channels > 1 && bsmod == 0x7) {
        st->codec->audio_service_type = AV_AUDIO_SERVICE_TYPE_KARAOKE;
    }

    return 0;
}

static int mov_read_wfex(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    ff_get_wav_header(pb, st->codec, atom.size);

    return 0;
}

static int mov_read_pasp(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    const int num = avio_rb32(pb);
    const int den = avio_rb32(pb);
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if ((st->sample_aspect_ratio.den != 1 || st->sample_aspect_ratio.num) && // default
        (den != st->sample_aspect_ratio.den || num != st->sample_aspect_ratio.num)) {
        av_log(c->fc, AV_LOG_WARNING,
               "sample aspect ratio already set to %d:%d, ignoring 'pasp' atom (%d:%d)\n",
               st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
               num, den);
    } else if (den != 0) {
        st->sample_aspect_ratio.num = num;
        st->sample_aspect_ratio.den = den;
    }
    return 0;
}

/* this atom contains actual media data */
static int mov_read_mdat(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    if (atom.size == 0) { /* wrong one (MP4) */
        return 0;
    }
    c->found_mdat = 1;
    /*
    * Lujian.Hu 2012-12-14
    * only use media_dataoffset to time_search(player_av.c) can not work correctly, because in mov_read_seek not only modify the pos to
    * media data offset but reset the sample pointer to the correct position according to the very timestamp
    */
    //c->fc->media_dataoffset = url_ftell(pb);
    return 0; /* now go for moov */
}

/* read major brand, minor version and compatible brands and store them as metadata */
static int mov_read_ftyp(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    uint32_t minor_ver;
    int comp_brand_size;
    char minor_ver_str[11]; /* 32 bit integer -> 10 digits + null */
    char* comp_brands_str;
    uint8_t type[5] = {0};

    avio_read(pb, type, 4);
    if (strcmp(type, "qt  ")) {
        c->isom = 1;
    }
    av_log(c->fc, AV_LOG_DEBUG, "ISO: File Type Major Brand: %.4s\n", (char *)&type);
    av_dict_set(&c->fc->metadata, "major_brand", type, 0);
    minor_ver = avio_rb32(pb); /* minor version */
    snprintf(minor_ver_str, sizeof(minor_ver_str), "%d", minor_ver);
    av_dict_set(&c->fc->metadata, "minor_version", minor_ver_str, 0);

    comp_brand_size = atom.size - 8;
    if (comp_brand_size < 0) {
        return -1;
    }
    comp_brands_str = av_malloc(comp_brand_size + 1); /* Add null terminator */
    if (!comp_brands_str) {
        return AVERROR(ENOMEM);
    }
    avio_read(pb, comp_brands_str, comp_brand_size);
    comp_brands_str[comp_brand_size] = 0;
    av_dict_set(&c->fc->metadata, "compatible_brands", comp_brands_str, 0);
    av_freep(&comp_brands_str);

    return 0;
}

/* this atom should contain all header atoms */
static int mov_read_moov(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    if (mov_read_default(c, pb, atom) < 0) {
        return -1;
    }
    /* we parsed the 'moov' atom, we can terminate the parsing as soon as we find the 'mdat' */
    /* so we don't parse the whole file if over a network */
    c->found_moov = 1;
    return 0; /* now go for mdat */
}

static int mov_read_moof(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    c->fragment.moof_offset = avio_tell(pb) - 8;
    av_dlog(c->fc, "moof offset %"PRIx64"\n", c->fragment.moof_offset);
    return mov_read_default(c, pb, atom);
}

static void mov_metadata_creation_time(AVDictionary **metadata, time_t time)
{
    char buffer[32];
    if (time) {
        struct tm *ptm;
        time -= 2082844800;  /* seconds between 1904-01-01 and Epoch */
        ptm = gmtime(&time);
        if (!ptm) {
            return;
        }
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);
        av_dict_set(metadata, "creation_time", buffer, 0);
    }
}

static int mov_read_mdhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int version;
    char language[4] = {0};
    unsigned lang;
    time_t creation_time;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    version = avio_r8(pb);
    if (version > 1) {
        return -1;    /* unsupported */
    }

    avio_rb24(pb); /* flags */
    if (version == 1) {
        creation_time = avio_rb64(pb);
        avio_rb64(pb);
    } else {
        creation_time = avio_rb32(pb);
        avio_rb32(pb); /* modification time */
    }
    mov_metadata_creation_time(&st->metadata, creation_time);

    sc->time_scale = avio_rb32(pb);
    st->duration = (version == 1) ? avio_rb64(pb) : avio_rb32(pb); /* duration */

    lang = avio_rb16(pb); /* language */
    if (ff_mov_lang_to_iso639(lang, language)) {
        av_dict_set(&st->metadata, "language", language, 0);
    }
    avio_rb16(pb); /* quality */

    return 0;
}

static int mov_read_mvhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    time_t creation_time;
    int version = avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    if (version == 1) {
        creation_time = avio_rb64(pb);
        avio_rb64(pb);
    } else {
        creation_time = avio_rb32(pb);
        avio_rb32(pb); /* modification time */
    }
    mov_metadata_creation_time(&c->fc->metadata, creation_time);
    c->time_scale = avio_rb32(pb); /* time scale */

    av_dlog(c->fc, "time scale = %i\n", c->time_scale);

    c->duration = (version == 1) ? avio_rb64(pb) : avio_rb32(pb); /* duration */
    avio_rb32(pb); /* preferred scale */

    avio_rb16(pb); /* preferred volume */

    avio_skip(pb, 10); /* reserved */

    avio_skip(pb, 36); /* display matrix */

    avio_rb32(pb); /* preview time */
    avio_rb32(pb); /* preview duration */
    avio_rb32(pb); /* poster time */
    avio_rb32(pb); /* selection time */
    avio_rb32(pb); /* selection duration */
    avio_rb32(pb); /* current time */
    avio_rb32(pb); /* next track ID */

    return 0;
}

static int mov_read_smi(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if ((uint64_t)atom.size > (1 << 30)) {
        return -1;
    }

    // currently SVQ3 decoder expect full STSD header - so let's fake it
    // this should be fixed and just SMI header should be passed
    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size + 0x5a + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata) {
        return AVERROR(ENOMEM);
    }
    st->codec->extradata_size = 0x5a + atom.size;
    memcpy(st->codec->extradata, "SVQ3", 4); // fake
    avio_read(pb, st->codec->extradata + 0x5a, atom.size);
    av_dlog(c->fc, "Reading SMI %"PRId64"  %s\n", atom.size, st->codec->extradata + 0x5a);
    return 0;
}

static int mov_read_enda(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int little_endian;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    little_endian = avio_rb16(pb) & 0xFF;
    av_dlog(c->fc, "enda %d\n", little_endian);
    if (little_endian == 1) {
        switch (st->codec->codec_id) {
        case CODEC_ID_PCM_S24BE:
            st->codec->codec_id = CODEC_ID_PCM_S24LE;
            break;
        case CODEC_ID_PCM_S32BE:
            st->codec->codec_id = CODEC_ID_PCM_S32LE;
            break;
        case CODEC_ID_PCM_F32BE:
            st->codec->codec_id = CODEC_ID_PCM_F32LE;
            break;
        case CODEC_ID_PCM_F64BE:
            st->codec->codec_id = CODEC_ID_PCM_F64LE;
            break;
        default:
            break;
        }
    }
    return 0;
}

/* FIXME modify qdm2/svq3/h264 decoders to take full atom as extradata */
static int mov_read_extradata(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint64_t size;
    uint8_t *buf;

    if (c->fc->nb_streams < 1) { // will happen with jp2 files
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    size = (uint64_t)st->codec->extradata_size + atom.size + 8 + FF_INPUT_BUFFER_PADDING_SIZE;
    if (size > INT_MAX || (uint64_t)atom.size > INT_MAX) {
        return -1;
    }
    buf = av_realloc(st->codec->extradata, size);
    if (!buf) {
        return -1;
    }
    st->codec->extradata = buf;
    buf += st->codec->extradata_size;
    st->codec->extradata_size = size - FF_INPUT_BUFFER_PADDING_SIZE;
    AV_WB32(buf    , atom.size + 8);
    AV_WL32(buf + 4, atom.type);
    avio_read(pb, buf + 8, atom.size);
    return 0;
}

static int mov_read_wave(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if ((uint64_t)atom.size > (1 << 30)) {
        return -1;
    }

    if (st->codec->codec_id == CODEC_ID_QDM2 || st->codec->codec_id == CODEC_ID_QDMC) {
        // pass all frma atom to codec, needed at least for QDMC and QDM2
        av_free(st->codec->extradata);
        st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
        if (!st->codec->extradata) {
            return AVERROR(ENOMEM);
        }
        st->codec->extradata_size = atom.size;
        avio_read(pb, st->codec->extradata, atom.size);
    } else if (atom.size > 8) { /* to read frma, esds atoms */
        if (mov_read_default(c, pb, atom) < 0) {
            return -1;
        }
    } else {
        avio_skip(pb, atom.size);
    }
    return 0;
}

/**
 * This function reads atom content and puts data in extradata without tag
 * nor size unlike mov_read_extradata.
 */
static int mov_read_glbl(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if ((uint64_t)atom.size > (1 << 30)) {
        return -1;
    }

    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata) {
        return AVERROR(ENOMEM);
    }
    st->codec->extradata_size = atom.size;
    avio_read(pb, st->codec->extradata, atom.size);
    return 0;
}

static int mov_read_dvc1(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint8_t profile_level;
    int ret;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if (atom.size >= (1 << 28) || atom.size < 7) {
        return AVERROR_INVALIDDATA;
    }

    profile_level = avio_r8(pb);
    if (profile_level & 0xf0 != 0xc0) {
        return 0;
    }

    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size - 7 + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata) {
        return AVERROR(ENOMEM);
    }
    st->codec->extradata_size = atom.size - 7;
    avio_seek(pb, 6, SEEK_CUR);
    ret = avio_read(pb, st->codec->extradata, st->codec->extradata_size);
    if (ret != st->codec->extradata_size) {
        return ret < 0 ? ret : AVERROR_INVALIDDATA;
    }

    return 0;
}

/**
 * An mvcC atom contains the necessary information about the multiview data for
 * 3D content. This function reads atom content and puts data in extradata.
 */
static int mov_read_mvcC(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    st->codec->codec_id = CODEC_ID_H264MVC;

    return mov_read_glbl(c, pb, atom);
}

/**
 * An strf atom is a BITMAPINFOHEADER struct. This struct is 40 bytes itself,
 * but can have extradata appended at the end after the 40 bytes belonging
 * to the struct.
 */
static int mov_read_strf(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    if (atom.size <= 40) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];

    if ((uint64_t)atom.size > (1 << 30)) {
        return -1;
    }

    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size - 40 + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata) {
        return AVERROR(ENOMEM);
    }
    st->codec->extradata_size = atom.size - 40;
    avio_skip(pb, 40);
    avio_read(pb, st->codec->extradata, atom.size - 40);
    return 0;
}

static int mov_read_stco(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    if (entries >= UINT_MAX / sizeof(int64_t)) {
        return -1;
    }

    sc->chunk_offsets = av_malloc(entries * sizeof(int64_t));
    if (!sc->chunk_offsets) {
        return AVERROR(ENOMEM);
    }
    sc->chunk_count = entries;

    if (atom.type == MKTAG('s', 't', 'c', 'o'))
        for (i = 0; i < entries; i++) {
            sc->chunk_offsets[i] = avio_rb32(pb);
        }
    else if (atom.type == MKTAG('c', 'o', '6', '4'))
        for (i = 0; i < entries; i++) {
            sc->chunk_offsets[i] = avio_rb64(pb);
        }
    else {
        return -1;
    }

    return 0;
}

/**
 * Compute codec id for 'lpcm' tag.
 * See CoreAudioTypes and AudioStreamBasicDescription at Apple.
 */
enum CodecID ff_mov_get_lpcm_codec_id(int bps, int flags)
{
    if (flags & 1) { // floating point
        if (flags & 2) { // big endian
            if (bps == 32) {
                return CODEC_ID_PCM_F32BE;
            } else if (bps == 64) {
                return CODEC_ID_PCM_F64BE;
            }
        } else {
            if (bps == 32) {
                return CODEC_ID_PCM_F32LE;
            } else if (bps == 64) {
                return CODEC_ID_PCM_F64LE;
            }
        }
    } else {
        if (flags & 2) {
            if (bps == 8)
                // signed integer
                if (flags & 4) {
                    return CODEC_ID_PCM_S8;
                } else {
                    return CODEC_ID_PCM_U8;
                }
            else if (bps == 16) {
                return CODEC_ID_PCM_S16BE;
            } else if (bps == 24) {
                return CODEC_ID_PCM_S24BE;
            } else if (bps == 32) {
                return CODEC_ID_PCM_S32BE;
            }
        } else {
            if (bps == 8)
                if (flags & 4) {
                    return CODEC_ID_PCM_S8;
                } else {
                    return CODEC_ID_PCM_U8;
                }
            else if (bps == 16) {
                return CODEC_ID_PCM_S16LE;
            } else if (bps == 24) {
                return CODEC_ID_PCM_S24LE;
            } else if (bps == 32) {
                return CODEC_ID_PCM_S32LE;
            }
        }
    }
    return CODEC_ID_NONE;
}

int ff_mov_read_stsd_entries(MOVContext *c, AVIOContext *pb, int entries)
{
    AVStream *st;
    MOVStreamContext *sc;
    int j, pseudo_stream_id;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    for (pseudo_stream_id = 0; pseudo_stream_id < entries; pseudo_stream_id++) {
        //Parsing Sample description table
        enum CodecID id;
        int dref_id = 1;
        MOVAtom a = { AV_RL32("stsd") };
        int64_t start_pos = avio_tell(pb);
        int size = avio_rb32(pb); /* size */
        uint32_t format = avio_rl32(pb); /* data format */

        if (size >= 16) {
            avio_rb32(pb); /* reserved */
            avio_rb16(pb); /* reserved */
            dref_id = avio_rb16(pb);
        }

        if (st->codec->codec_tag &&
            st->codec->codec_tag != format &&
            (c->fc->video_codec_id ? ff_codec_get_id(codec_movvideo_tags, format) != c->fc->video_codec_id
             : st->codec->codec_tag != MKTAG('j', 'p', 'e', 'g'))
           ) {
            /* Multiple fourcc, we skip JPEG. This is not correct, we should
             * export it as a separate AVStream but this needs a few changes
             * in the MOV demuxer, patch welcome. */
multiple_stsd:
            av_log(c->fc, AV_LOG_WARNING, "multiple fourcc not supported\n");
            avio_skip(pb, size - (avio_tell(pb) - start_pos));
            continue;
        }
        /* we cannot demux concatenated h264 streams because of different extradata */
        if (st->codec->codec_tag && (st->codec->codec_tag == AV_RL32("avc1")
                                     || st->codec->codec_tag == AV_RL32("hvc1")
                                     || st->codec->codec_tag == AV_RL32("hev1"))) {
            goto multiple_stsd;
        }
        sc->pseudo_stream_id = st->codec->codec_tag ? -1 : pseudo_stream_id;
        sc->dref_id = dref_id;

        st->codec->codec_tag = format;
        id = ff_codec_get_id(codec_movaudio_tags, format);
        if (id <= 0 && ((format & 0xFFFF) == 'm' + ('s' << 8) || (format & 0xFFFF) == 'T' + ('S' << 8))) {
            id = ff_codec_get_id(ff_codec_wav_tags, av_bswap32(format) & 0xFFFF);
        }

        if (st->codec->codec_type != AVMEDIA_TYPE_VIDEO && id > 0) {
            st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        } else if (st->codec->codec_type != AVMEDIA_TYPE_AUDIO && /* do not overwrite codec type */
                   format && format != MKTAG('m', 'p', '4', 's')) { /* skip old asf mpeg4 tag */
            id = ff_codec_get_id(codec_movvideo_tags, format);
            if (id <= 0) {
                id = ff_codec_get_id(ff_codec_bmp_tags, format);
            }
            if (id > 0) {
                st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
            } else if (st->codec->codec_type == AVMEDIA_TYPE_DATA) {
                id = ff_codec_get_id(ff_codec_movsubtitle_tags, format);
                if (id > 0) {
                    st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;
                }
            }
        }

        av_dlog(c->fc, "size=%d 4CC= %c%c%c%c codec_type=%d\n", size,
                (format >> 0) & 0xff, (format >> 8) & 0xff, (format >> 16) & 0xff,
                (format >> 24) & 0xff, st->codec->codec_type);

        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            unsigned int color_depth, len;
            int color_greyscale;

            if ((id != CODEC_ID_H264) || (st->codec->codec_id != CODEC_ID_H264MVC)) {
                st->codec->codec_id = id;
            } else {
                st->codec->codec_tag = CODEC_ID_H264MVC;
                av_dlog(c->fc, "code_tag set to MVC with mvcC tag\n");
            }
            avio_rb16(pb); /* version */
            avio_rb16(pb); /* revision level */
            avio_rb32(pb); /* vendor */
            avio_rb32(pb); /* temporal quality */
            avio_rb32(pb); /* spatial quality */

            st->codec->width = avio_rb16(pb); /* width */
            st->codec->height = avio_rb16(pb); /* height */

            avio_rb32(pb); /* horiz resolution */
            avio_rb32(pb); /* vert resolution */
            avio_rb32(pb); /* data size, always 0 */
            avio_rb16(pb); /* frames per samples */

            len = avio_r8(pb); /* codec name, pascal string */
            if (len > 31) {
                len = 31;
            }
            mov_read_mac_string(c, pb, len, st->codec->codec_name, 32);
            if (len < 31) {
                avio_skip(pb, 31 - len);
            }
            /* codec_tag YV12 triggers an UV swap in rawdec.c */
            if (!memcmp(st->codec->codec_name, "Planar Y'CbCr 8-bit 4:2:0", 25)) {
                st->codec->codec_tag = MKTAG('I', '4', '2', '0');
            }

            st->codec->bits_per_coded_sample = avio_rb16(pb); /* depth */
            st->codec->color_table_id = avio_rb16(pb); /* colortable id */
            av_dlog(c->fc, "depth %d, ctab id %d\n",
                    st->codec->bits_per_coded_sample, st->codec->color_table_id);
            /* figure out the palette situation */
            color_depth = st->codec->bits_per_coded_sample & 0x1F;
            color_greyscale = st->codec->bits_per_coded_sample & 0x20;

            /* if the depth is 2, 4, or 8 bpp, file is palettized */
            if ((color_depth == 2) || (color_depth == 4) ||
                (color_depth == 8)) {
                /* for palette traversal */
                unsigned int color_start, color_count, color_end;
                unsigned char r, g, b;

                if (color_greyscale) {
                    int color_index, color_dec;
                    /* compute the greyscale palette */
                    st->codec->bits_per_coded_sample = color_depth;
                    color_count = 1 << color_depth;
                    color_index = 255;
                    color_dec = 256 / (color_count - 1);
                    for (j = 0; j < color_count; j++) {
                        r = g = b = color_index;
                        sc->palette[j] =
                            (r << 16) | (g << 8) | (b);
                        color_index -= color_dec;
                        if (color_index < 0) {
                            color_index = 0;
                        }
                    }
                } else if (st->codec->color_table_id) {
                    const uint8_t *color_table;
                    /* if flag bit 3 is set, use the default palette */
                    color_count = 1 << color_depth;
                    if (color_depth == 2) {
                        color_table = ff_qt_default_palette_4;
                    } else if (color_depth == 4) {
                        color_table = ff_qt_default_palette_16;
                    } else {
                        color_table = ff_qt_default_palette_256;
                    }

                    for (j = 0; j < color_count; j++) {
                        r = color_table[j * 3 + 0];
                        g = color_table[j * 3 + 1];
                        b = color_table[j * 3 + 2];
                        sc->palette[j] =
                            (r << 16) | (g << 8) | (b);
                    }
                } else {
                    /* load the palette from the file */
                    color_start = avio_rb32(pb);
                    color_count = avio_rb16(pb);
                    color_end = avio_rb16(pb);
                    if ((color_start <= 255) &&
                        (color_end <= 255)) {
                        for (j = color_start; j <= color_end; j++) {
                            /* each R, G, or B component is 16 bits;
                             * only use the top 8 bits; skip alpha bytes
                             * up front */
                            avio_r8(pb);
                            avio_r8(pb);
                            r = avio_r8(pb);
                            avio_r8(pb);
                            g = avio_r8(pb);
                            avio_r8(pb);
                            b = avio_r8(pb);
                            avio_r8(pb);
                            sc->palette[j] =
                                (r << 16) | (g << 8) | (b);
                        }
                    }
                }
                sc->has_palette = 1;
            }
        } else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            int bits_per_sample, flags;
            uint16_t version = avio_rb16(pb);

            st->codec->codec_id = id;
            avio_rb16(pb); /* revision level */
            avio_rb32(pb); /* vendor */

            st->codec->channels = avio_rb16(pb);             /* channel count */
            av_dlog(c->fc, "audio channels %d\n", st->codec->channels);
            st->codec->bits_per_coded_sample = avio_rb16(pb);      /* sample size */

            sc->audio_cid = avio_rb16(pb);
            avio_rb16(pb); /* packet size = 0 */

            st->codec->sample_rate = ((avio_rb32(pb) >> 16));

            //Read QT version 1 fields. In version 0 these do not exist.
            av_dlog(c->fc, "version =%d, isom =%d\n", version, c->isom);
            if (!c->isom) {
                if (version == 1) {
                    sc->samples_per_frame = avio_rb32(pb);
                    avio_rb32(pb); /* bytes per packet */
                    sc->bytes_per_frame = avio_rb32(pb);
                    avio_rb32(pb); /* bytes per sample */
                } else if (version == 2) {
                    avio_rb32(pb); /* sizeof struct only */
                    st->codec->sample_rate = av_int2dbl(avio_rb64(pb)); /* float 64 */
                    st->codec->channels = avio_rb32(pb);
                    avio_rb32(pb); /* always 0x7F000000 */
                    st->codec->bits_per_coded_sample = avio_rb32(pb); /* bits per channel if sound is uncompressed */
                    flags = avio_rb32(pb); /* lpcm format specific flag */
                    sc->bytes_per_frame = avio_rb32(pb); /* bytes per audio packet if constant */
                    sc->samples_per_frame = avio_rb32(pb); /* lpcm frames per audio packet if constant */
                    if (format == MKTAG('l', 'p', 'c', 'm')) {
                        st->codec->codec_id = ff_mov_get_lpcm_codec_id(st->codec->bits_per_coded_sample, flags);
                    }
                }
            }

            switch (st->codec->codec_id) {
            case CODEC_ID_PCM_S8:
            case CODEC_ID_PCM_U8:
                if (st->codec->bits_per_coded_sample == 16) {
                    st->codec->codec_id = CODEC_ID_PCM_S16BE;
                }
                break;
            case CODEC_ID_PCM_S16LE:
            case CODEC_ID_PCM_S16BE:
                if (st->codec->bits_per_coded_sample == 8) {
                    st->codec->codec_id = CODEC_ID_PCM_S8;
                } else if (st->codec->bits_per_coded_sample == 24)
                    st->codec->codec_id =
                        st->codec->codec_id == CODEC_ID_PCM_S16BE ?
                        CODEC_ID_PCM_S24BE : CODEC_ID_PCM_S24LE;
                break;
            /* set values for old format before stsd version 1 appeared */
            case CODEC_ID_MACE3:
                sc->samples_per_frame = 6;
                sc->bytes_per_frame = 2 * st->codec->channels;
                break;
            case CODEC_ID_MACE6:
                sc->samples_per_frame = 6;
                sc->bytes_per_frame = 1 * st->codec->channels;
                break;
            case CODEC_ID_ADPCM_IMA_QT:
                sc->samples_per_frame = 64;
                sc->bytes_per_frame = 34 * st->codec->channels;
                break;
            case CODEC_ID_GSM:
                sc->samples_per_frame = 160;
                sc->bytes_per_frame = 33;
                break;
            default:
                break;
            }

            bits_per_sample = av_get_bits_per_sample(st->codec->codec_id);
            if (bits_per_sample) {
                st->codec->bits_per_coded_sample = bits_per_sample;
                sc->sample_size = (bits_per_sample >> 3) * st->codec->channels;
            }
        } else if (st->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            // ttxt stsd contains display flags, justification, background
            // color, fonts, and default styles, so fake an atom to read it
            MOVAtom fake_atom = { .size = size - (avio_tell(pb) - start_pos) };
            if (format != AV_RL32("mp4s")) { // mp4s contains a regular esds atom
                mov_read_glbl(c, pb, fake_atom);
            }
            st->codec->codec_id = id;
            st->codec->width = sc->width;
            st->codec->height = sc->height;
        } else {
            /* other codec type, just skip (rtp, mp4s, tmcd ...) */
            avio_skip(pb, size - (avio_tell(pb) - start_pos));
        }
        /* this will read extra atoms at the end (wave, alac, damr, avcC, SMI ...) */
        a.size = size - (avio_tell(pb) - start_pos);
        if (a.size > 8) {
            if (mov_read_default(c, pb, a) < 0) {
                return -1;
            }
        } else if (a.size > 0) {
            avio_skip(pb, a.size);
        }
    }

    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO && st->codec->sample_rate == 0 && sc->time_scale > 1) {
        st->codec->sample_rate = sc->time_scale;
    }

    /* special codec parameters handling */
    switch (st->codec->codec_id) {
#if CONFIG_DV_DEMUXER
    case CODEC_ID_DVAUDIO:
        c->dv_fctx = avformat_alloc_context();
        c->dv_demux = dv_init_demux(c->dv_fctx);
        if (!c->dv_demux) {
            av_log(c->fc, AV_LOG_ERROR, "dv demux context init error\n");
            return -1;
        }
        sc->dv_audio_container = 1;
        st->codec->codec_id = CODEC_ID_PCM_S16LE;
        break;
#endif
    /* no ifdef since parameters are always those */
    case CODEC_ID_QCELP:
        // force sample rate for qcelp when not stored in mov
        if (st->codec->codec_tag != MKTAG('Q', 'c', 'l', 'p')) {
            st->codec->sample_rate = 8000;
        }
        st->codec->frame_size = 160;
        st->codec->channels = 1; /* really needed */
        break;
    case CODEC_ID_AMR_NB:
    case CODEC_ID_AMR_WB:
        st->codec->frame_size = sc->samples_per_frame;
        st->codec->channels = 1; /* really needed */
        /* force sample rate for amr, stsd in 3gp does not store sample rate */
        if (st->codec->codec_id == CODEC_ID_AMR_NB) {
            st->codec->sample_rate = 8000;
        } else if (st->codec->codec_id == CODEC_ID_AMR_WB) {
            st->codec->sample_rate = 16000;
        }
        break;
    case CODEC_ID_MP2:
    case CODEC_ID_MP3:
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO; /* force type after stsd for m1a hdlr */
        st->need_parsing = AVSTREAM_PARSE_FULL;
        break;
    case CODEC_ID_GSM:
    case CODEC_ID_ADPCM_MS:
    case CODEC_ID_ADPCM_IMA_WAV:
        st->codec->frame_size = sc->samples_per_frame;
        st->codec->block_align = sc->bytes_per_frame;
        break;
    case CODEC_ID_ALAC:
        if (st->codec->extradata_size == 36) {
            st->codec->frame_size = AV_RB32(st->codec->extradata + 12);
            st->codec->channels   = AV_RB8(st->codec->extradata + 21);
            st->codec->sample_rate = AV_RB32(st->codec->extradata + 32);
        }
        break;
    case CODEC_ID_HEVC:
        st->need_parsing = AVSTREAM_PARSE_HEADERS;
        break;
    default:
        break;
    }

    return 0;
}

static int mov_read_stsd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int entries;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    return ff_mov_read_stsd_entries(c, pb, entries);
}

static int mov_read_stsc(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].stsc.entries = %i\n", c->fc->nb_streams - 1, entries);

    if (entries >= UINT_MAX / sizeof(*sc->stsc_data)) {
        return -1;
    }
    sc->stsc_data = av_malloc(entries * sizeof(*sc->stsc_data));
    if (!sc->stsc_data) {
        return AVERROR(ENOMEM);
    }
    sc->stsc_count = entries;

    for (i = 0; i < entries; i++) {
        sc->stsc_data[i].first = avio_rb32(pb);
        sc->stsc_data[i].count = avio_rb32(pb);
        sc->stsc_data[i].id = avio_rb32(pb);
    }
    return 0;
}

static int mov_read_stps(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned i, entries;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags

    entries = avio_rb32(pb);
    if (entries >= UINT_MAX / sizeof(*sc->stps_data)) {
        return -1;
    }
    sc->stps_data = av_malloc(entries * sizeof(*sc->stps_data));
    if (!sc->stps_data) {
        return AVERROR(ENOMEM);
    }
    sc->stps_count = entries;

    for (i = 0; i < entries; i++) {
        sc->stps_data[i] = avio_rb32(pb);
        //av_dlog(c->fc, "stps %d\n", sc->stps_data[i]);
    }

    return 0;
}

static int mov_read_stss(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    av_dlog(c->fc, "keyframe_count = %d\n", entries);

    if (entries >= UINT_MAX / sizeof(int)) {
        return -1;
    }
    sc->keyframes = av_malloc(entries * sizeof(int));
    if (!sc->keyframes) {
        return AVERROR(ENOMEM);
    }
    sc->keyframe_count = entries;
    // add for mp4/mov file seek
    if (st->index == CODEC_TYPE_VIDEO) {
        st->video_keyframe_count = entries;
    }
    av_log(NULL, AV_LOG_INFO, "%s,st->video_keyframe_count=%d\n", __func__, st->video_keyframe_count);
    for (i = 0; i < entries; i++) {
        sc->keyframes[i] = avio_rb32(pb);
        //av_dlog(c->fc, "keyframes[]=%d\n", sc->keyframes[i]);
    }
    return 0;
}

static int mov_read_stsz(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries, sample_size, field_size, num_bytes;
    GetBitContext gb;
    unsigned char* buf;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    if (atom.type == MKTAG('s', 't', 's', 'z')) {
        sample_size = avio_rb32(pb);
        if (!sc->sample_size) { /* do not overwrite value computed in stsd */
            sc->sample_size = sample_size;
        }
        field_size = 32;
    } else {
        sample_size = 0;
        avio_rb24(pb); /* reserved */
        field_size = avio_r8(pb);
    }
    entries = avio_rb32(pb);

    av_dlog(c->fc, "sample_size = %d sample_count = %d\n", sc->sample_size, entries);

    sc->sample_count = entries;
    if (sample_size) {
        return 0;
    }

    if (field_size != 4 && field_size != 8 && field_size != 16 && field_size != 32) {
        av_log(c->fc, AV_LOG_ERROR, "Invalid sample field size %d\n", field_size);
        return -1;
    }

    if (entries >= UINT_MAX / sizeof(int) || entries >= (UINT_MAX - 4) / field_size) {
        return -1;
    }
    sc->sample_sizes = av_malloc(entries * sizeof(int));
    if (!sc->sample_sizes) {
        return AVERROR(ENOMEM);
    }

    num_bytes = (entries * field_size + 4) >> 3;

    buf = av_malloc(num_bytes + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!buf) {
        av_freep(&sc->sample_sizes);
        return AVERROR(ENOMEM);
    }

    if (avio_read(pb, buf, num_bytes) < num_bytes) {
        av_freep(&sc->sample_sizes);
        av_free(buf);
        //return -1;
        return 0;//*don't retrun -1,if only read end*/
    }

    init_get_bits(&gb, buf, 8 * num_bytes);

    for (i = 0; i < entries; i++) {
        sc->sample_sizes[i] = get_bits_long(&gb, field_size);
    }

    av_free(buf);
    return 0;
}

static int mov_read_stts(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;
    int64_t duration = 0;
    int64_t total_sample_count = 0;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].stts.entries = %i\n", c->fc->nb_streams - 1, entries);

    if (entries >= UINT_MAX / sizeof(*sc->stts_data)) {
        return -1;
    }
    sc->stts_data = av_malloc(entries * sizeof(*sc->stts_data));
    if (!sc->stts_data) {
        return AVERROR(ENOMEM);
    }
    sc->stts_count = entries;

    for (i = 0; i < entries; i++) {
        int sample_duration;
        int sample_count;

        sample_count = avio_rb32(pb);
        sample_duration = avio_rb32(pb);
        /* sample_duration < 0 is invalid based on the spec */
        if (sample_duration < 0) {
            av_log(c->fc, AV_LOG_ERROR, "Invalid SampleDelta %d in STTS, at %d st:%d\n",
                   sample_duration, i, c->fc->nb_streams - 1);
            sample_duration = 1;
        }
        if (sample_count < 0) {
            av_log(c->fc, AV_LOG_ERROR, "Invalid sample_count=%d\n", sample_count);
            return AVERROR_INVALIDDATA;
        }

        sc->stts_data[i].count = sample_count;
        sc->stts_data[i].duration = sample_duration;

        av_dlog(c->fc, "sample_count=%d, sample_duration=%d\n", sample_count, sample_duration);

        duration += (int64_t)sample_duration * sample_count;
        total_sample_count += sample_count;

        if (url_interrupt_cb()) {
            av_log(NULL, AV_LOG_WARNING, "mov_read_stts interrupt, exit\n");
            return AVERROR_EXIT;
        }
    }

    if (pb->eof_reached) {
        return AVERROR_EOF;
    }

    st->nb_frames = total_sample_count;
    if (duration) {
        st->duration = duration;
    }
    sc->track_end = duration;
    return 0;
}

static void mov_update_dts_shift(MOVStreamContext *sc, int duration)
{
    if (duration < 0) {
        sc->dts_shift = FFMAX(sc->dts_shift, -duration);
    }
}

static int mov_read_ctts(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].ctts.entries = %i\n", c->fc->nb_streams - 1, entries);

    if (entries >= UINT_MAX / sizeof(*sc->ctts_data)) {
        return -1;
    }
    sc->ctts_data = av_malloc(entries * sizeof(*sc->ctts_data));
    if (!sc->ctts_data) {
        return AVERROR(ENOMEM);
    }
    sc->ctts_count = entries;
    int negative_count = 0;
    int positive_count = 0;

    for (i = 0; i < entries; i++) {
        int count    = avio_rb32(pb);
        int duration = avio_rb32(pb);

        sc->ctts_data[i].count   = count;
        sc->ctts_data[i].duration = duration;

        if (duration > 0) {
            positive_count++;
        }
        if (duration < 0) {
            negative_count++;
        }

        if (FFABS(duration) > (1 << 28) && i + 2 < entries) {
            av_log(c->fc, AV_LOG_WARNING, "CTTS invalid\n");
            av_freep(&sc->ctts_data);
            sc->ctts_count = 0;
            return 0;
        }

        if (duration < 0 && i + 2 < entries) {
            sc->dts_shift = FFMAX(sc->dts_shift, -duration);
        }
    }

    /*
    * PTS = DTS (from stts) + CTS (from ctts)
    *
    * According to MPEG-4 ISO standard, CTS can never be negative, but quicktime allows
    * negative value. The CTS must be all positive or negative in that case, otherwise ignore the dts_shift.
    */
    if (positive_count > negative_count) {
        sc->dts_shift = 0;
    }

    av_dlog(c->fc, "dts shift %d\n", sc->dts_shift);

    return 0;
}
static int mov_read_sbgp(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;
    uint8_t version;
    uint32_t grouping_type;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    version = avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    grouping_type = avio_rl32(pb);
    if (grouping_type != MKTAG('r', 'a', 'p', ' ')) {
        return 0;    /* only support 'rap ' grouping */
    }
    if (version == 1) {
        avio_rb32(pb);    /* grouping_type_parameter */
    }

    entries = avio_rb32(pb);
    if (!entries) {
        return 0;
    }
    if (sc->rap_group) {
        av_log(c->fc, AV_LOG_WARNING, "Duplicated SBGP atom\n");
    }
    av_free(sc->rap_group);
    sc->rap_group_count = 0;
    sc->rap_group = av_malloc_array(entries, sizeof(*sc->rap_group));
    if (!sc->rap_group) {
        return AVERROR(ENOMEM);
    }

    for (i = 0; i < entries && !pb->eof_reached; i++) {
        sc->rap_group[i].count = avio_rb32(pb); /* sample_count */
        sc->rap_group[i].index = avio_rb32(pb); /* group_description_index */
    }

    sc->rap_group_count = i;

    return pb->eof_reached ? AVERROR_EOF : 0;
}

static void mov_build_index(MOVContext *mov, AVStream *st)
{
    MOVStreamContext *sc = st->priv_data;
    int64_t current_offset;
    int64_t current_dts = 0;
    unsigned int stts_index = 0;
    unsigned int stsc_index = 0;
    unsigned int stss_index = 0;
    unsigned int stps_index = 0;
    unsigned int i, j;
    uint64_t stream_size = 0;

    /* adjust first dts according to edit list */
    if (sc->time_offset && mov->time_scale > 0) {
        if (sc->time_offset < 0) {
            sc->time_offset = av_rescale(sc->time_offset, sc->time_scale, mov->time_scale);
        }
        current_dts = -sc->time_offset;
        if (sc->ctts_data && sc->stts_data &&
            sc->ctts_data[0].duration / FFMAX(sc->stts_data[0].duration, 1) > 16) {
            /* more than 16 frames delay, dts are likely wrong
               this happens with files created by iMovie */
            sc->wrong_dts = 1;
            st->codec->has_b_frames = 1;
        }
    }

    /* only use old uncompressed audio chunk demuxing when stts specifies it */
    if (!(st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
          sc->stts_count == 1 && sc->stts_data[0].duration == 1)) {
        unsigned int current_sample = 0;
        unsigned int stts_sample = 0;
        unsigned int sample_size;
        unsigned int distance = 0;
        unsigned int rap_group_index = 0;
        unsigned int rap_group_sample = 0;
        int rap_group_present = sc->rap_group_count && sc->rap_group;
        int key_off = (sc->keyframe_count && sc->keyframes[0] > 0) || (sc->stps_count && sc->stps_data[0] > 0);

        current_dts -= sc->dts_shift;

        if (sc->sample_count >= UINT_MAX / sizeof(*st->index_entries)) {
            return;
        }
        st->index_entries = av_malloc(sc->sample_count * sizeof(*st->index_entries));
        if (!st->index_entries) {
            return;
        }
        st->index_entries_allocated_size = sc->sample_count * sizeof(*st->index_entries);

        for (i = 0; i < sc->chunk_count; i++) {
            current_offset = sc->chunk_offsets[i];
            while (stsc_index + 1 < sc->stsc_count &&
                   i + 1 == sc->stsc_data[stsc_index + 1].first) {
                stsc_index++;
            }
            for (j = 0; j < sc->stsc_data[stsc_index].count; j++) {
                int keyframe = 0;
                if (current_sample >= sc->sample_count) {
                    av_log(mov->fc, AV_LOG_ERROR, "wrong sample count\n");
                    return;
                }

                if (!sc->keyframe_count || current_sample + key_off == sc->keyframes[stss_index]) {
                    keyframe = 1;
                    if (stss_index + 1 < sc->keyframe_count) {
                        stss_index++;
                    }
                } else if (sc->stps_count && current_sample + key_off == sc->stps_data[stps_index]) {
                    keyframe = 1;
                    if (stps_index + 1 < sc->stps_count) {
                        stps_index++;
                    }
                }
                if (rap_group_present && rap_group_index < sc->rap_group_count) {
                    if (sc->rap_group[rap_group_index].index > 0) {
                        keyframe = 1;
                    }
                    if (++rap_group_sample == sc->rap_group[rap_group_index].count) {
                        rap_group_sample = 0;
                        rap_group_index++;
                    }
                }

                if (keyframe) {
                    distance = 0;
                }
                sample_size = sc->sample_size > 0 ? sc->sample_size : sc->sample_sizes[current_sample];
                if (sc->pseudo_stream_id == -1 ||
                    sc->stsc_data[stsc_index].id - 1 == sc->pseudo_stream_id ||
                    st->codec->codec_type == CODEC_TYPE_VIDEO) {
                    AVIndexEntry *e = &st->index_entries[st->nb_index_entries++];
                    e->pos = current_offset;
                    e->timestamp = current_dts;
                    e->size = sample_size;
                    e->min_distance = distance;
                    e->flags = keyframe ? AVINDEX_KEYFRAME : 0;
                    av_dlog(mov->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                            "size %d, distance %d, keyframe %d\n", st->index, current_sample,
                            current_offset, current_dts, sample_size, distance, keyframe);
                }

                current_offset += sample_size;
                stream_size += sample_size;
                current_dts += sc->stts_data[stts_index].duration;
                distance++;
                stts_sample++;
                current_sample++;
                if (stts_index + 1 < sc->stts_count && stts_sample == sc->stts_data[stts_index].count) {
                    stts_sample = 0;
                    stts_index++;
                }
            }
        }
        if (st->duration > 0) {
            st->codec->bit_rate = stream_size * 8 * sc->time_scale / st->duration;
        }
    } else {
        unsigned chunk_samples, total = 0;

        // compute total chunk count
        for (i = 0; i < sc->stsc_count; i++) {
            unsigned count, chunk_count;

            chunk_samples = sc->stsc_data[i].count;
            if (sc->samples_per_frame && chunk_samples % sc->samples_per_frame) {
                av_log(mov->fc, AV_LOG_ERROR, "error unaligned chunk\n");
                return;
            }

            if (sc->samples_per_frame >= 160) { // gsm
                count = chunk_samples / sc->samples_per_frame;
            } else if (sc->samples_per_frame > 1) {
                unsigned samples = (1024 / sc->samples_per_frame) * sc->samples_per_frame;
                count = (chunk_samples + samples - 1) / samples;
            } else {
                count = (chunk_samples + 1023) / 1024;
            }

            if (i < sc->stsc_count - 1) {
                chunk_count = sc->stsc_data[i + 1].first - sc->stsc_data[i].first;
            } else {
                chunk_count = sc->chunk_count - (sc->stsc_data[i].first - 1);
            }
            total += chunk_count * count;
        }

        av_dlog(mov->fc, "chunk count %d\n", total);
        if (total >= UINT_MAX / sizeof(*st->index_entries)) {
            return;
        }
        st->index_entries = av_malloc(total * sizeof(*st->index_entries));
        if (!st->index_entries) {
            return;
        }
        st->index_entries_allocated_size = total * sizeof(*st->index_entries);

        // populate index
        for (i = 0; i < sc->chunk_count; i++) {
            current_offset = sc->chunk_offsets[i];
            if (stsc_index + 1 < sc->stsc_count &&
                i + 1 == sc->stsc_data[stsc_index + 1].first) {
                stsc_index++;
            }
            chunk_samples = sc->stsc_data[stsc_index].count;

            while (chunk_samples > 0) {
                AVIndexEntry *e;
                unsigned size, samples;

                if (sc->samples_per_frame >= 160) { // gsm
                    samples = sc->samples_per_frame;
                    size = sc->bytes_per_frame;
                } else {
                    if (sc->samples_per_frame > 1) {
                        samples = FFMIN((1024 / sc->samples_per_frame) *
                                        sc->samples_per_frame, chunk_samples);
                        size = (samples / sc->samples_per_frame) * sc->bytes_per_frame;
                    } else {
                        samples = FFMIN(1024, chunk_samples);
                        size = samples * sc->sample_size;
                    }
                }

                if (st->nb_index_entries >= total) {
                    av_log(mov->fc, AV_LOG_ERROR, "wrong chunk count %d\n", total);
                    return;
                }
                e = &st->index_entries[st->nb_index_entries++];
                e->pos = current_offset;
                e->timestamp = current_dts;
                e->size = size;
                e->min_distance = 0;
                e->flags = AVINDEX_KEYFRAME;
                av_dlog(mov->fc, "AVIndex stream %d, chunk %d, offset %"PRIx64", dts %"PRId64", "
                        "size %d, duration %d\n", st->index, i, current_offset, current_dts,
                        size, samples);

                current_offset += size;
                current_dts += samples;
                chunk_samples -= samples;
            }
        }
    }
}

static int mov_open_dref(AVIOContext **pb, const char *src, MOVDref *ref)
{
    /* try relative path, we do not try the absolute because it can leak information about our
        system to an attacker */
#if 0
    /* try absolute path */
    if (!url_fopen(pb, ref->path, URL_RDONLY)) {
        return 0;
    }

    /* try relative path */
#endif
    if (ref->nlvl_to > 0 && ref->nlvl_from > 0) {
        char filename[1024];
        const char *src_path;
        int i, l;

        /* find a source dir */
        src_path = strrchr(src, '/');
        if (src_path) {
            src_path++;
        } else {
            src_path = src;
        }

        /* find a next level down to target */
        for (i = 0, l = strlen(ref->path) - 1; l >= 0; l--)
            if (ref->path[l] == '/') {
                if (i == ref->nlvl_to - 1) {
                    break;
                } else {
                    i++;
                }
            }

        /* compose filename if next level down to target was found */
        if (i == ref->nlvl_to - 1 && src_path - src  < sizeof(filename)) {
            memcpy(filename, src, src_path - src);
            filename[src_path - src] = 0;

            for (i = 1; i < ref->nlvl_from; i++) {
                av_strlcat(filename, "../", 1024);
            }

            av_strlcat(filename, ref->path + l + 1, 1024);

            if (!avio_open(pb, filename, AVIO_FLAG_READ)) {
                return 0;
            }
        }
    }

    return AVERROR(ENOENT);
}

static int mov_read_trak(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int ret;

    st = av_new_stream(c->fc, c->fc->nb_streams);
    if (!st) {
        return AVERROR(ENOMEM);
    }
    sc = av_mallocz(sizeof(MOVStreamContext));
    if (!sc) {
        return AVERROR(ENOMEM);
    }

    st->priv_data = sc;
    st->codec->codec_type = AVMEDIA_TYPE_DATA;
    sc->ffindex = st->index;

    if ((ret = mov_read_default(c, pb, atom)) < 0) {
        return ret;
    }

    /* sanity checks */
    if (sc->chunk_count && (!sc->stts_count || !sc->stsc_count ||
                            (!sc->sample_size && !sc->sample_count))) {
        av_log(c->fc, AV_LOG_ERROR, "stream %d, missing mandatory atoms, broken header\n",
               st->index);
        return 0;
    }

    if (sc->time_scale <= 0) {
        av_log(c->fc, AV_LOG_WARNING, "stream %d, timescale not set\n", st->index);
        sc->time_scale = c->time_scale;
        if (sc->time_scale <= 0) {
            sc->time_scale = 1;
        }
    }

    av_set_pts_info(st, 64, 1, sc->time_scale);

    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
        !st->codec->frame_size && sc->stts_count == 1) {
        st->codec->frame_size = av_rescale(sc->stts_data[0].duration,
                                           st->codec->sample_rate, sc->time_scale);
        av_dlog(c->fc, "frame size %d\n", st->codec->frame_size);
    }

    mov_build_index(c, st);

    if (sc->dref_id - 1 < sc->drefs_count && sc->drefs[sc->dref_id - 1].path) {
        MOVDref *dref = &sc->drefs[sc->dref_id - 1];
        if (mov_open_dref(&sc->pb, c->fc->filename, dref) < 0)
            av_log(c->fc, AV_LOG_ERROR,
                   "stream %d, error opening alias: path='%s', dir='%s', "
                   "filename='%s', volume='%s', nlvl_from=%d, nlvl_to=%d\n",
                   st->index, dref->path, dref->dir, dref->filename,
                   dref->volume, dref->nlvl_from, dref->nlvl_to);
    } else {
        sc->pb = c->fc->pb;
    }

    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (!st->sample_aspect_ratio.num &&
            (st->codec->width != sc->width || st->codec->height != sc->height)) {
            st->sample_aspect_ratio = av_d2q(((double)st->codec->height * sc->width) /
                                             ((double)st->codec->width * sc->height), INT_MAX);
        }

        av_reduce(&st->avg_frame_rate.num, &st->avg_frame_rate.den,
                  sc->time_scale * st->nb_frames, st->duration, INT_MAX);

        if (sc->stts_count == 1 || (sc->stts_count == 2 && sc->stts_data[1].count == 1))
            av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den,
                      sc->time_scale, sc->stts_data[0].duration, INT_MAX);
    }

    switch (st->codec->codec_id) {
#if CONFIG_H261_DECODER
    case CODEC_ID_H261:
#endif
#if CONFIG_H263_DECODER
    case CODEC_ID_H263:
#endif
#if CONFIG_H264_DECODER
    case CODEC_ID_H264:
#endif
#if CONFIG_MPEG4_DECODER
    case CODEC_ID_MPEG4:
#endif
        st->codec->width = 0; /* let decoder init width/height */
        st->codec->height = 0;
        break;
    }

    /* Do not need those anymore. */
    av_freep(&sc->chunk_offsets);
    av_freep(&sc->stsc_data);
    av_freep(&sc->sample_sizes);
    av_freep(&sc->keyframes);
    av_freep(&sc->stts_data);
    av_freep(&sc->stps_data);
    av_freep(&sc->rap_group);

    return 0;
}

static int mov_read_ilst(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int ret;
    c->itunes_metadata = 1;
    ret = mov_read_default(c, pb, atom);
    c->itunes_metadata = 0;
    return ret;
}

static int mov_read_meta(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    while (atom.size > 8) {
        uint32_t tag = avio_rl32(pb);
        atom.size -= 4;
        if (tag == MKTAG('h', 'd', 'l', 'r')) {
            avio_seek(pb, -8, SEEK_CUR);
            atom.size += 8;
            return mov_read_default(c, pb, atom);
        }
    }
    return 0;
}

static int mov_read_tkhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int i;
    int width;
    int height;
    int64_t disp_transform[2];
    int display_matrix[3][2];
    AVStream *st;
    MOVStreamContext *sc;
    int version;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    st = c->fc->streams[c->fc->nb_streams - 1];
    sc = st->priv_data;

    version = avio_r8(pb);
    avio_rb24(pb); /* flags */
    /*
    MOV_TRACK_ENABLED 0x0001
    MOV_TRACK_IN_MOVIE 0x0002
    MOV_TRACK_IN_PREVIEW 0x0004
    MOV_TRACK_IN_POSTER 0x0008
    */

    if (version == 1) {
        avio_rb64(pb);
        avio_rb64(pb);
    } else {
        avio_rb32(pb); /* creation time */
        avio_rb32(pb); /* modification time */
    }
    st->id = (int)avio_rb32(pb); /* track id (NOT 0 !)*/
    avio_rb32(pb); /* reserved */

    /* highlevel (considering edits) duration in movie timebase */
    (version == 1) ? avio_rb64(pb) : avio_rb32(pb);
    avio_rb32(pb); /* reserved */
    avio_rb32(pb); /* reserved */

    avio_rb16(pb); /* layer */
    avio_rb16(pb); /* alternate group */
    avio_rb16(pb); /* volume */
    avio_rb16(pb); /* reserved */

    //read in the display matrix (outlined in ISO 14496-12, Section 6.2.2)
    // they're kept in fixed point format through all calculations
    // ignore u,v,z b/c we don't need the scale factor to calc aspect ratio
    for (i = 0; i < 3; i++) {
        display_matrix[i][0] = avio_rb32(pb);   // 16.16 fixed point
        display_matrix[i][1] = avio_rb32(pb);   // 16.16 fixed point
        avio_rb32(pb);           // 2.30 fixed point (not used)
    }

    width = avio_rb32(pb);       // 16.16 fixed point track width
    height = avio_rb32(pb);      // 16.16 fixed point track height
    sc->width = width >> 16;
    sc->height = height >> 16;

    if (display_matrix[0][0] == -65536
        && display_matrix[0][1] == 0
        && display_matrix[1][0] == 0
        && display_matrix[1][1] == -65536) {
        av_dict_set(&st->metadata, "rotate", "180", 0);
        st->rotation_degree = 2;
    } else if (display_matrix[0][0] == 0
               && display_matrix[0][1] == 65536
               && display_matrix[1][0] == -65536
               && display_matrix[1][1] == 0) {
        av_dict_set(&st->metadata, "rotate", "90", 0);
        st->rotation_degree = 1;
    } else if (display_matrix[0][0] == 0
               && display_matrix[0][1] == -65536
               && display_matrix[1][0] == 65536
               && display_matrix[1][1] == 0) {
        av_dict_set(&st->metadata, "rotate", "270", 0);
        st->rotation_degree = 3;
    } else if (display_matrix[0][0] == 65536
               && display_matrix[0][1] == 0
               && display_matrix[1][0] == 0
               && display_matrix[1][1] == 65536) {
        av_dict_set(&st->metadata, "rotate", "0", 0);
        st->rotation_degree = 0;
    }

    // transform the display width/height according to the matrix
    // skip this if the display matrix is the default identity matrix
    // or if it is rotating the picture, ex iPhone 3GS
    // to keep the same scale, use [width height 1<<16]
    if (width && height &&
        ((display_matrix[0][0] != 65536  ||
          display_matrix[1][1] != 65536) &&
         !display_matrix[0][1] &&
         !display_matrix[1][0] &&
         !display_matrix[2][0] && !display_matrix[2][1])) {
        for (i = 0; i < 2; i++)
            disp_transform[i] =
                (int64_t)  width  * display_matrix[0][i] +
                (int64_t)  height * display_matrix[1][i] +
                ((int64_t) display_matrix[2][i] << 16);

        //sample aspect ratio is new width/height divided by old width/height
        st->sample_aspect_ratio = av_d2q(
                                      ((double) disp_transform[0] * height) /
                                      ((double) disp_transform[1] * width), INT_MAX);
    }
    return 0;
}

static int mov_read_tfhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVFragment *frag = &c->fragment;
    MOVTrackExt *trex = NULL;
    int flags, track_id, i;

    avio_r8(pb); /* version */
    flags = avio_rb24(pb);

    track_id = avio_rb32(pb);
    if (!track_id) {
        return -1;
    }
    frag->track_id = track_id;
    for (i = 0; i < c->trex_count; i++)
        if (c->trex_data[i].track_id == frag->track_id) {
            trex = &c->trex_data[i];
            break;
        }
    if (!trex) {
        av_log(c->fc, AV_LOG_ERROR, "could not find corresponding trex\n");
        return -1;
    }

    if (flags & 0x01) {
        frag->base_data_offset = avio_rb64(pb);
    } else {
        frag->base_data_offset = frag->moof_offset;
    }
    if (flags & 0x02) {
        frag->stsd_id          = avio_rb32(pb);
    } else {
        frag->stsd_id          = trex->stsd_id;
    }

    frag->duration = flags & 0x08 ? avio_rb32(pb) : trex->duration;
    frag->size     = flags & 0x10 ? avio_rb32(pb) : trex->size;
    frag->flags    = flags & 0x20 ? avio_rb32(pb) : trex->flags;
    av_dlog(c->fc, "frag flags 0x%x\n", frag->flags);
    return 0;
}

static int mov_read_chap(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    c->chapter_track = avio_rb32(pb);
    return 0;
}

static int mov_read_trex(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVTrackExt *trex;

    if ((uint64_t)c->trex_count + 1 >= UINT_MAX / sizeof(*c->trex_data)) {
        return -1;
    }
    trex = av_realloc(c->trex_data, (c->trex_count + 1) * sizeof(*c->trex_data));
    if (!trex) {
        return AVERROR(ENOMEM);
    }
    c->trex_data = trex;
    trex = &c->trex_data[c->trex_count++];
    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    trex->track_id = avio_rb32(pb);
    trex->stsd_id  = avio_rb32(pb);
    trex->duration = avio_rb32(pb);
    trex->size     = avio_rb32(pb);
    trex->flags    = avio_rb32(pb);
    return 0;
}

static int mov_read_trun(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVFragment *frag = &c->fragment;
    AVStream *st = NULL;
    MOVStreamContext *sc;
    MOVStts *ctts_data;
    uint64_t offset;
    int64_t dts;
    int data_offset = 0;
    unsigned entries, first_sample_flags = frag->flags;
    int flags, distance, i, found_keyframe = 0;

    for (i = 0; i < c->fc->nb_streams; i++) {
        if (c->fc->streams[i]->id == frag->track_id) {
            st = c->fc->streams[i];
            break;
        }
    }
    if (!st) {
        av_log(c->fc, AV_LOG_ERROR, "could not find corresponding track id %d\n", frag->track_id);
        return -1;
    }
    sc = st->priv_data;
    if (sc->pseudo_stream_id + 1 != frag->stsd_id) {
        return 0;
    }
    avio_r8(pb); /* version */
    flags = avio_rb24(pb);
    entries = avio_rb32(pb);
    av_dlog(c->fc, "flags 0x%x entries %d\n", flags, entries);

    /* Always assume the presence of composition time offsets.
     * Without this assumption, for instance, we cannot deal with a track in fragmented movies that meet the following.
     *  1) in the initial movie, there are no samples.
     *  2) in the first movie fragment, there is only one sample without composition time offset.
     *  3) in the subsequent movie fragments, there are samples with composition time offset. */
    if (!sc->ctts_count && sc->sample_count) {
        /* Complement ctts table if moov atom doesn't have ctts atom. */
        ctts_data = av_malloc(sizeof(*sc->ctts_data));
        if (!ctts_data) {
            return AVERROR(ENOMEM);
        }
        sc->ctts_data = ctts_data;
        sc->ctts_data[sc->ctts_count].count = sc->sample_count;
        sc->ctts_data[sc->ctts_count].duration = 0;
        sc->ctts_count++;
    }
    if ((uint64_t)entries + sc->ctts_count >= UINT_MAX / sizeof(*sc->ctts_data)) {
        return -1;
    }
    ctts_data = av_realloc(sc->ctts_data,
                           (entries + sc->ctts_count) * sizeof(*sc->ctts_data));
    if (!ctts_data) {
        return AVERROR(ENOMEM);
    }
    sc->ctts_data = ctts_data;

    if (flags & 0x001) {
        data_offset        = avio_rb32(pb);
    }
    if (flags & 0x004) {
        first_sample_flags = avio_rb32(pb);
    }
    dts    = sc->track_end - sc->time_offset;
    offset = frag->base_data_offset + data_offset;
    distance = 0;
    av_dlog(c->fc, "first sample flags 0x%x\n", first_sample_flags);
    for (i = 0; i < entries && !pb->eof_reached; i++) {
        unsigned sample_size = frag->size;
        int sample_flags = i ? frag->flags : first_sample_flags;
        unsigned sample_duration = frag->duration;
        int keyframe = 0;

        if (flags & 0x100) {
            sample_duration = avio_rb32(pb);
        }
        if (flags & 0x200) {
            sample_size     = avio_rb32(pb);
        }
        if (flags & 0x400) {
            sample_flags    = avio_rb32(pb);
        }
        sc->ctts_data[sc->ctts_count].count = 1;
        sc->ctts_data[sc->ctts_count].duration = (flags & 0x800) ? avio_rb32(pb) : 0;
        mov_update_dts_shift(sc, sc->ctts_data[sc->ctts_count].duration);
        sc->ctts_count++;
        if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            keyframe = 1;
        } else if (!found_keyframe)
            keyframe = found_keyframe =
                           !(sample_flags & (0x00010000 |  0x01000000));
        if (keyframe) {
            distance = 0;
        }
        av_add_index_entry(st, offset, dts, sample_size, distance,
                           keyframe ? AVINDEX_KEYFRAME : 0);
        av_dlog(c->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                "size %d, distance %d, keyframe %d\n", st->index, sc->sample_count + i,
                offset, dts, sample_size, distance, keyframe);
        distance++;
        dts += sample_duration;
        offset += sample_size;
    }

    if (pb->eof_reached) {
        return AVERROR_EOF;
    }

    frag->moof_offset = offset;
    st->duration = sc->track_end = dts + sc->time_offset;
    return 0;
}

/* this atom should be null (from specs), but some buggy files put the 'moov' atom inside it... */
/* like the files created with Adobe Premiere 5.0, for samples see */
/* http://graphics.tudelft.nl/~wouter/publications/soundtests/ */
static int mov_read_wide(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int err;

    if (atom.size < 8) {
        return 0;    /* continue */
    }
    if (avio_rb32(pb) != 0) { /* 0 sized mdat atom... use the 'wide' atom size */
        avio_skip(pb, atom.size - 4);
        return 0;
    }
    atom.type = avio_rl32(pb);
    atom.size -= 8;
    if (atom.type != MKTAG('m', 'd', 'a', 't')) {
        avio_skip(pb, atom.size);
        return 0;
    }
    err = mov_read_mdat(c, pb, atom);
    return err;
}

static int mov_read_cmov(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
#if CONFIG_ZLIB
    AVIOContext ctx;
    uint8_t *cmov_data;
    uint8_t *moov_data; /* uncompressed data */
    long cmov_len, moov_len;
    int ret = -1;

    avio_rb32(pb); /* dcom atom */
    if (avio_rl32(pb) != MKTAG('d', 'c', 'o', 'm')) {
        return -1;
    }
    if (avio_rl32(pb) != MKTAG('z', 'l', 'i', 'b')) {
        av_log(c->fc, AV_LOG_ERROR, "unknown compression for cmov atom !");
        return -1;
    }
    avio_rb32(pb); /* cmvd atom */
    if (avio_rl32(pb) != MKTAG('c', 'm', 'v', 'd')) {
        return -1;
    }
    moov_len = avio_rb32(pb); /* uncompressed size */
    cmov_len = atom.size - 6 * 4;

    cmov_data = av_malloc(cmov_len);
    if (!cmov_data) {
        return AVERROR(ENOMEM);
    }
    moov_data = av_malloc(moov_len);
    if (!moov_data) {
        av_free(cmov_data);
        return AVERROR(ENOMEM);
    }
    avio_read(pb, cmov_data, cmov_len);
    if (uncompress(moov_data, (uLongf *) &moov_len, (const Bytef *)cmov_data, cmov_len) != Z_OK) {
        goto free_and_return;
    }
    if (ffio_init_context(&ctx, moov_data, moov_len, 0, NULL, NULL, NULL, NULL) != 0) {
        goto free_and_return;
    }
    atom.type = MKTAG('m', 'o', 'o', 'v');
    atom.size = moov_len;
    ret = mov_read_default(c, &ctx, atom);
free_and_return:
    av_free(moov_data);
    av_free(cmov_data);
    return ret;
#else
    av_log(c->fc, AV_LOG_ERROR, "this file requires zlib support compiled in\n");
    return -1;
#endif
}

/* edit list atom */
static int mov_read_elst(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVStreamContext *sc;
    int i, edit_count, version;

    if (c->fc->nb_streams < 1) {
        return 0;
    }
    sc = c->fc->streams[c->fc->nb_streams - 1]->priv_data;

    version = avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    edit_count = avio_rb32(pb); /* entries */

    if ((uint64_t)edit_count * 12 + 8 > atom.size) {
        return -1;
    }

    for (i = 0; i < edit_count; i++) {
        int64_t time;
        int64_t duration;
        if (version == 1) {
            duration = avio_rb64(pb);
            time     = avio_rb64(pb);
        } else {
            duration = avio_rb32(pb); /* segment duration */
            time     = (int32_t)avio_rb32(pb); /* media time */
        }
        avio_rb32(pb); /* Media rate */
#if 0
        // ignore time_offset
        if (i == 0 && time >= -1) {
            sc->time_offset = time != -1 ? time : -duration;
        }
#endif
    }

    if (edit_count > 1)
        av_log(c->fc, AV_LOG_WARNING, "multiple edit list entries, "
               "a/v desync might occur, patch welcome\n");

    av_dlog(c->fc, "track[%i].edit_count = %i\n", c->fc->nb_streams - 1, edit_count);
    return 0;
}

static int mov_read_chan(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    if (atom.size < 16) {
        return AVERROR_INVALIDDATA;
    }
    avio_skip(pb, 4);
    ff_mov_read_chan(c->fc, atom.size - 4, c->fc->streams[0]->codec);
    return 0;
}

static const MOVParseTableEntry mov_default_parse_table[] = {
    { MKTAG('a', 'v', 's', 's'), mov_read_extradata },
    { MKTAG('c', 'h', 'p', 'l'), mov_read_chpl },
    { MKTAG('c', 'o', '6', '4'), mov_read_stco },
    { MKTAG('c', 't', 't', 's'), mov_read_ctts }, /* composition time to sample */
    { MKTAG('d', 'i', 'n', 'f'), mov_read_default },
    { MKTAG('d', 'r', 'e', 'f'), mov_read_dref },
    { MKTAG('e', 'd', 't', 's'), mov_read_default },
    { MKTAG('e', 'l', 's', 't'), mov_read_elst },
    { MKTAG('e', 'n', 'd', 'a'), mov_read_enda },
    { MKTAG('f', 'i', 'e', 'l'), mov_read_extradata },
    { MKTAG('f', 't', 'y', 'p'), mov_read_ftyp },
    { MKTAG('g', 'l', 'b', 'l'), mov_read_glbl },
    { MKTAG('h', 'd', 'l', 'r'), mov_read_hdlr },
    { MKTAG('i', 'l', 's', 't'), mov_read_ilst },
    { MKTAG('j', 'p', '2', 'h'), mov_read_extradata },
    { MKTAG('m', 'd', 'a', 't'), mov_read_mdat },
    { MKTAG('a', 'v', 'i', 'd'), mov_read_mdat }, /*for 3dv*/
    { MKTAG('m', 'd', 'h', 'd'), mov_read_mdhd },
    { MKTAG('m', 'd', 'i', 'a'), mov_read_default },
    { MKTAG('m', 'e', 't', 'a'), mov_read_meta },
    { MKTAG('m', 'i', 'n', 'f'), mov_read_default },
    { MKTAG('m', 'o', 'o', 'f'), mov_read_moof },
    { MKTAG('m', 'o', 'o', 'v'), mov_read_moov },
    { MKTAG('3', 'd', 'v', 'f'), mov_read_moov }, /*for 3dv*/
    { MKTAG('m', 'v', 'e', 'x'), mov_read_default },
    { MKTAG('m', 'v', 'h', 'd'), mov_read_mvhd },
    { MKTAG('m', 'v', 'c', 'C'), mov_read_mvcC },
    { MKTAG('S', 'M', 'I', ' '), mov_read_smi }, /* Sorenson extension ??? */
    { MKTAG('a', 'l', 'a', 'c'), mov_read_extradata }, /* alac specific atom */
    { MKTAG('a', 'v', 'c', 'C'), mov_read_glbl },
    { MKTAG('p', 'a', 's', 'p'), mov_read_pasp },
    { MKTAG('s', 't', 'b', 'l'), mov_read_default },
    { MKTAG('s', 't', 'c', 'o'), mov_read_stco },
    { MKTAG('s', 't', 'p', 's'), mov_read_stps },
    { MKTAG('s', 't', 'r', 'f'), mov_read_strf },
    { MKTAG('s', 't', 's', 'c'), mov_read_stsc },
    { MKTAG('s', 't', 's', 'd'), mov_read_stsd }, /* sample description */
    { MKTAG('s', 't', 's', 's'), mov_read_stss }, /* sync sample */
    { MKTAG('s', 't', 's', 'z'), mov_read_stsz }, /* sample size */
    { MKTAG('s', 't', 't', 's'), mov_read_stts },
    { MKTAG('s', 't', 'z', '2'), mov_read_stsz }, /* compact sample size */
    { MKTAG('t', 'k', 'h', 'd'), mov_read_tkhd }, /* track header */
    { MKTAG('t', 'f', 'h', 'd'), mov_read_tfhd }, /* track fragment header */
    { MKTAG('t', 'r', 'a', 'k'), mov_read_trak },
    { MKTAG('t', 'r', 'a', 'f'), mov_read_default },
    { MKTAG('t', 'r', 'e', 'f'), mov_read_default },
    { MKTAG('c', 'h', 'a', 'p'), mov_read_chap },
    { MKTAG('t', 'r', 'e', 'x'), mov_read_trex },
    { MKTAG('t', 'r', 'u', 'n'), mov_read_trun },
    { MKTAG('u', 'd', 't', 'a'), mov_read_default },
    { MKTAG('w', 'a', 'v', 'e'), mov_read_wave },
    { MKTAG('e', 's', 'd', 's'), mov_read_esds },
    { MKTAG('d', 'a', 'c', '3'), mov_read_dac3 }, /* AC-3 info */
    { MKTAG('w', 'i', 'd', 'e'), mov_read_wide }, /* place holder */
    { MKTAG('w', 'f', 'e', 'x'), mov_read_wfex },
    { MKTAG('c', 'm', 'o', 'v'), mov_read_cmov },
    { MKTAG('c', 'h', 'a', 'n'), mov_read_chan },
    { MKTAG('h', 'v', 'c', 'C'), mov_read_glbl }, /* HEVC/H.265 */
    { MKTAG('d', 'v', 'c', '1'), mov_read_dvc1 },
    { MKTAG('s', 'b', 'g', 'p'), mov_read_sbgp },
    { 0, NULL }
};

static int mov_probe(AVProbeData *p)
{
    unsigned int offset;
    uint32_t tag;
    int score = 0;

    /* check file header */
    offset = 0;
    for (;;) {
        /* ignore invalid offset */
        if ((offset + 8) > (unsigned int)p->buf_size) {
            return score;
        }
        tag = AV_RL32(p->buf + offset + 4);
        switch (tag) {
        /* check for obvious tags */
        case MKTAG('j', 'P', ' ', ' '): /* jpeg 2000 signature */
        case MKTAG('m', 'o', 'o', 'v'):
        case MKTAG('m', 'd', 'a', 't'):
        case MKTAG('p', 'n', 'o', 't'): /* detect movs with preview pics like ew.mov and april.mov */
        case MKTAG('u', 'd', 't', 'a'): /* Packet Video PVAuthor adds this and a lot of more junk */
        case MKTAG('f', 't', 'y', 'p'):
        case MKTAG('p', 'i', 'n', 'f'): /* for 3dv */
            return AVPROBE_SCORE_MAX;
        /* those are more common words, so rate then a bit less */
        case MKTAG('e', 'd', 'i', 'w'): /* xdcam files have reverted first tags */
        case MKTAG('w', 'i', 'd', 'e'):
        case MKTAG('f', 'r', 'e', 'e'):
        case MKTAG('j', 'u', 'n', 'k'):
        case MKTAG('p', 'i', 'c', 't'):
            return AVPROBE_SCORE_MAX - 5;
        case MKTAG(0x82, 0x82, 0x7f, 0x7d):
        case MKTAG('s', 'k', 'i', 'p'):
        case MKTAG('u', 'u', 'i', 'd'):
        case MKTAG('p', 'r', 'f', 'l'):
            offset = AV_RB32(p->buf + offset) + offset;
            /* if we only find those cause probedata is too small at least rate them */
            score = AVPROBE_SCORE_MAX - 50;
            break;
        default:
            /* unrecognized tag */
            return score;
        }
    }
    return score;
}

// must be done after parsing all trak because there's no order requirement
static void mov_read_chapters(AVFormatContext *s)
{
    MOVContext *mov = s->priv_data;
    AVStream *st = NULL;
    MOVStreamContext *sc;
    int64_t cur_pos;
    int i;

    for (i = 0; i < s->nb_streams; i++)
        if (s->streams[i]->id == mov->chapter_track) {
            st = s->streams[i];
            break;
        }
    if (!st) {
        av_log(s, AV_LOG_ERROR, "Referenced QT chapter track not found\n");
        return;
    }

    st->discard = AVDISCARD_ALL;
    sc = st->priv_data;
    cur_pos = avio_tell(sc->pb);

    for (i = 0; i < st->nb_index_entries; i++) {
        AVIndexEntry *sample = &st->index_entries[i];
        int64_t end = i + 1 < st->nb_index_entries ? st->index_entries[i + 1].timestamp : st->duration;
        uint8_t *title;
        uint16_t ch;
        int len, title_len;

        if (avio_seek(sc->pb, sample->pos, SEEK_SET) != sample->pos) {
            av_log(s, AV_LOG_ERROR, "Chapter %d not found in file\n", i);
            goto finish;
        }

        // the first two bytes are the length of the title
        len = avio_rb16(sc->pb);
        if (len > sample->size - 2) {
            continue;
        }
        title_len = 2 * len + 1;
        if (!(title = av_mallocz(title_len))) {
            goto finish;
        }

        // The samples could theoretically be in any encoding if there's an encd
        // atom following, but in practice are only utf-8 or utf-16, distinguished
        // instead by the presence of a BOM
        ch = avio_rb16(sc->pb);
        if (ch == 0xfeff) {
            avio_get_str16be(sc->pb, len, title, title_len);
        } else if (ch == 0xfffe) {
            avio_get_str16le(sc->pb, len, title, title_len);
        } else {
            AV_WB16(title, ch);
            get_strz(sc->pb, title + 2, len - 1);
        }

        ff_new_chapter(s, i, st->time_base, sample->timestamp, end, title);
        av_freep(&title);
    }
finish:
    avio_seek(sc->pb, cur_pos, SEEK_SET);
}

static int mov_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    MOVContext *mov = s->priv_data;
    AVIOContext *pb = s->pb;
    int err;
    MOVAtom atom = { AV_RL32("root") };

    mov->fc = s;
    /* .mov and .mp4 aren't streamable anyway (only progressive download if moov is before mdat) */
    if (pb->seekable) {
        atom.size = avio_size(pb);
    } else {
        atom.size = INT64_MAX;
    }

    /* check MOV header */
    if ((err = mov_read_default(mov, pb, atom)) < 0) {
        av_log(s, AV_LOG_ERROR, "error reading header: %d\n", err);
        return err;
    }
    if (!mov->found_moov) {
        av_log(s, AV_LOG_ERROR, "moov atom not found\n");
        return -1;
    }
    av_dlog(mov->fc, "on_parse_exit_offset=%"PRId64"\n", avio_tell(pb));

    if (pb->seekable && mov->chapter_track > 0) {
        mov_read_chapters(s);
    }


    if (s->pb && s->pb->local_playback != 1) { //web live playback,check movie file
        int64_t amin_pos = INT64_MAX, vmin_pos = INT64_MAX, min = INT64_MAX;
        int64_t amax_pos = 0, vmax_pos = 0, max = 0;

        for (int i = 0; i < s->nb_streams; i++) {
            AVStream *avst = s->streams[i];
            min = INT64_MAX;
            max = 0;
            if (avst->nb_index_entries > 0 && avst->index_entries[avst->nb_index_entries - 1].pos > max) {
                max = avst->index_entries[avst->nb_index_entries - 1].pos;
            }
            if (avst->nb_index_entries > 0 && avst->index_entries[0].pos < min) {
                min = avst->index_entries[0].pos;
            }

            if (avst->codec->codec_type == AVMEDIA_TYPE_AUDIO && min < amin_pos && max > amax_pos) {
                amin_pos = min;
                amax_pos = max;
            }
            if (avst->codec->codec_type == AVMEDIA_TYPE_VIDEO && min < vmin_pos && max > vmax_pos) {
                vmin_pos = min;
                vmax_pos = max;
            }
        }
        if ((vmin_pos > amax_pos && abs(vmin_pos - amin_pos) > MAX_READ_SEEK && amin_pos >= 0 && amin_pos != INT64_MAX) ||
            (amin_pos > vmax_pos && abs(amin_pos - vmin_pos) > MAX_READ_SEEK && vmin_pos >= 0 && vmin_pos != INT64_MAX)) {
            url_set_more_data_seek(s->pb);
            av_log(NULL, AV_LOG_WARNING, "May need cache more for smooth playback on line\n");
        }
    }

    return 0;
}

static AVIndexEntry *mov_find_next_sample(AVFormatContext *s, AVStream **st)
{
    AVIndexEntry *sample = NULL;
    int64_t best_dts = INT64_MAX;
    int i;
    float limit_sec = 0;
    float bit_rateKB = 0;
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *avst = s->streams[i];
        MOVStreamContext *msc = avst->priv_data;
        MOVStreamContext *best_sc;
        int wantnew = 0;
        int beststream_readed_cnt = 0;
        if (*st) {
            best_sc = (*st)->priv_data;
            beststream_readed_cnt = best_sc->readed_count;
        }
        if (msc->pb && msc->current_sample < avst->nb_index_entries) {
            AVIndexEntry *current_sample = &avst->index_entries[msc->current_sample];
            int64_t dts = av_rescale(current_sample->timestamp, AV_TIME_BASE, msc->time_scale);
            av_dlog(s, "stream %d, sample %d, dts %"PRId64"\n", i, msc->current_sample, dts);
            if (!sample || (!s->pb->seekable && current_sample->pos < sample->pos)) { /*not seekable streaming,*/
                wantnew = 1;
            } else if (msc->pb != s->pb && dts < best_dts) { /*read from different file,*/
                wantnew = 1;
            } else if ((s->pb->seekable && !s->pb->is_slowmedia) && /*local files.,*/
                       ((FFABS(best_dts - dts) <= AV_TIME_BASE && current_sample->pos < sample->pos) ||
                        (FFABS(best_dts - dts) > AV_TIME_BASE && dts < best_dts))) {
                wantnew = 1;
            } else if ((s->pb->seekable && s->pb->is_slowmedia)) { /*seekable network,seek is slow...*/
                int64_t curentpos = avio_tell(s->pb);
                int reached_cnt = 0;
                bit_rateKB = (s->bit_rate / (1024 * 8));
                limit_sec = (float)LIMIT_BUFSIZE / (bit_rateKB * 1024);
                /*add limit_sec and reached_cnt choose, avoid after seek only get audio or video data
                  cause playback not smooth */
                if (limit_sec >= 5) {
                    limit_sec = 5;
                    reached_cnt = 500;
                } else if (limit_sec < 5 && limit_sec >= 3) {
                    limit_sec = 3;
                    reached_cnt = 400;
                } else {
                    limit_sec = 2;
                    reached_cnt = 200;
                }
                if ((FFABS(best_dts - dts) < AV_TIME_BASE * limit_sec) && beststream_readed_cnt > reached_cnt && msc->readed_count > reached_cnt) { /*not first parse.*/
                    if (FFABS(curentpos - current_sample->pos) < FFABS(curentpos - sample->pos)) {
                        wantnew = 1;
                    }
                } else if (((FFABS(best_dts - dts) <= AV_TIME_BASE && current_sample->pos < sample->pos) ||
                            (FFABS(best_dts - dts) > AV_TIME_BASE && dts < best_dts))) {
                    wantnew = 1;
                }
                ///av_log(s, AV_LOG_WARNING, "curentpos=%llx,dts=%llx,best_dts=%llx,%llx,%llx,new=%d\n",curentpos,dts,best_dts,current_sample->pos,sample->pos,wantnew);
            }

            if (wantnew) {
                sample = current_sample;
                best_dts = dts;
                *st = avst;
            }

        }
    }
    return sample;
}
static int should_retry(AVIOContext *pb, int error_code)
{
    if (error_code == AVERROR_EOF || url_feof(pb)) {
        return 0;
    }

    return 1;
}

static int mov_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    MOVContext *mov = s->priv_data;
    MOVStreamContext *sc;
    AVIndexEntry *sample;
    AVStream *st = NULL;
    int ret;
    int64_t offset = 0;
retry:
    if (url_interrupt_cb()) {
        av_log(s, AV_LOG_WARNING, "interrupt, exit\n");
        return AVERROR_EXIT;
    }
    sample = mov_find_next_sample(s, &st);
    if (!sample) {
        mov->found_mdat = 0;
        if (s->pb->seekable ||
        mov_read_default(mov, s->pb, (MOVAtom) {
        AV_RL32("root"), INT64_MAX
        }) < 0 ||
        url_feof(s->pb)) {
            return AVERROR_EOF;
        }
        av_dlog(s, "read fragments, offset 0x%"PRIx64"\n", avio_tell(s->pb));
        goto retry;
    }
    sc = st->priv_data;
    /* must be done just before reading, to avoid infinite loop on sample */
    sc->current_sample++;
    if (st->discard != AVDISCARD_ALL) {
        offset = avio_seek(sc->pb, sample->pos, SEEK_SET);
        if (offset != sample->pos) {
            av_log(mov->fc, AV_LOG_ERROR, "stream %d, seekto offset 0x%"PRIx64" ret 0x%"PRIx64":partial file\n",
                   sc->ffindex, sample->pos, offset);
            // seek failed, retry same sample
            sc->current_sample -= should_retry(sc->pb, offset);

            if (sample->pos > s->file_size || offset == AVERROR_EOF) {
                return AVERROR_EOF;
            } else {
                return -1;
            }
        }
        ret = av_get_packet(sc->pb, pkt, sample->size);
        /*retry same sample ,avoid show error pic*/
        if (ret != sample->size) {
            if (!(ret <= 0)) {
                av_free_packet(pkt);
            }
            sc->current_sample -= should_retry(sc->pb, ret);
            return ret;
        }
        sc->readed_count++;
        if (sc->has_palette) {
            uint8_t *pal;

            pal = av_packet_new_side_data(pkt, AV_PKT_DATA_PALETTE, AVPALETTE_SIZE);
            if (!pal) {
                av_log(mov->fc, AV_LOG_ERROR, "Cannot append palette to packet\n");
            } else {
                memcpy(pal, sc->palette, AVPALETTE_SIZE);
                sc->has_palette = 0;
            }
        }
#if CONFIG_DV_DEMUXER
        if (mov->dv_demux && sc->dv_audio_container) {
            dv_produce_packet(mov->dv_demux, pkt, pkt->data, pkt->size, pkt->pos);
            av_free(pkt->data);
            pkt->size = 0;
            ret = dv_get_packet(mov->dv_demux, pkt);
            if (ret < 0) {
                return ret;
            }
        }
#endif
    }

    pkt->stream_index = sc->ffindex;
    pkt->dts = sample->timestamp;
    if (sc->ctts_data) {
        pkt->pts = pkt->dts + sc->dts_shift + sc->ctts_data[sc->ctts_index].duration;
        /* update ctts context */
        sc->ctts_sample++;
        if (sc->ctts_index < sc->ctts_count &&
            sc->ctts_data[sc->ctts_index].count == sc->ctts_sample) {
            sc->ctts_index++;
            sc->ctts_sample = 0;
        }
        if (sc->wrong_dts) {
            pkt->dts = AV_NOPTS_VALUE;
        }
    } else {
        int64_t next_dts = (sc->current_sample < st->nb_index_entries) ?
                           st->index_entries[sc->current_sample].timestamp : st->duration;
        pkt->duration = next_dts - pkt->dts;
        pkt->pts = pkt->dts;
    }
    if (st->discard == AVDISCARD_ALL) {
        goto retry;
    }
    pkt->flags |= sample->flags & AVINDEX_KEYFRAME ? AV_PKT_FLAG_KEY : 0;
    pkt->pos = sample->pos;
    av_dlog(s, "stream %d, pts %"PRId64", dts %"PRId64", pos 0x%"PRIx64", duration %d\n",
            pkt->stream_index, pkt->pts, pkt->dts, pkt->pos, pkt->duration);
    return 0;
}

static int64_t mov_read_seek_sync(AVFormatContext *s,
                                  int stream_index,
                                  int64_t min_ts,
                                  int64_t target_ts,
                                  int64_t max_ts,
                                  int flags)
{
    int64_t pos, t_pos;

    int64_t ts_ret, ts_adj;
    int stream_index_gen_search = stream_index;
    int sample, i;
    AVStream *st;
    AVParserState *backup;

    backup = ff_store_parser_state(s);

    // detect direction of seeking for search purposes
    flags |= (target_ts - min_ts > (uint64_t)(max_ts - target_ts)) ?
             0 : AVSEEK_FLAG_BACKWARD;

    st = s->streams[stream_index_gen_search];
    sample = av_index_search_timestamp(st, target_ts, AVSEEK_FLAG_ANY);
    pos = st->index_entries[sample].pos;
    target_ts = st->index_entries[sample].timestamp;
    for (i = 0; i < s->nb_streams; i++) {
        MOVStreamContext *sc = s->streams[i]->priv_data;
        sc->current_sample = (sample - 500) > 0 ? (sample - 500) : 0;  // hard code for mov, repos the sample.
    }

    // search for actual matching keyframe/starting position for all streams
    if ((t_pos = ff_gen_syncpoint_search(s, stream_index, pos,
                                         min_ts, target_ts, max_ts,
                                         flags)) < 0) {
        ff_restore_parser_state(s, backup);
        return -1;
    }

    ff_free_parser_state(s, backup);
    return t_pos;
}

static int64_t mov_read_seek2(AVFormatContext *s, int stream_index, int64_t target_ts, int flags)
{
    int ret;
    if (!flags) {
        flags = AVSEEK_FLAG_BACKWARD;
    }

    if (flags & AVSEEK_FLAG_BACKWARD) {
        flags &= ~AVSEEK_FLAG_BACKWARD;
        ret = mov_read_seek_sync(s, stream_index, INT64_MIN, target_ts, target_ts, flags);
        if (ret < 0) {
            // for compatibility reasons, seek to the best-fitting timestamp
            ret = mov_read_seek_sync(s, stream_index, INT64_MIN, target_ts, INT64_MAX, flags);
        }
    } else {
        ret = mov_read_seek_sync(s, stream_index, target_ts, target_ts, INT64_MAX, flags);
        if (ret < 0)
            // for compatibility reasons, seek to the best-fitting timestamp
        {
            ret = mov_read_seek_sync(s, stream_index, INT64_MIN, target_ts, INT64_MAX, flags);
        }
    }
    return ret;
}

static int mov_index_search_pos(const AVIndexEntry *entries, int nb_entries,
                                int64_t pos, int flags)
{
    int a, b, m;
    int64_t ppos;

    a = - 1;
    b = nb_entries;

    //optimize appending index entries at the end
    if (b && entries[b - 1].pos < pos) {
        a = b - 1;
    }

    while (b - a > 1) {
        m = (a + b) >> 1;
        ppos = entries[m].pos;
        if (ppos >= pos) {
            b = m;
        }
        if (ppos <= pos) {
            a = m;
        }
    }

    m = (flags & AVSEEK_FLAG_BACKWARD) ? a : b;
    return  m;
}

static int mov_seek_stream(AVFormatContext *s, AVStream *st, int64_t timestamp, int flags)
{
    MOVStreamContext *sc = st->priv_data;
    int sample, time_sample;
    int i;

    sample = av_index_search_timestamp(st, timestamp, flags);

    // mov's stss is wrong sometimes, need to read seek
    // added by senbai.tao
    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO && sample <= 0 && st->nb_index_entries && sc->keyframe_count <= 1) {
        int64_t sync_point = mov_read_seek2(s, st->index, timestamp, flags);
        sample = mov_index_search_pos(st->index_entries, st->nb_index_entries, sync_point, AVSEEK_FLAG_ANY);
    }

    av_dlog(s, "stream %d, timestamp %"PRId64", sample %d\n", st->index, timestamp, sample);
    if (sample < 0 && st->nb_index_entries && timestamp < st->index_entries[0].timestamp) {
        sample = 0;
    }
    if (sample < 0) { /* not sure what to do */
        return -1;
    }
    sc->current_sample = sample;
    av_dlog(s, "stream %d, found sample %d\n", st->index, sc->current_sample);
    /* adjust ctts index */
    if (sc->ctts_data) {
        time_sample = 0;
        for (i = 0; i < sc->ctts_count; i++) {
            int next = time_sample + sc->ctts_data[i].count;
            if (next > sc->current_sample) {
                sc->ctts_index = i;
                sc->ctts_sample = sc->current_sample - time_sample;
                break;
            }
            time_sample = next;
        }
    }
    return sample;
}

static int mov_read_seek(AVFormatContext *s, int stream_index, int64_t sample_time, int flags)
{
    AVStream *st;
    int64_t seek_timestamp, timestamp;
    int sample;
    int i;

    if (stream_index >= s->nb_streams) {
        return -1;
    }
    if (sample_time < 0) {
        sample_time = 0;
    }

    st = s->streams[stream_index];
    MOVStreamContext *msc = st->priv_data;
    msc->readed_count = 0;  // clear readed_count before seek
    sample = mov_seek_stream(s, st, sample_time, flags);
    if (sample < 0) {
        return -1;
    }

    /* adjust seek timestamp to found sample timestamp */
    seek_timestamp = st->index_entries[sample].timestamp;

    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (stream_index == i) {
            continue;
        }

        timestamp = av_rescale_q(seek_timestamp, s->streams[stream_index]->time_base, st->time_base);
        mov_seek_stream(s, st, timestamp, flags);
    }
    return 0;
}

static int mov_read_close(AVFormatContext *s)
{
    MOVContext *mov = s->priv_data;
    int i, j;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        MOVStreamContext *sc = st->priv_data;

        av_freep(&sc->ctts_data);
        for (j = 0; j < sc->drefs_count; j++) {
            av_freep(&sc->drefs[j].path);
            av_freep(&sc->drefs[j].dir);
        }
        av_freep(&sc->drefs);
        if (sc->pb && sc->pb != s->pb) {
            avio_close(sc->pb);
        }

        av_freep(&st->codec->palctrl);
    }

    if (mov->dv_demux) {
        for (i = 0; i < mov->dv_fctx->nb_streams; i++) {
            av_freep(&mov->dv_fctx->streams[i]->codec);
            av_freep(&mov->dv_fctx->streams[i]);
        }
        av_freep(&mov->dv_fctx);
        av_freep(&mov->dv_demux);
    }

    av_freep(&mov->trex_data);

    return 0;
}

AVInputFormat ff_mov_demuxer = {
    "mov,mp4,m4a,3gp,3g2,mj2",
    NULL_IF_CONFIG_SMALL("QuickTime/MPEG-4/Motion JPEG 2000 format"),
    sizeof(MOVContext),
    mov_probe,
    mov_read_header,
    mov_read_packet,
    mov_read_close,
    mov_read_seek,
    .flags = AVFMT_NOGENSEARCH,
};
