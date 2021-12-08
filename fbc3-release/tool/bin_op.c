/*
*	This tool is used to generate audio
control parameters' config file,
that is stored on spi.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <panel.h>
#include <board_config.h>

#define CC_MAX_BIN_FILE_SIZE    CONFIG_SPI_SIZE

#define CC_PROJECT_ID_OFFSET    (USER_CRI_DATA_START)
#define CC_DEVICE_ID_OFFSET     (USER_CRI_DATA_START + 128)

static char *optstring = "i:t:o:";

static struct option long_options[] = {
	{"input", 2, NULL, 'i'},
	{"type", 1, NULL, 't'},
	{"output", 1, NULL, 'o'},
};

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

static void usage ( void )
{
	return;
}

char gFileNames[256][256];
int gFileCnt = 0;

int trave_dir ( char *path, int depth, char *ext_name )
{
	int i = 0;
	char *tmp_ptr = NULL;
	DIR *d;
	struct dirent *file;
	struct stat sb;
	d = opendir ( path );

	if ( !d ) {
		printf ( "error opendir %s!!!\n", path );
		return -1;
	}

	while ( ( file = readdir ( d ) ) != NULL ) {
		if ( strncmp ( file->d_name, ".", 1 ) == 0 ) {
			continue;
		}

		tmp_ptr = strrchr ( file->d_name, '.' );

		if ( tmp_ptr != NULL && strcasecmp ( tmp_ptr, ext_name ) == 0 ) {
			strcpy ( gFileNames[gFileCnt++], file->d_name );
		}

		if ( stat ( file->d_name, &sb ) >= 0 && S_ISDIR ( sb.st_mode )
			 && depth <= 3 ) {
			trave_dir ( file->d_name, depth + 1, ext_name );
		}
	}

	closedir ( d );
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

static int readOneBinFile ( const char *file_name, unsigned char data_buf[],
							int rd_len )
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

	if ( rd_len > tmp_len ) {
		return 0;
	}

	fseek ( fp, 0, SEEK_SET );
	rd_len = fread ( data_buf, rd_len, 1, fp );
	fclose ( fp );
	fp = NULL;
	return rd_len;
}

static int writeOneBinFile ( const char *file_name, unsigned char data_buf[],
							 int wr_len )
{
	int tmp_len = 0;
	FILE *fp = NULL;

	if ( file_name == NULL || strlen ( file_name ) == 0 ) {
		return 0;
	}

	fp = fopen ( file_name, "wb" );

	if ( fp == NULL ) {
		return 0;
	}

	wr_len = fwrite ( data_buf, wr_len, 1, fp );
	fflush ( fp );
	fclose ( fp );
	fp = NULL;
	return wr_len;
}

static int writeFileAddBinData ( const char *file_name, unsigned char data_buf[],
								 int wr_len )
{
	int tmp_len = 0;
	FILE *fp = NULL;

	if ( file_name == NULL || strlen ( file_name ) == 0 ) {
		return 0;
	}

	fp = fopen ( file_name, "wb+" );

	if ( fp == NULL ) {
		return 0;
	}

	wr_len = fwrite ( data_buf, 1, wr_len, fp );
	fclose ( fp );
	fp = NULL;
	return wr_len;
}

void quicksort ( char s[][256], int left, int right )
{
	int i = left, j = right, p = left;
	char tmp_buf[512];
	strcpy ( tmp_buf, s[left] );

	while ( i < j ) {
		while ( strcmp ( s[j], tmp_buf ) >= 0 && j >= p ) {
			j--;
		}

		if ( j >= p ) {
			strcpy ( s[p], s[j] );
			p = j;
		}

		while ( strcmp ( s[i], tmp_buf ) <= 0 && i <= p ) {
			i++;
		}

		if ( i <= p ) {
			strcpy ( s[p], s[i] );
			p = i;
		}
	}

	strcpy ( s[p], tmp_buf );

	if ( p - left > 1 ) {
		quicksort ( s, left, p - 1 );
	}

	if ( right - p > 1 ) {
		quicksort ( s, p + 1, right );
	}
}

int main ( int argc, char **argv )
{
	int i, flag, val, magic;
	int tmp_val = 0;
	unsigned if_size = 0;
	unsigned of_size = 0;
	unsigned real_rc = 0;
	unsigned int tmp_crc = 0;
	int rd_len = 0;
	char *if_path = NULL;
	char *of_path = NULL;
	char *type = NULL;
	char *tmp_ptr = NULL;
	unsigned char *data_buf = NULL;
	FILE *out = NULL;
	FILE *in = NULL;
	char cur_path[512];
	char ouput_path[512];
	char tmp_path[512];
	char dst_path[512];
	char cmd_buf[1024];
	flag = 0;

	if ( argc < 7 ) {
		flag = 1;
		goto EXT_1;
	}

	while ( ( val =
				  getopt_long ( argc, argv, optstring, long_options, NULL ) ) != -1 ) {
		switch ( val ) {
			case 'i':
				if_path = optarg;
				break;

			case 't':
				type = optarg;
				break;

			case 'o':
				of_path = optarg;
				break;
		}
	}

	printf ( "type %s\n", type );
	printf ( "if_path %s\n", if_path );
	printf ( "of_path %s\n", of_path );

	if ( strcasecmp ( type, "pq" ) == 0 ) {
		getcwd ( cur_path, 512 );
		strcpy ( tmp_path, cur_path );
		strcat ( tmp_path, "/" );
		strcat ( tmp_path, if_path );
		trave_dir ( tmp_path, 1, ".pq" );
		quicksort ( gFileNames, 0, gFileCnt - 1 );
		strcpy ( cmd_buf, "cat " );
		data_buf = ( unsigned char * ) malloc ( PQ_BINARY_UNIT_SIZE );

		if ( data_buf != NULL ) {
			for ( i = 0; i < gFileCnt; i++ ) {
				strcpy ( tmp_path, cur_path );
				strcat ( tmp_path, "/" );
				strcat ( tmp_path, if_path );
				strcat ( tmp_path, "/" );
				strcat ( tmp_path, gFileNames[i] );
				printf ( "\"%s\" is pq file, size is %d.\n",
						 gFileNames[i], getFileSize ( tmp_path ) );
				memset ( ( void * ) data_buf, 0,
						 PQ_BINARY_UNIT_SIZE );
				rd_len = getFileSize ( tmp_path );
				readOneBinFile ( tmp_path, data_buf, rd_len );
				strcpy ( dst_path, tmp_path );
				tmp_ptr = strrchr ( dst_path, '.' );

				if ( tmp_ptr != NULL ) {
					*tmp_ptr = '\0';
					strcat ( dst_path, ".pqbin" );
					strcat ( cmd_buf, dst_path );
					strcat ( cmd_buf, " " );
					remove ( dst_path );
					tmp_crc =
						crc32 ( 0, data_buf,
								PQ_BINARY_UNIT_SIZE - 4 );
					printf ( "tmp_crc = 0x%x\n", tmp_crc );
					memcpy ( ( void * ) ( data_buf +
										  PQ_BINARY_UNIT_SIZE -
										  4 ), ( void * ) &tmp_crc,
							 4 );
					writeOneBinFile ( dst_path, data_buf,
									  PQ_BINARY_UNIT_SIZE );
				}
			}

			strcpy ( ouput_path, cur_path );
			strcat ( ouput_path, "/" );
			strcat ( ouput_path, of_path );
			remove ( ouput_path );
			strcat ( cmd_buf, " >> " );
			strcat ( cmd_buf, ouput_path );
			puts ( cmd_buf );
			system ( cmd_buf );

			for ( i = 0; i < gFileCnt; i++ ) {
				strcpy ( tmp_path, cur_path );
				strcat ( tmp_path, "/" );
				strcat ( tmp_path, if_path );
				strcat ( tmp_path, "/" );
				strcat ( tmp_path, gFileNames[i] );
				tmp_ptr = strrchr ( tmp_path, '.' );

				if ( tmp_ptr != NULL ) {
					*tmp_ptr = '\0';
					strcat ( tmp_path, ".pqbin" );
					remove ( tmp_path );
				}
			}

			free ( data_buf );
			data_buf = NULL;
		}

	} else if ( strcasecmp ( type, "all" ) == 0 ) {
//		getcwd ( cur_path, 512 );
//		strcpy ( cmd_buf, "cat " );
//		strcpy ( tmp_path, cur_path );
//		strcat ( tmp_path, "/" );
//		strcat ( tmp_path, "spi.bin" );
//		strcat ( cmd_buf, tmp_path );
//		strcat ( cmd_buf, " " );
//		strcpy ( tmp_path, cur_path );
//		strcat ( tmp_path, "/" );
//		strcat ( tmp_path, if_path );
//		strcat ( cmd_buf, tmp_path );
//		strcpy ( ouput_path, cur_path );
//		strcat ( ouput_path, "/" );
//		strcat ( ouput_path, of_path );
//		strcat ( cmd_buf, " >> " );
//		strcat ( cmd_buf, ouput_path );
//		puts ( cmd_buf );
//		system ( cmd_buf );
		data_buf = ( unsigned char * ) malloc ( CC_MAX_BIN_FILE_SIZE );

		if ( data_buf != NULL ) {
			memset ( ( void * ) data_buf, 0, CC_MAX_BIN_FILE_SIZE );
			rd_len = getFileSize ( if_path );
			readOneBinFile ( if_path, data_buf, rd_len );
			/* handle project id */
			tmp_val = PROJECT_ID;
			tmp_crc = crc32 ( 0, ( unsigned char * ) &tmp_val, 4 );
			memcpy ( ( void * ) ( data_buf + CC_PROJECT_ID_OFFSET ),
					 ( void * ) &tmp_val, 4 );
			memcpy ( ( void * ) ( data_buf + CC_PROJECT_ID_OFFSET + 4 ),
					 ( void * ) &tmp_crc, 4 );
			/* handle device id (panel module) */
			tmp_val = strlen ( PANEL_MODULE );
			tmp_crc = crc32 ( 0, ( unsigned char * ) &tmp_val, 4 );
			memcpy ( ( void * ) ( data_buf + CC_DEVICE_ID_OFFSET ),
					 ( void * ) &tmp_val, 4 );
			memcpy ( ( void * ) ( data_buf + CC_DEVICE_ID_OFFSET + 4 ),
					 ( void * ) &tmp_crc, 4 );
			tmp_crc =
				crc32 ( 0, ( unsigned char * ) PANEL_MODULE, tmp_val );
			memcpy ( ( void * ) ( data_buf + CC_DEVICE_ID_OFFSET + 8 ),
					 ( void * ) &tmp_crc, 4 );
			memcpy ( ( void * ) ( data_buf + CC_DEVICE_ID_OFFSET + 12 ),
					 ( void * ) PANEL_MODULE, tmp_val );
			/* write data to the bin file */
			writeOneBinFile ( of_path, data_buf, CC_MAX_BIN_FILE_SIZE );
			free ( data_buf );
			data_buf = NULL;
		}
	}

EXT_1:
	return 0;
}
