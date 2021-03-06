LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftADPCM.cpp

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils liblog
  include  $(AMLOGIC_FRAMEWORKS_AV_CONFIG_MK)
  LOCAL_SHARED_LIBRARIES += libmedia_amlogic_support
LOCAL_MODULE := libstagefright_soft_adpcmdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
