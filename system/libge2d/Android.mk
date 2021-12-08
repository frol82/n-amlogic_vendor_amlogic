LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ge2d_port.c
LOCAL_MODULE := libge2d
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/kernel-headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include $(LOCAL_PATH)/kernel-headers
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := ge2d_feature_test
LOCAL_SRC_FILES:= aml_ge2d.c IONmem.c ge2d_feature_test.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/kernel-headers \
	system/core/libion/include/ \
	system/core/libion/kernel-headers \
	system/core/include/ion

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libge2d \
	libion

LOCAL_CFLAGS +=-g
LOCAL_CPPFLAGS := -g

LOCAL_MODULE := ge2d_feature_test
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false


LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := ge2d_load_test
LOCAL_SRC_FILES:= aml_ge2d.c IONmem.c ge2d_load_test.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/kernel-headers \
	system/core/libion/include/ \
	system/core/libion/kernel-headers \
	system/core/include/ion

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libge2d \
	libion

LOCAL_CFLAGS +=-g
LOCAL_CPPFLAGS := -g

LOCAL_MODULE := ge2d_load_test
include $(BUILD_EXECUTABLE)
