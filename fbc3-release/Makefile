ifeq ($(ARC_COMPILER), GNU)
  include makefile_gnu_main
else
  ifeq ($(ARC_COMPILER), METAWARE)
    include makefile_metaware_main
  endif
endif

include ./version.mk
PRODUCT = fbc-main
VERSION_FILE = version_autogenarated.h

LD_LIB_PATH=$(ARC_TOOLCHAIN_PATH)/../lib/a6
CC_FLAGS+= -I./include/vpp/
CC_FLAGS+= -I./project/include/$(board)/
CC_FLAGS+= -I./project/include/
ASM_FLAGS+= -I./include/vpp/
ASM_FLAGS+= -I./project/include/

CC_FLAGS += -DIN_FBC_MAIN_CONFIG

include project/$(board)/config.mk

ifeq ($(ENABLE_AUTO_BACKLIGHT_CONTROL), 1)
    CC_FLAGS += -DENABLE_AUTO_BACKLIGHT
endif

ifeq ($(CONFIG_LOCAL_DIMMING), 1)
    CC_FLAGS += -DENABLE_LOCAL_DIMMING
endif

ifeq ($(ENABLE_IW7019_CTRL), 1)
    CC_FLAGS += -DENABLE_IW7019
endif

ifeq ($(ENABLE_IW7027_CTRL), 1)
    CC_FLAGS += -DENABLE_IW7027
endif

ifeq ($(CONFIG_HAVE_PRO_LOGO), 1)
    CC_FLAGS += -DHAVE_PRO_LOGO
endif

ASM_FLAGS += -DIN_FBC_MAIN_CONFIG

SSRCS	= driver/vtable.s

MAIN    = main.c
SRCS	= common/printf.c
SRCS	+= common/console.c
SRCS	+= common/command.c
SRCS	+= common/cmd_test.c
SRCS	+= common/cmd_suspend.c
SRCS	+= common/cmd_debug.c
SRCS	+= common/dbg_task.c
SRCS	+= common/cmd_reboot.c
SRCS	+= common/cmd_sf.c
SRCS	+= common/test_running_spi.c
SRCS	+= common/build_version.c
SRCS	+= common/cmd_cri.c
SRCS	+= common/cmd_panel.c
ifeq ($(ENABLE_AUTO_BACKLIGHT_CONTROL), 1)
    SRCS += common/auto_backlight.c
endif

ifneq ($(CONFIG_CUSTOMER_PROTOCOL), 1)
SRCS    += common/cmd_parameters.c
endif

PRJ_SRCS    += boot/relocate.c
PRJ_SRCS    += project/user_setting.c
PRJ_SRCS	+= project/ui.c
PRJ_SRCS    += project/customer_key_conf.c
PRJ_SRCS	+= project/v_protocol.c
PRJ_SRCS    += project/config.c

include project/$(board)/build.mk
OBJS     = $(SSRCS:%.s=%.o$(SUFFIX)) $(MAIN:%.c=%.o$(SUFFIX)) $(SRCS:%.c=%.o$(SUFFIX)) $(PRJ_SRCS:%.c=%.o$(SUFFIX))

export CC_FLAGS_ADD
export LD_FLAGS_ADD

main_mem_code_size = $(shell cat mm_size)
main_spi_code_size = $(shell cat ms_size)


all : main.elf
main_libs:=fbc.a

add_flags:
	$(eval CC_FLAGS_ADD = -Ml)
	$(eval ASM_FLAGS_ADD = -Ml)
main.elf: version.info add_flags $(OBJS)
	$(LD) $(OBJS) $(main_libs) $(LD_LIB_PATH)/le/crt1.o $(LD_LIB_PATH)/le/crti.o $(LD_FLAGS) main.cmd -e _start -o $@
	$(DUMPELF) $(DUMP_FLAGS) $@ > main.asm
sections_info.asm : main.elf
	$(DUMPELF) -s -q main.elf > sections_info.asm

mem_code_size : sections_info.asm
	./get_section_size.sh sections_info.asm mm_size 10 .start .text .fini .init .plt

spi_code_size : sections_info.asm
	./get_section_size.sh sections_info.asm ms_size 1091264512 .running.on.spi .spi.text

spi_main: clean main.elf mem_code_size spi_code_size
	$(CP) main.elf rom.elf
	$(STRIP) $(STRIP_FALGS) rom.elf
	$(DUMPELF) $(DUMP_FLAGS) rom.elf > rom.asm
	$(ELF2BIN) rom.elf rom_code_orig.bin -b0x0 -t$(main_mem_code_size)
	$(ELF2BIN) rom.elf rs_code_orig.bin -b0x410B6000 -t$(main_spi_code_size)
	$(OBJCOPY) $(COPY_FLAGS) rom.elf -o rom_rodata_orig.bin -cl
	$(OBJCOPY) $(COPY_FLAGS) rom.elf -o rom_data_orig.bin -cd
	$(CP) ./project/$(board)/pq/$(PANEL_PQ) ./project/rom_pqparam
	$(MK) -C tool all
	$(MK) -f makefile.rom

spi_boot:
	$(MK) -C boot clean
	$(MK) -C boot rom

