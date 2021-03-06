/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AMSUB_IO_CTRL_H_
#define AMSUB_IO_CTRL_H_
#include "amsub_dec.h"

int subtitle_poll_sub_fd(int sub_fd, int timeout);
int subtitle_get_sub_size_fd(int sub_fd);
int subtitle_read_sub_data_fd(int sub_fd, char *buf, unsigned int length);
int update_read_pointer(int sub_handle, int flag);

int amsub_read_sub_data(amsub_para_t *amsub_para, amsub_info_t *amsub_info);
int open_sub_device();



#endif
