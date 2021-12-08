LOCAL_PATH:= $(call my-dir)

common_src_files := \
	DongleManager.cpp \
	NetlinkManager.cpp \
	NetlinkHandler.cpp \
	UsbMdmMgr.cpp \
	UsbModem.cpp

common_c_includes := \
	system/extras/ext4_utils \
	system/extras/f2fs_utils \
	external/openssl/include \
	external/stlport/stlport \
	bionic \
	external/scrypt/lib/crypto \
	frameworks/native/include \
	system/security/keystore \
	hardware/libhardware/include/hardware \
	system/security/softkeymaster/include/keymaster

common_shared_libraries := \
	libsysutils \
	libstlport \
	libcutils \
	liblog \
	libhardware_legacy \
	liblogwrap \
	libext4_utils \
	libcrypto \
	libselinux \
	libutils \
	libhardware

common_static_libraries := \
	libscrypt_static \
	libmincrypt

include $(CLEAR_VARS)

LOCAL_MODULE:= modem_dongle_d

LOCAL_SRC_FILES := \
	main.cpp \
	$(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

LOCAL_CFLAGS := -Werror=format

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_STATIC_LIBRARIES := $(common_static_libraries)

include $(BUILD_EXECUTABLE)