PARTITION_INFO_SIZE=512
SECTION_INFO_SIZE=4096
SECTION_SIZE=708608
KEY_SIZE=266240
spi_header:
	bash openssl_operate.sh boot/first_boot_code.bin  boot/first_boot_data.bin
	gcc tool/add_partition_info.c -I project/include/$(board)/ -o add_partition_info
	./add_partition_info -o key.bin -s $(KEY_SIZE)
	./add_partition_info -i boot/first_boot_code.bin -i boot/first_boot_data.bin -o pi_first_boot.bin -s $(SECTION_INFO_SIZE)
	./add_partition_info -i boot/second_boot_code.bin -i boot/second_boot_data.bin -o pi_second_boot.bin -s $(PARTITION_INFO_SIZE)
	./add_partition_info -i boot/suspend_code.bin -i boot/suspend_data.bin -o pi_suspend.bin -s $(PARTITION_INFO_SIZE)
	./add_partition_info -i boot/update_code.bin -i boot/update_data.bin -o pi_update.bin -s $(PARTITION_INFO_SIZE)
	./add_partition_info -i rom.bin -o pi_main.bin  -s $(PARTITION_INFO_SIZE)
	./tool/bin_op -i project/$(board)/pq -t pq -o pq.bin
	./add_partition_info -i pq.bin -o pi_pq.bin -s $(PARTITION_INFO_SIZE)
	./add_partition_info -o pi_user.bin -s $(PARTITION_INFO_SIZE)
	./add_partition_info -o pi_factory.bin -s $(PARTITION_INFO_SIZE)
	cat pi_second_boot.bin pi_suspend.bin pi_update.bin pi_main.bin pi_pq.bin pi_user.bin pi_factory.bin >> pi_section_0.bin
	head -c $(PARTITION_INFO_SIZE) /dev/zero >> pi_section_0.bin

	@echo "generate head.sha256 that write efuse, 520 = sizeof (struct rsa_public_key)"
	dd if=key.bin of=rsa_key.bin bs=520 count=1
	openssl dgst -sha256 -binary -out rsa_key/head.sha256 rsa_key.bin

spi: spi_main spi_boot spi_header
	cat boot/second_boot.bin boot/suspend.bin boot/update.bin rom.bin >> section_0.bin
	cat key.bin pi_first_boot.bin pi_section_0.bin pi_section_0.bin boot/first_boot.bin section_0.bin section_0.bin >> spi.bin
	cat spi.bin pq.bin >> spi_pq.bin
	./tool/bin_op -i spi_pq.bin -t all -o spi_all.bin
	$(MKDIR) out
	@split -b 64k -d spi.bin ./out/spi.bin.
	bin2hex spi.bin 1 > spi.hex

version.info:
	@$(RM) include/$(VERSION_FILE)
	@echo "#define PRODUCT_NAME \"$(PRODUCT)\"" > include/$(VERSION_FILE)
	@echo "#define FBC_VERSION_MAIN $(FBC_VERSION_MAIN)" >> include/$(VERSION_FILE)
	@echo "#define FBC_VERSION_SUB1 $(FBC_VERSION_SUB1)" >> include/$(VERSION_FILE)
	@echo "#define FBC_VERSION_SUB2 $(FBC_VERSION_SUB2)" >> include/$(VERSION_FILE)
	@echo "#define RELEASE_DATE \"$(RELEASE_DATE)\"" >> include/$(VERSION_FILE)
	@echo "#define RELEASE_TIME \"$(RELEASE_TIME)\"" >> include/$(VERSION_FILE)
	@echo "#define RELEASE_USER \"$(RELEASE_USER)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_BOOT_VERSION \"$(PRODUCT) $(VERSION) (RELEASE:$(RELEASE_USER) COMPILER:$(ARC_COMPILER)) $(RELEASE_DATE) $(RELEASE_TIME)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_GIT_BRANCH \"$(FBC_GIT_BRANCH)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_GIT_COMMIT \"$(FBC_GIT_COMMIT)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_GIT_UNCOMMIT_FILE_NUM \"$(FBC_GIT_UNCOMMIT_FILE_NUM)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_GIT_LAST_CHANGED \"$(FBC_GIT_LAST_CHANGED)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_BUILD_TIME \"$(FBC_BUILD_TIME)\"" >> include/$(VERSION_FILE)
	@echo "#define FBC_BUILD_NAME \"$(FBC_BUILD_NAME)\"" >> include/$(VERSION_FILE)
	@echo "$(FBC_BUILD_TIME)" > ./tool/version.dat

clean:
	@$(RM) include/$(VERSION_FILE)
	@$(RM) tool/*.dat
	@$(RM) $(OBJS)
	@$(RM) *.elf
	@$(RM) *.bin
	@$(RM) *.bin.*
	@$(RM) *.hex
	@$(RM) *.asm
	@$(RM) mm_size
	@$(RM) ms_size
	@$(RM) rom_map.*
	@$(RM) ./project/rom_pqparam
	$(MK) -C boot clean
	$(MK) -C tool clean
	@$(RM) -r ./out
	@$(RM) -rf rsa_key/signature2048_1* rsa_key/head.sha256
	@$(RM) add_partition_info

distclean:clean

.c.o:
	$(CC) $(CC_FLAGS) $< -o $@

.s.o:
	$(ASM) $(ASM_FLAGS) $< -o $@
