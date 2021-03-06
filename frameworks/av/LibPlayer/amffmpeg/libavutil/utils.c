/*
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

#include "config.h"
#include "avutil.h"

/**
 * @file
 * various utility functions
 */

unsigned avutil_version(void)
{
    return LIBAVUTIL_VERSION_INT;
}

const char *avutil_configuration(void)
{
    return FFMPEG_CONFIGURATION;
}

const char *avutil_license(void)
{
#define LICENSE_PREFIX "libavutil license: "
    return LICENSE_PREFIX FFMPEG_LICENSE + sizeof(LICENSE_PREFIX) - 1;
}

char av_get_picture_type_char(enum AVPictureType pict_type)
{
    switch (pict_type) {
    case AV_PICTURE_TYPE_I:
        return 'I';
    case AV_PICTURE_TYPE_P:
        return 'P';
    case AV_PICTURE_TYPE_B:
        return 'B';
    case AV_PICTURE_TYPE_S:
        return 'S';
    case AV_PICTURE_TYPE_SI:
        return 'i';
    case AV_PICTURE_TYPE_SP:
        return 'p';
    case AV_PICTURE_TYPE_BI:
        return 'b';
    default:
        return '?';
    }
}
