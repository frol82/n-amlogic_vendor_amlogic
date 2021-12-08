LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    InstabootSupport.cpp

LOCAL_C_INCLUDES += system/core/fs_mgr/include \
        external/fw_env \
        system/extras/ext4_utils \
        system/vold \
        vendor/amlogic/frameworks/services/systemcontrol \
        vendor/amlogic/frameworks/tv/include

LOCAL_MODULE := libinstaboot_support

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libdl \
        libutils \
        liblog \
        libbinder \
        libsystemcontrolservice \
        libc \
        libtvplay

include $(BUILD_SHARED_LIBRARY)
