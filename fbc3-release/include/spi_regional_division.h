#ifndef SPI_REGIONAL_DIVISION_H
#define SPI_REGIONAL_DIVISION_H


#define SPI_BASE 0x41000000

#define RSA_KEY_OFFSET 0
#define RSA_KEY_SIZE 0x1000

#define HDCP_KEY_OFFSET 0x1000
#define HDCP_KEY_SIZE 0x3F000

#define LAYOUT_VERSION_OFFSET 0x40000
#define LAYOUT_VERSION_SIZE 0x1000

#define PARTITION_INFO_SIZE 0x200
#define FIRST_BOOT_INFO_OFFSET 0x41000 //for rom-boot
#define FIRST_BOOT_INFO_SIZE 0x1000

#define SECTION_INFO_SIZE 0x1000
#define SECTION_0_INFO_OFFSET 0x42000
#define SECTION_1_INFO_OFFSET 0x43000

#define FIRST_BOOT_OFFSET 0x44000
#define FIRST_BOOT_SIZE 0x5000

#define SECTION_SIZE 0xAD000 //692K
#define SECTION_0_OFFSET 0x49000
#define SECTION_1_OFFSET 0xF6000


//section
enum {
	SECTION_0,
	SECTION_1,
};

//partition
enum {
	PARTITION_FIRST_BOOT = 0,
	PARTITION_SECOND_BOOT,
	PARTITION_SUSPEND,
	PARTITION_UPDATE,
	PARTITION_MAIN,
	PARTITION_PQ,
	PARTITION_USER,
	PARTITION_FACTORY,
	PARTITION_NUM,
};


#define SIGNATURE_SIZE 256
typedef struct {
	unsigned code_offset;
	unsigned code_size;
	unsigned data_offset;
	unsigned data_size;
	unsigned bss_offset;
	unsigned bss_size;
	unsigned readonly_offset;
	unsigned readonly_size;
	unsigned char signature[SIGNATURE_SIZE];
	unsigned spi_code_offset;
	unsigned spi_code_size;
	unsigned audio_param_offset;
	unsigned audio_param_size;
	unsigned sys_param_offset;
	unsigned sys_param_size;
	unsigned crc;
	unsigned char sha[32];
	unsigned char name[32];
} partition_info_t;

#endif
