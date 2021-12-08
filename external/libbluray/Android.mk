LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_DIR := $(LOCAL_PATH)/src
LOCAL_LIBBLURAY_DIR := $(LOCAL_PATH)/src/libbluray
# LOCAL_FILE_DIR := $(LOCAL_PATH)/src/file
# LOCAL_UTIL_DIR := $(LOCAL_PATH)/src/util

LOCAL_CFLAGS := \
    -DHAVE_PTHREAD_H \
    -DHAVE_SYS_TIME_H \
    -DHAVE_DIRENT_H \
    -DHAVE_DLFCN_H

LOCAL_CFLAGS += -Wno-format-security

LOCAL_LDLIBS := -ldl

LOCAL_SRC_FILES := \
    src/libbluray/bluray.c \
    src/libbluray/register.c \
    src/libbluray/decoders/m2ts_filter.c \
    src/libbluray/decoders/graphics_controller.c \
    src/libbluray/decoders/rle.c \
    src/libbluray/decoders/graphics_processor.c \
    src/libbluray/decoders/textst_render.c \
    src/libbluray/decoders/textst_decode.c \
    src/libbluray/decoders/pg_decode.c \
    src/libbluray/decoders/ig_decode.c \
    src/libbluray/decoders/m2ts_demux.c \
    src/libbluray/decoders/pes_buffer.c \
    src/libbluray/bdnav/navigation.c \
    src/libbluray/bdnav/clpi_parse.c \
    src/libbluray/bdnav/meta_parse.c \
    src/libbluray/bdnav/sound_parse.c \
    src/libbluray/bdnav/index_parse.c \
    src/libbluray/bdnav/bdid_parse.c \
    src/libbluray/bdnav/mpls_parse.c \
    src/libbluray/bdnav/extdata_parse.c \
    src/libbluray/hdmv/hdmv_vm.c \
    src/libbluray/hdmv/mobj_parse.c \
    src/libbluray/hdmv/mobj_print.c \
    src/libbluray/disc/disc.c \
    src/libbluray/disc/dec.c \
    src/libbluray/disc/bdplus.c \
    src/libbluray/disc/aacs.c \
    src/file/file.c \
    src/file/dir_posix.c \
    src/file/file_posix.c \
    src/file/mount.c \
    src/file/dl_posix.c \
    src/util/logging.c \
    src/util/strutl.c \
    src/util/mutex.c \
    src/util/array.c \
    src/util/refcnt.c \
    src/util/time.c \
    src/util/bits.c

LOCAL_C_INCLUDES := \
    $(LOCAL_SRC_DIR) \
    $(LOCAL_LIBBLURAY_DIR)

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libdl

# LOCAL_STATIC_LIBRARIES :=

LOCAL_MODULE := libbluray

include $(BUILD_SHARED_LIBRARY)
