LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(BOARD_AML_MEDIA_HAL_CONFIG)
LOCAL_SRC_FILES:= \
        sink/LinearRegression.cpp       \
        sink/RTPSink.cpp                \
        sink/TunnelRenderer.cpp         \
        sink/WifiDisplaySink.cpp        \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \
        $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol \
        $(TOP)/frameworks/native/include/media/hardware \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \
        libhardware        \
        libsystemcontrolservice\


LOCAL_MODULE:= libstagefright_wfd_sink

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
