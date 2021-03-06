LOCAL_PATH := $(call my-dir)
AMLOGIC_LIBPLAYER :=y

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 25)))
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include\
       hardware/amlogic/LibAudio/amadec/include
AMADEC_LIBS:=libamadec
else
AMADEC_C_INCLUDES:=vendor/amlogic/frameworks/av/LibPlayer/amcodec/include\
       vendor/amlogic/frameworks/av/LibPlayer/amadec/include
AMADEC_LIBS:=libamplayer
endif

include $(CLEAR_VARS)

LOCAL_MODULE    := libam_adp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c\
		   am_av/am_av.c am_av/aml/aml.c\
		   am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
		   am_dmx/dvr/dvr.c\
		   am_aout/am_aout.c\
		   am_vout/am_vout.c\
		   am_vout/aml/aml.c\
		   am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c\
		   am_time/am_time.c\
		   am_evt/am_evt.c\
		   am_kl/am_kl.c \
		   am_dsc/am_dsc.c am_dsc/aml/aml.c\
		   am_smc/am_smc.c\
		   am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
           am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
           am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
           am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
           am_open_lib/am_ci/libdvben50221/en50221_session.c       \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
           am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		    $(LOCAL_PATH)/am_open_lib/am_ci\
		    $(LOCAL_PATH)/../include/am_mw\
		    $(LOCAL_PATH)/../android/ndk/include\
			$(AMADEC_C_INCLUDES)\
		    external/icu/icu4c/source/common\
		    common/include/linux/amlogic

LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libcutils liblog libdl libc

LOCAL_PRELINK_MODULE := false

#LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE    := libam_adp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_dmx/am_dmx.c am_dmx/linux_dvb/linux_dvb.c\
		   am_fend/am_fend.c am_fend/am_fend_diseqc_cmd.c am_fend/am_rotor_calc.c am_fend/linux_dvb/linux_dvb.c\
	           am_av/am_av.c am_av/aml/aml.c\
	           am_dvr/am_dvr.c am_dvr/linux_dvb/linux_dvb.c\
	           am_dmx/dvr/dvr.c\
	           am_aout/am_aout.c\
	           am_vout/am_vout.c\
	           am_vout/aml/aml.c\
	           am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_iconv.c\
	           am_time/am_time.c\
	           am_evt/am_evt.c\
			   am_kl/am_kl.c\
	           am_dsc/am_dsc.c am_dsc/aml/aml.c\
	           am_smc/am_smc.c\
	           am_smc/aml/aml.c\
		   am_userdata/am_userdata.c\
		   am_userdata/aml/aml.c\
		   am_userdata/emu/emu.c\
		   am_pes/am_pes.c \
		   am_ad/am_ad.c \
		   am_open_lib/libdvbsi/tables/bat.c\
		   am_open_lib/libdvbsi/tables/sdt.c\
		   am_open_lib/libdvbsi/tables/pat.c\
		   am_open_lib/libdvbsi/tables/cat.c\
		   am_open_lib/libdvbsi/tables/pmt.c\
		   am_open_lib/libdvbsi/tables/tot.c\
		   am_open_lib/libdvbsi/tables/eit.c\
		   am_open_lib/libdvbsi/tables/nit.c\
		   am_open_lib/libdvbsi/demux.c\
		   am_open_lib/libdvbsi/descriptors/dr_0f.c\
		   am_open_lib/libdvbsi/descriptors/dr_44.c\
		   am_open_lib/libdvbsi/descriptors/dr_0a.c\
		   am_open_lib/libdvbsi/descriptors/dr_47.c\
		   am_open_lib/libdvbsi/descriptors/dr_03.c\
		   am_open_lib/libdvbsi/descriptors/dr_5a.c\
		   am_open_lib/libdvbsi/descriptors/dr_05.c\
		   am_open_lib/libdvbsi/descriptors/dr_48.c\
		   am_open_lib/libdvbsi/descriptors/dr_69.c\
		   am_open_lib/libdvbsi/descriptors/dr_02.c\
		   am_open_lib/libdvbsi/descriptors/dr_4d.c\
		   am_open_lib/libdvbsi/descriptors/dr_58.c\
		   am_open_lib/libdvbsi/descriptors/dr_56.c\
		   am_open_lib/libdvbsi/descriptors/dr_4e.c\
		   am_open_lib/libdvbsi/descriptors/dr_4a.c\
		   am_open_lib/libdvbsi/descriptors/dr_45.c\
		   am_open_lib/libdvbsi/descriptors/dr_41.c\
		   am_open_lib/libdvbsi/descriptors/dr_43.c\
		   am_open_lib/libdvbsi/descriptors/dr_0e.c\
		   am_open_lib/libdvbsi/descriptors/dr_04.c\
		   am_open_lib/libdvbsi/descriptors/dr_59.c\
		   am_open_lib/libdvbsi/descriptors/dr_0c.c\
		   am_open_lib/libdvbsi/descriptors/dr_54.c\
		   am_open_lib/libdvbsi/descriptors/dr_09.c\
		   am_open_lib/libdvbsi/descriptors/dr_52.c\
		   am_open_lib/libdvbsi/descriptors/dr_40.c\
		   am_open_lib/libdvbsi/descriptors/dr_55.c\
		   am_open_lib/libdvbsi/descriptors/dr_08.c\
		   am_open_lib/libdvbsi/descriptors/dr_0b.c\
		   am_open_lib/libdvbsi/descriptors/dr_42.c\
		   am_open_lib/libdvbsi/descriptors/dr_07.c\
		   am_open_lib/libdvbsi/descriptors/dr_0d.c\
		   am_open_lib/libdvbsi/descriptors/dr_06.c\
		   am_open_lib/libdvbsi/descriptors/dr_83.c\
		   am_open_lib/libdvbsi/descriptors/dr_86.c\
		   am_open_lib/libdvbsi/descriptors/dr_87.c\
		   am_open_lib/libdvbsi/descriptors/dr_88.c\
		   am_open_lib/libdvbsi/descriptors/dr_5d.c\
		   am_open_lib/libdvbsi/descriptors/dr_6a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7a.c\
		   am_open_lib/libdvbsi/descriptors/dr_7f.c\
		   am_open_lib/libdvbsi/psi.c\
		   am_open_lib/libdvbsi/dvbpsi.c\
		   am_open_lib/libdvbsi/descriptor.c\
		   am_open_lib/am_ci/libdvben50221/asn_1.c \
           am_open_lib/am_ci/libdvben50221/en50221_app_ai.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_auth.c      \
           am_open_lib/am_ci/libdvben50221/en50221_app_ca.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_datetime.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_dvb.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_epg.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_lowspeed.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_mmi.c       \
           am_open_lib/am_ci/libdvben50221/en50221_app_rm.c        \
           am_open_lib/am_ci/libdvben50221/en50221_app_smartcard.c \
           am_open_lib/am_ci/libdvben50221/en50221_app_teletext.c  \
           am_open_lib/am_ci/libdvben50221/en50221_app_utils.c     \
           am_open_lib/am_ci/libdvben50221/en50221_session.c       \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam.c        \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam_hlci.c   \
           am_open_lib/am_ci/libdvben50221/en50221_stdcam_llci.c   \
           am_open_lib/am_ci/libdvben50221/en50221_transport.c \
		   am_open_lib/am_ci/libucsi/dvb/types.c \
		   am_open_lib/am_ci/libdvbapi/dvbca.c \
		   am_open_lib/am_ci/libucsi/mpeg/pmt_section.c \
		   am_open_lib/am_ci/am_ci.c \
		   am_open_lib/am_ci/ca_ci.c \
		   am_open_lib/am_freesat/freesat.c



LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND -DLOG_LEVEL=1
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_adp\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		    $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		    $(LOCAL_PATH)/am_open_lib/am_ci\
		    $(LOCAL_PATH)/../include/am_mw\
		    $(LOCAL_PATH)/../android/ndk/include\
			$(AMADEC_C_INCLUDES)\
		    external/icu/icu4c/source/common\
		    common/include/linux/amlogic

LOCAL_SHARED_LIBRARIES+=$(AMADEC_LIBS) libcutils liblog libdl libc

LOCAL_PRELINK_MODULE := false

#LOCAL_32_BIT_ONLY := true

include $(BUILD_STATIC_LIBRARY)


