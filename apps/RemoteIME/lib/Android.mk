LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
         $(call all-subdir-java-files) \
         com/droidlogic/inputmethod/remote/IPinyinDecoderService.aidl

LOCAL_MODULE := com.droidlogic.inputmethod.remote.lib

include $(BUILD_STATIC_JAVA_LIBRARY)
