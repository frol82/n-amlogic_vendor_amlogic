#include <spi_flash.h>
#include <board_config.h>
#include <crc.h>
#include <relocate.h>


partition_info_t* get_partition_info(int section, int partition)
{
	unsigned int offset;
	partition_info_t *info;

	if (partition == PARTITION_FIRST_BOOT)
		offset = FIRST_BOOT_INFO_OFFSET;

	else if ((section < 2) && (partition < PARTITION_NUM)) {
		offset = section ? SECTION_1_INFO_OFFSET : SECTION_0_INFO_OFFSET;
		offset += PARTITION_INFO_SIZE * (partition - 1);
	}
	else {
		printf(" unknown secition(%d) or partiton(%d)!\n", section, partition);
		return 0;
	}
	info = (partition_info_t *)(SPI_BASE + offset);
	printf("setction %d partition %d crc=0x%x\n", section, partition, info->crc);
	printf("code_offset=0x%x, code_size=0x%x\n", info->code_offset, info->code_size);
	printf("data_offset=0x%x, data_size=0x%x\n", info->data_offset, info->data_size);
//	printf("bss_offset=0x%x, bss_size=0x%x\n", info->bss_offset, info->bss_size);
//	printf("readonly_offset=0x%x, readonly_size=0x%x\n", info->readonly_offset, info->readonly_size);
//	printf("audio_param_offset=0x%x, audio_param_size=0x%x\n", info->audio_param_offset, info->audio_param_size);
//	printf("sys_param_offset=0x%x, sys_param_size=0x%x\n", info->sys_param_offset, info->sys_param_size);
	return info;
}

int check_partition(partition_info_t *info)
{
	unsigned int crc = 0;
	int crc_flag = 0;

	if (info) {
		if (info->code_offset && info->code_size && (info->code_offset < CONFIG_SPI_SIZE) && (info->code_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->code_offset), info->code_size);
			crc_flag = 1;
		}
		if (info->data_offset && info->data_size && (info->data_offset < CONFIG_SPI_SIZE) && (info->data_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->data_offset), info->data_size);
			crc_flag = 1;
		}
		if (info->spi_code_offset && info->spi_code_size && (info->spi_code_offset < CONFIG_SPI_SIZE) && (info->spi_code_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->spi_code_offset), info->spi_code_size);
			crc_flag = 1;
		}
		if (info->readonly_offset && info->readonly_size && (info->readonly_offset < CONFIG_SPI_SIZE) && (info->readonly_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->readonly_offset), info->readonly_size);
			crc_flag = 1;
		}
		if (info->audio_param_offset && info->audio_param_size && (info->audio_param_offset < CONFIG_SPI_SIZE) && (info->audio_param_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->audio_param_offset), info->audio_param_size);
			crc_flag = 1;
		}
		if (info->sys_param_offset && info->sys_param_size && (info->sys_param_offset < CONFIG_SPI_SIZE) && (info->sys_param_size < CONFIG_SPI_SIZE)) {
			crc = crc32(crc, (void *)(SPI_BASE + info->sys_param_offset), info->sys_param_size);
			crc_flag = 1;
		}
		printf("cal crc=0x%x\n", crc);
		if (crc_flag && (crc == info->crc))
			return 0;
	}
	return -1;
}

/* return: 0-legal, -1-illegal */
int illegal_address_check(unsigned offset, unsigned size)
{
	unsigned end = offset + size - 1;

	if ((offset >= SECTION_0_INFO_OFFSET)
	&& (end < (SECTION_0_INFO_OFFSET + SECTION_INFO_SIZE)))
		return 0;

	if ((offset >= SECTION_0_OFFSET)
	&& (end < (SECTION_0_OFFSET + SECTION_SIZE)))
		return 0;

	if ((offset >= PQ_BINARY_START)
	&& (end < FBC_FACTORY_START))
		return 0;

	return -1;
}

