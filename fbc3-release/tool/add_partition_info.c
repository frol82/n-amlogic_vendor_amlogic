
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <board_config.h>
#include "../include/spi_regional_division.h"
#include "../rsa_key/signature2048_1.h"
#include "../include/version_autogenarated.h"

//SECTION_FIRST_BOOT
#define FIRST_BOOT_CODE_OFFSET FIRST_BOOT_OFFSET
#define FIRST_BOOT_CODE_SIZE (1024 * 16)
#define FIRST_BOOT_DATA_OFFSET (FIRST_BOOT_CODE_OFFSET + FIRST_BOOT_CODE_SIZE)
#define FIRST_BOOT_DATA_SIZE (1024 * 4)

//SECTION_0
#define SECOND_BOOT_CODE_OFFSET SECTION_0_OFFSET
#define SECOND_BOOT_CODE_SIZE (1024 * 38)
#define SECOND_BOOT_DATA_OFFSET (SECOND_BOOT_CODE_OFFSET + SECOND_BOOT_CODE_SIZE)
#define SECOND_BOOT_DATA_SIZE (1024 * 6)

#define SUSPEND_CODE_OFFSET (SECOND_BOOT_DATA_OFFSET + SECOND_BOOT_DATA_SIZE)
#define SUSPEND_CODE_SIZE (1024 * 20)
#define SUSPEND_DATA_OFFSET (SUSPEND_CODE_OFFSET + SUSPEND_CODE_SIZE)
#define SUSPEND_DATA_SIZE (1024 * 4)

#define UPDATE_CODE_OFFSET (SUSPEND_DATA_OFFSET + SUSPEND_DATA_SIZE)
#define UPDATE_CODE_SIZE (1024 * 96)
#define UPDATE_DATA_OFFSET (UPDATE_CODE_OFFSET + UPDATE_CODE_SIZE)
#define UPDATE_DATA_SIZE (1024 * 24)

#define MAIN_CODE_OFFSET (UPDATE_DATA_OFFSET + UPDATE_DATA_SIZE)
#define MAIN_CODE_SIZE (1024 * 192)
#define MAIN_DATA_OFFSET (MAIN_CODE_OFFSET + MAIN_CODE_SIZE)
#define MAIN_DATA_SIZE (1024 * 56)
#define MAIN_SPI_CODE_OFFSET (MAIN_DATA_OFFSET + MAIN_DATA_SIZE)
#define MAIN_SPI_CODE_SIZE (1024 * 64)
#define MAIN_RO_DATA_OFFSET (MAIN_SPI_CODE_OFFSET + MAIN_SPI_CODE_SIZE)
#define MAIN_RO_DATA_SIZE (1024 * 180)
#define MAIN_AUDIO_PARAM_OFFSET (MAIN_RO_DATA_OFFSET + MAIN_RO_DATA_SIZE)
#define MAIN_AUDIO_PARAM_SIZE (1024 * 8)
#define MAIN_SYS_PARAM_OFFSET (MAIN_AUDIO_PARAM_OFFSET + MAIN_AUDIO_PARAM_SIZE)
#define MAIN_SYS_PARAM_SIZE (1024 * 4)


struct rsa_public_key {
	/* Length of n[] in number of uint32_t */
	unsigned int len;
	/* modulus as little endian array */
	unsigned int n[64];
	/* R^2 as little endian array */
	unsigned int rr[64];
	/* -1 / n[0] mod 2^32 */
	unsigned int n0inv;
};

struct rsa_public_key rsa_key =
#include "../rsa_key/rsa2048_pkey_1.h"

static char *if_name=NULL;
static char *optstring = "i:o:t:s:";
static struct option long_options[] = {
	{"input", required_argument, NULL, 'i'},
	{"output", required_argument, NULL, 'o'},
	{"type", required_argument, NULL, 't'},
	{"size", required_argument, NULL, 's'},
};
/*
	./add_partition_info -o key.bin -s 266240
	./add_partition_info -i boot/first_boot_code.bin -i boot/first_boot_data.bin -o pi_first_boot.bin -s 4096
	./add_partition_info -i boot/second_boot_code.bin -i boot/second_boot_data.bin -o pi_second_boot.bin -s 512
	./add_partition_info -i boot/suspend_code.bin -i boot/suspend_data.bin -o pi_suspend.bin -s 512
	./add_partition_info -i boot/update_code.bin -i boot/update_data.bin -o pi_update.bin -s 512
	./add_partition_info -i rom_code.bin -i rom_data.bin -o pi_main.bin  -s 512
	./add_partition_info -o pi_pq.bin -s 512
	./add_partition_info -o pi_user.bin -s 5632
	cat key.bin pi_first_boot.bin pi_second_boot.bin pi_suspend.bin pi_update.bin pi_main.bin pi_pq.bin pi_user.bin >> par_info.bin
*/

