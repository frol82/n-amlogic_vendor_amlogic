LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libsubjni
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := sub_jni.c sub_api.c log_print.c sub_subtitle.c sub_vob_sub.c sub_set_sys.c vob_sub.c sub_pgs_sub.c sub_control.c avi_sub.c sub_dvb_sub.c amsysfsutils.c Amsyswrite.cpp MemoryLeakTrackUtilTmp.cpp sub_socket.cpp sub_io.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
    $(TOP)/frameworks/native/services \
    $(TOP)/frameworks/native/include

LOCAL_SHARED_LIBRARIES += libutils libmedia libcutils libbinder libsystemcontrolservice liblog

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
