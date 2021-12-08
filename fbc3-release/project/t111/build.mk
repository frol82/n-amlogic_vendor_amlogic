PRJ_SRCS    += project/t111/power.c
PRJ_SRCS    += project/t111/vpp_cmd.c
PRJ_SRCS    += project/t111/panel.c
PRJ_SRCS    += project/t111/io_config.c
ifeq ($(CONFIG_LOCAL_DIMMING), 1)
PRJ_SRCS  += project/t111/ldim_init.c
endif
