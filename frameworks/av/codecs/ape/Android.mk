LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        apedec.c    \
	SoftApe.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/     \
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax \
        $(LOCAL_PATH)/../include

ifeq ($(ARCH_ARM_HAVE_NEON),true)
	LOCAL_CFLAGS += -D__ARM_HAVE_NEON -DOPT_NEON
endif

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx \
        libstagefright_foundation libutils liblog
include  $(AMLOGIC_FRAMEWORKS_AV_CONFIG_MK)
LOCAL_SHARED_LIBRARIES += libmedia_amlogic_support
LOCAL_MODULE := libstagefright_soft_apedec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

