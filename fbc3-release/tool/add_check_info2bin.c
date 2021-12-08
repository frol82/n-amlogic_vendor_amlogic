/*
*	This tool is used to generate audio control */
/* parameters' config file,  that is stored on spi.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define MAX_NAME_LEN				0x10
#define MAX_BUF_SIZE				(800*0x400)
#define CHECK_INFO_SIZE				0x30
#define MAIN_H_OFFS					0x100
#define BOOT_H_OFFS					0x0

#define HAS_INPUT_FILE		(0x1 << 0)
#define HAS_OUTPUT_FILE		(0x1 << 1)
#define HAS_ROM_MAP_INFO	(0x1 << 2)
#define HAS_CHECK_INFO		(0x1 << 3)
#define HAS_TYPE_INFO		(0x1 << 4)

static int cmd_flag;

static struct option long_options[] = {
	{"input", 1, NULL, 'i'},
	{"config", 1, NULL, 'c'},
	{"output", 1, NULL, 'o'},
	{"tpye", 1, NULL, 't'},
	{"rommap", 1, NULL, 's'},
};
static char *optstring = "i:c:o:t:s:";

static void usage ( void )
{
	return;
}

int main ( int argc, char **argv )
{
	int i, flag, val;
	unsigned if_size = 0;
	unsigned of_size = 0;
	unsigned real_rc = 0;
	unsigned offset = 0;
	char *if_name = NULL;
	char *of_name = NULL;
	char *ip_name = NULL;
	char *xp_name = NULL;
	char *tp_name = NULL;
	char bin_buf[MAX_BUF_SIZE];
	char ci_buf[CHECK_INFO_SIZE];
	char rommap_buf[CHECK_INFO_SIZE];
	FILE *out = NULL;
	FILE *in = NULL;
	FILE *p_in = NULL;
	FILE *rommap_in = NULL;
	flag = 0;

	if ( argc < 5 ) {
		flag = 1;
		goto EXT_1;
	}

	cmd_flag = 0;

	while ( ( val =
				  getopt_long ( argc, argv,
								optstring, long_options, NULL ) )
			!= -1 ) {
		switch ( val ) {
			case 'i':
				cmd_flag |= HAS_INPUT_FILE;
				if_name = optarg;
				break;

			case 'c':
				cmd_flag |= HAS_CHECK_INFO;
				ip_name = optarg;
				break;

			case 'o':
				cmd_flag |= HAS_OUTPUT_FILE;
				of_name = optarg;
				break;

			case 't':
				cmd_flag |= HAS_TYPE_INFO;
				tp_name = optarg;
				break;

			case 's':
				cmd_flag |= HAS_ROM_MAP_INFO;
				xp_name = optarg;
				break;
		}
	}

	if ( cmd_flag & HAS_INPUT_FILE ) {
		printf ( "if_name: %s\n", if_name );
		in = fopen ( if_name, "r" );

		if ( NULL == in ) {
			printf ( "open %s failed!\n", if_name );
			goto EXT_1;
		}

		clearerr ( in );
	}

	if ( cmd_flag & HAS_OUTPUT_FILE ) {
		printf ( "of_name: %s\n", of_name );
		out = fopen ( of_name, "wb" );

		if ( NULL == in ) {
			printf ( "open %s failed!\n", of_name );
			goto EXT_2;
		}

		clearerr ( out );
	}

	if ( cmd_flag & HAS_CHECK_INFO ) {
		printf ( "cofig file: %s\n", ip_name );
		p_in = fopen ( ip_name, "r" );

		if ( NULL == p_in ) {
			printf ( "open %s failed!\n", ip_name );
			goto EXT_3;
		}

		clearerr ( p_in );
	}

	if ( cmd_flag & HAS_ROM_MAP_INFO ) {
		printf ( "xp_name: %s\n", xp_name );
		rommap_in = fopen ( xp_name, "r" );

		if ( NULL == rommap_in ) {
			printf ( "open %s failed!\n", xp_name );
			goto EXT_3;
		}

		clearerr ( rommap_in );
	}

	if ( cmd_flag & HAS_TYPE_INFO ) {
		printf ( "tp_name: %s\n", tp_name );

		if ( !strcmp ( "main", tp_name ) ) {
			offset = MAIN_H_OFFS;

		} else {
			offset = BOOT_H_OFFS;
		}

		printf ( "offset = %d\n", offset );
	}

	memset ( bin_buf, 0, sizeof ( bin_buf ) );
	memset ( ci_buf, 0, sizeof ( ci_buf ) );
	real_rc = fread ( ci_buf, sizeof ( ci_buf ) - 1, 1, p_in );

	if ( real_rc < 1 ) {
		if ( ferror ( p_in ) ) {
			printf ( "fread %s error.\n", if_name );
			goto EXT_3;
		}

		ftell ( p_in );
	}

	printf ( "ci_buf: %s\n", ci_buf );
	real_rc = fread ( bin_buf, MAX_BUF_SIZE, 1, in );

	if ( real_rc < 1 ) {
		if ( ferror ( in ) ) {
			printf ( "fread %s error.\n", if_name );
			goto EXT_3;
		}

		if_size = ftell ( in );
	}

	if ( cmd_flag & HAS_ROM_MAP_INFO ) {
		memset ( rommap_buf, 0, sizeof ( rommap_buf ) );
		fgets ( rommap_buf, sizeof ( rommap_buf ), rommap_in );
		printf ( "main code size: %s\n", rommap_buf );
		int size = strtol ( rommap_buf, NULL, 10 );
		memcpy ( ci_buf + sizeof ( ci_buf ) - 2 * sizeof ( size ),
				 &size, sizeof ( size ) );
		memset ( rommap_buf, 0, sizeof ( rommap_buf ) );
		fgets ( rommap_buf, sizeof ( rommap_buf ),
				rommap_in );
		printf ( "main data size: %s\n", rommap_buf );
		size = strtol ( rommap_buf, NULL, 10 );
		memcpy ( ci_buf + sizeof ( ci_buf ) - sizeof ( size ),
				 &size, sizeof ( size ) );
	}

	memcpy ( bin_buf + if_size - CHECK_INFO_SIZE, ci_buf,
			 sizeof ( ci_buf ) );
	memcpy ( bin_buf + offset, ci_buf, sizeof ( ci_buf ) );
	of_size = if_size;
	fwrite ( bin_buf, of_size, 1, out );
	fflush ( out );
EXT_3:
	fclose ( out );
EXT_2:
	fclose ( in );
EXT_1:

	if ( flag ) {
		usage();
	}

EXT_0:
	return 0;
}

