/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef HLS_MACRO_H
#define HLS_MACRO_H

#define INITIAL_BUFFER_SIZE 32768

#define MAX_FIELD_LEN 64
#define MAX_CHARACTERISTICS_LEN 512

#define MPEG_TIME_BASE 90000
#define MPEG_TIME_BASE_Q (AVRational){1, MPEG_TIME_BASE}

// user defined
#define HLS_MAX_CACHE_SIZE 10*1024*1024
#define HLS_TIME_BASE 1000000
#define MAX_URL_SIZE 4096

#endif
