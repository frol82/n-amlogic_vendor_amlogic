LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= am_dmx_test.c

LOCAL_MODULE:= am_dmx_test

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp $(LOCAL_PATH)/../../android/ndk/include

#LOCAL_STATIC_LIBRARIES := libam_adp
LOCAL_SHARED_LIBRARIES := libam_adp liblog libdl libc
LOCAL_32_BIT_ONLY := true

include $(BUILD_EXECUTABLE)
