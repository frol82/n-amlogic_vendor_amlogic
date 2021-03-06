/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef _STREAM_FORMAT_H_
#define _STREAM_FORMAT_H_

#include "amports/vformat.h"
#include "amports/aformat.h"

typedef enum {
    NONE = 0,
    ID3V1,
    ID3V2,
    APEV1,
    APEV2,
    WMATAG,
    MPEG4TAG,
} audio_tag_type;

typedef enum {
    UNKNOWN_FILE    = 0,
    AVI_FILE        = 1,
    MPEG_FILE       = 2,
    WAV_FILE        = 3,
    MP3_FILE        = 4,
    AAC_FILE        = 5,
    AC3_FILE        = 6,
    RM_FILE         = 7,
    DTS_FILE        = 8,
    MKV_FILE        = 9,
    MOV_FILE        = 10,
    MP4_FILE        = 11,
    FLAC_FILE       = 12,
    H264_FILE       = 13,
    M2V_FILE        = 14,
    FLV_FILE        = 15,
    P2P_FILE        = 16,
    ASF_FILE        = 17,
    STREAM_FILE     = 18,
    APE_FILE        = 19,
    AMR_FILE        = 20,
    AVS_FILE        = 21,
    PMP_FILE        = 22,
    OGM_FILE            = 23,
    HEVC_FILE       = 24,
    DRA_FILE        = 25,
    IVF_FILE        = 26,
    FILE_MAX,
} pfile_type;

#endif