int copy_partition_to_sram2(int section, int partition)
{
	void *des, *src;
	partition_info_t *info = get_partition_info(section, partition);

	if (check_partition(info)) {
		printf("partition crc error, copy to SRAM2 failed!\n");
		return -1;
	}
	des = (void *)(SRAM2_BASE);
	src = (void *)(SPI_BASE + info->code_offset);
	memcpy(des, src, info->code_size);

	des = (void *)(SRAM2_BASE + info->code_size);
	src = (void *)(SPI_BASE + info->data_offset);
	memcpy(des, src, info->data_size);
	return 0;
}


int copy_partition_to_ccm(int section, int partition)
{
	void *des, *src;
	partition_info_t *info = get_partition_info(section, partition);

	if (check_partition(info)) {
		printf("partition crc error, copy to CCM failed!\n");
		return -1;
	}
	des = (void *)(ICCM_BASE);
	src = (void *)(SPI_BASE + info->code_offset);
	memcpy(des, src, info->code_size);

	des = (void *)(DCCM_BASE);
	src = (void *)(SPI_BASE + info->data_offset);
	memcpy(des, src, info->data_size);
	return 0;
}


int move_image(
			struct spi_flash *flash,
			unsigned s_offs,
			unsigned b_offs,
			unsigned size)
{
	char buf[SPI_BLOCK_SIZE];
	char rbbuf[SPI_BLOCK_SIZE];
	int block_num;

	block_num = size / SPI_BLOCK_SIZE;
	if (size % SPI_BLOCK_SIZE)
		block_num++;
	printf("move image: block_num=%d\n", block_num);
	int i=0;
	while (block_num--) {
		printf("%d: s_offs=0x%x, b_offs=0x%x\n", i, s_offs, b_offs);
		i++;
		spi_flash_erase(flash, b_offs, SPI_BLOCK_SIZE);
		spi_flash_read(flash, s_offs, SPI_BLOCK_SIZE, buf);
		spi_flash_write(flash, b_offs, SPI_BLOCK_SIZE, buf);
		spi_flash_read(flash, b_offs, SPI_BLOCK_SIZE, rbbuf);
		if (memcmp(rbbuf, buf, SPI_BLOCK_SIZE))
			return -1;
		s_offs += SPI_BLOCK_SIZE;
		b_offs += SPI_BLOCK_SIZE;
	}

	return 0;
}
/*
static void print_spi(struct spi_flash *flash, unsigned offset, unsigned size)
{
	#define LINE_SIZE 16
	unsigned char buf[LINE_SIZE];
	int i, line = size / LINE_SIZE;

	if (size % LINE_SIZE)
		line++;
	for (i=0; i<line; i++) {
		spi_flash_read(flash, offset, LINE_SIZE, buf);
		printf ("0x%8x: %2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,\n",
		offset, buf[0], buf[1],buf[2], buf[3],buf[4], buf[5],buf[6], buf[7],
		buf[8], buf[9],buf[10], buf[11],buf[12], buf[13],buf[14], buf[15]);
		offset += LINE_SIZE;
	}
}
*/
int section_backup(struct spi_flash *flash)
{
	int ret;

	printf("backup section...\n");
	ret = move_image(flash, SECTION_0_OFFSET, SECTION_1_OFFSET, SECTION_SIZE);
	if (!ret)
		ret = move_image(flash, SECTION_0_INFO_OFFSET, SECTION_1_INFO_OFFSET, SECTION_INFO_SIZE);
	if (ret)
		printf("backup section failed!\n");
	else
		printf("backup section success!\n");
	return ret;
}


int section_restore(struct spi_flash *flash)
{
	partition_info_t info;
	int ret;

	printf("restore section...\n");
	info = *get_partition_info(SECTION_1, PARTITION_SECOND_BOOT);
	info.code_offset += SECTION_SIZE;
	info.data_offset += SECTION_SIZE;
	//print_spi(flash, info.code_offset, 0xb000);
	if (check_partition(&info)) {
		printf("partition crc error, restore section failed!\n");
		return -1;
	}
	ret = move_image(flash, SECTION_1_OFFSET, SECTION_0_OFFSET, SECTION_SIZE);
	if (!ret)
		ret = move_image(flash, SECTION_1_INFO_OFFSET, SECTION_0_INFO_OFFSET, SECTION_INFO_SIZE);
	if (ret)
		printf("restore section failed!\n");
	else
		printf("restore section success!\n");
	return ret;
}
