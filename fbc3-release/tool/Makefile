GCC = gcc

OBJ_TOOLS = audio_param
OBJ_TOOLS += bin_wrapper
OBJ_TOOLS += bin_op

TOOLS_SRCS = audio_param.c
TOOLS_SRCS += bin_wrapper.c
TOOLS_SRCS += bin_op.c
TOOLS_SRCS += add_partition_info.c

CODESIZE = $(shell echo `stat -c %s ./../rom_code_orig.bin`)
DATASIZE = $(shell echo `stat -c %s ./../rom_data_orig.bin`)

CFLAGS+= -I../project/include/$(board)/	\
			-I../include/	\

include ../project/$(board)/config.mk


audio_param: audio_param.o
	$(GCC) $^ -o $@

bin_wrapper: bin_wrapper.o
	$(GCC) $^ -o $@

config2bin: config2bin.o
	$(GCC) $^ -o $@

rom_map:
	echo "$(CODESIZE)" > rom_map.dat
	echo "$(DATASIZE)" >> rom_map.dat

all: clean rom_map $(OBJ_TOOLS)

clean:
	@$(RM) *.o
	@$(RM) rom_audioparam
	@$(RM) $(OBJ_TOOLS)

.c.o:
	$(GCC) -c $<