/* Karl Malbrain's compact CRC-32. See "A compact
CCITT crc16 and crc32 C implementation that balances
processor cache usage against speed":
http://www.geocities.com/malbrain/ */
static unsigned int crc32 ( unsigned int crc, const unsigned char *ptr,
							unsigned int buf_len )
{
	static const unsigned int s_crc32[16] = {
		0, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	unsigned int crcu32 = crc;

	if ( !ptr ) {
		return 0;
	}

	crcu32 = ~crcu32;

	while ( buf_len-- ) {
		unsigned char b = *ptr++;
		crcu32 = ( crcu32 >> 4 ) ^ s_crc32[ ( crcu32 & 0xF ) ^ ( b & 0xF )];
		crcu32 = ( crcu32 >> 4 ) ^ s_crc32[ ( crcu32 & 0xF ) ^ ( b >> 4 )];
	}

	return ~crcu32;
}

int file_crc(char *file_name, unsigned int *crc)
{
	FILE *fd;
	unsigned char buf[4096];
	int len;

	fd = fopen ( file_name, "r" );
	if ( NULL == fd ) {
		printf ( "open %s failed!\n", file_name );
		return -1;
	}

	clearerr ( fd );
	while ( !feof(fd) ) {
		len = fread ( buf, sizeof(char), sizeof(buf), fd );
		*crc = crc32 ( *crc, buf, len );
		printf ( "%s: len=0x%x, crc=0x%x\n", file_name, len ,*crc);
		//if ( len < sizeof(buf) )
		//	break;
	}

	fclose ( fd );
	return 0;
}

static int getFileSize ( const char *file_name )
{
	int tmp_len = 0;
	FILE *fp = NULL;

	if ( file_name == NULL || strlen ( file_name ) == 0 ) {
		return 0;
	}

	fp = fopen ( file_name, "rb" );

	if ( fp == NULL ) {
		return 0;
	}

	fseek ( fp, 0, SEEK_END );
	tmp_len = ftell ( fp );
	fclose ( fp );
	fp = NULL;
	return tmp_len;
}

int get_partition(char *of_name)
{
	int partition =-1;

	if ( !strcmp ( "pi_first_boot.bin", of_name ) )
		partition = PARTITION_FIRST_BOOT;
	else if ( !strcmp ( "pi_second_boot.bin", of_name ) )
		partition = PARTITION_SECOND_BOOT;
	else if ( !strcmp ( "pi_suspend.bin", of_name ) )
		partition = PARTITION_SUSPEND;
	else if ( !strcmp ( "pi_update.bin", of_name ) )
		partition = PARTITION_UPDATE;
	else if ( !strcmp ( "pi_main.bin", of_name ) )
		partition = PARTITION_MAIN;
	else if ( !strcmp ( "pi_pq.bin", of_name ) )
		partition = PARTITION_PQ;
	else if ( !strcmp ( "pi_user.bin", of_name ) )
		partition = PARTITION_USER;
	else if ( !strcmp ( "pi_factory.bin", of_name ) )
		partition = PARTITION_FACTORY;

	return partition;
}

void set_partition_info( int partition, partition_info_t *info )
{
	switch ( partition ) {
		case PARTITION_FIRST_BOOT:
			info->code_offset = FIRST_BOOT_CODE_OFFSET;
			info->code_size = FIRST_BOOT_CODE_SIZE;
			info->data_offset = FIRST_BOOT_DATA_OFFSET;
			info->data_size = FIRST_BOOT_DATA_SIZE;
			strcpy(info->name, "first boot");
			break;
		case PARTITION_SECOND_BOOT:
			info->code_offset = SECOND_BOOT_CODE_OFFSET;
			info->code_size = SECOND_BOOT_CODE_SIZE;
			info->data_offset = SECOND_BOOT_DATA_OFFSET;
			info->data_size = SECOND_BOOT_DATA_SIZE;
			strcpy(info->name, "second boot");
			break;
		case PARTITION_SUSPEND:
			info->code_offset = SUSPEND_CODE_OFFSET;
			info->code_size = SUSPEND_CODE_SIZE;
			info->data_offset = SUSPEND_DATA_OFFSET;
			info->data_size = SUSPEND_DATA_SIZE;
			strcpy(info->name, "suspend");
			break;
		case PARTITION_UPDATE:
			info->code_offset = UPDATE_CODE_OFFSET;
			info->code_size = UPDATE_CODE_SIZE;
			info->data_offset = UPDATE_DATA_OFFSET;
			info->data_size = UPDATE_DATA_SIZE;
			strcpy(info->name, "update");
			break;
		case PARTITION_MAIN:
			info->code_offset = MAIN_CODE_OFFSET;
			info->code_size = MAIN_CODE_SIZE;
			info->data_offset = MAIN_DATA_OFFSET;
			info->data_size = MAIN_DATA_SIZE;
			info->spi_code_offset = MAIN_SPI_CODE_OFFSET;
			info->spi_code_size = MAIN_SPI_CODE_SIZE;
			info->readonly_offset = MAIN_RO_DATA_OFFSET;
			info->readonly_size = MAIN_RO_DATA_SIZE;
			info->audio_param_offset = MAIN_AUDIO_PARAM_OFFSET;
			info->audio_param_size = MAIN_AUDIO_PARAM_SIZE;
			info->sys_param_offset = MAIN_SYS_PARAM_OFFSET;
			info->sys_param_size = MAIN_SYS_PARAM_SIZE;
			strcpy(info->name, "main");
			break;
		case PARTITION_PQ:
			info->data_offset = PQ_BINARY_START;
			info->data_size = getFileSize(if_name);
			strcpy(info->name, "pq");
			break;
		case PARTITION_USER:
			info->data_offset = FBC_USER_START;
			info->data_size = FBC_USER_SIZE;
			strcpy(info->name, "user");
			break;
		case PARTITION_FACTORY:
			info->data_offset = FBC_FACTORY_START;
			info->data_size = FBC_FACTORY_SIZE;
			strcpy(info->name, "factory");
			break;
		default:
			break;
	}
}

int main ( int argc, char *argv[] )
{
	unsigned of_size;
	partition_info_t *info;
	unsigned char *buff;
	char *of_name = NULL, *tp_name = NULL;
	FILE *out = NULL;
	int val = 0;
	unsigned int crc = 0;

	while ( ( val = getopt_long ( argc, argv,
			optstring, long_options, NULL ) ) != -1 ) {
		switch ( val ) {
			case 'i':
				file_crc(optarg, &crc);
				if_name = optarg;
				break;
			case 'o':
				of_name = optarg;
				break;
			case 't':
				tp_name = optarg;
				break;
			case 's':
				of_size = atoi ( optarg );
				break;
			default:
				break;
		}
	}

	if ( !of_name || !of_size ) {
		printf ("error input parameter!\n");
		return -1;
	}
	out = fopen ( of_name, "wb" );
	if ( NULL == out ) {
		printf ( "open %s failed!\n", of_name );
		return -1;
	}
	clearerr ( out );
	buff = ( unsigned char * ) malloc ( of_size );
	if ( NULL == buff ) {
		printf ( "malloc buff failure\n" );
		fclose ( out );
		return -1;
	}

	memset ( buff, '\0', of_size );
	if (!strcmp ( "key.bin", of_name )) {
		memcpy ( buff, &rsa_key, sizeof(rsa_key) );
		*(unsigned int *)(buff+LAYOUT_VERSION_OFFSET) = LAYOUT_VERSION;
		*(unsigned int *)(buff+LAYOUT_VERSION_OFFSET+64) = (int)(FBC_VERSION_MAIN & 0xff) << 24
														| (int)(FBC_VERSION_SUB1 & 0xff) << 16
														| (int)(FBC_VERSION_SUB2 & 0xff) << 8;
	}
	else if (!strncmp ( "dummy", of_name, 5 )) {
	}
	else {
		info = (partition_info_t *) buff;
		set_partition_info (get_partition(of_name), info);
		memcpy(info->signature, rsa_key_signature2048_1, sizeof(rsa_key_signature2048_1));
		info->crc = crc;
	}

	fwrite ( buff, of_size, 1, out );
	fflush ( out );
	free ( buff );
	buff = NULL;
	fclose ( out );
	return 0;
}
