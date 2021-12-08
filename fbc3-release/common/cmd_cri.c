#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <user_setting.h>
#include <spi_flash.h>
#include <board_config.h>

extern int isprint ( int c );

int do_cri ( cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[] )
{
	int i = 0, j = 0, tmp_val = 0;
	unsigned int tmp_off = 0, tmp_len = 0;
	const char *cmd;
	unsigned char *tmp_buf;
	char *tmp_ptr;
	char *endp;

	if ( argc < 2 ) {
		goto usage;
	}

	cmd = argv[1];

	if ( strcmp ( cmd, "pid" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}

		cmd = argv[2];

		if ( strcmp ( cmd, "r" ) == 0 ) {
			tmp_val = nvm_read_project_id();
			printf ( "current project id is %d.\n", tmp_val );
			return 0;

		} else if ( strcmp ( cmd, "w" ) == 0 ) {
			if ( argc < 4 ) {
				goto usage;
			}

			cmd = argv[3];
			tmp_val = strtoul ( cmd, &endp, 16 );

			if ( nvm_write_project_id ( tmp_val ) == 0 ) {
				printf ( "save project id %d only ok.\n", tmp_val );

			} else {
				printf ( "save project id %d only error!\n", tmp_val );
			}

			return 0;

		} else if ( strcmp ( cmd, "wa" ) == 0 ) {
			if ( argc < 4 ) {
				goto usage;
			}

			cmd = argv[3];
			tmp_val = strtoul ( cmd, &endp, 16 );

			if ( nvm_switch_project_id ( tmp_val ) == 0 ) {
				printf ( "save pro_id %d,load pqok.\n", tmp_val, tmp_val );

			} else {
				printf ( "save pro_id %d,load pqerror!\n", tmp_val, tmp_val );
			}

			return 0;
		}

	} else if ( strcmp ( cmd, "sn" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}

		cmd = argv[2];

		if ( strcmp ( cmd, "r" ) == 0 ) {
			tmp_ptr = nvm_read_factory_sn();
			printf ( "current factory sn is %s.\n", tmp_ptr );
			return 0;

		} else if ( strcmp ( cmd, "w" ) == 0 ) {
			if ( argc < 4 ) {
				goto usage;
			}

			cmd = argv[3];

			if ( nvm_write_factory_sn ( ( char * ) cmd ) == 0 ) {
				printf ( "save factory sn \"%s\" ok.\n", cmd );

			} else {
				printf ( "save factory sn \"%s\" error!\n", cmd );
			}

			return 0;
		}

	} else if ( strcmp ( cmd, "did" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}

		cmd = argv[2];

		if ( strcmp ( cmd, "r" ) == 0 ) {
			tmp_ptr = nvm_read_device_id();
			printf ( "current device id is %s.\n", tmp_ptr );
			return 0;

		} else if ( strcmp ( cmd, "w" ) == 0 ) {
			if ( argc < 4 ) {
				goto usage;
			}

			cmd = argv[3];

			if ( nvm_write_device_id ( ( char * ) cmd ) == 0 ) {
				printf ( "save device id \"%s\" ok.\n", cmd );

			} else {
				printf ( "save device id \"%s\" error!\n", cmd );
			}

			return 0;
		}

	} else if ( strcmp ( cmd, "sf" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}

		cmd = argv[2];

		if ( strcmp ( cmd, "r" ) == 0 ) {
			if ( argc < 5 ) {
				goto usage;
			}

			tmp_off = strtoul ( argv[3], &endp, 16 );
			tmp_len = strtoul ( argv[4], &endp, 16 );
			printf ( "offset = 0x%08X, len = 0x%08X.\n", tmp_off, tmp_len );

			if ( tmp_len > 0x4000 ) {
				goto usage;
			}

			tmp_buf = ( unsigned char * ) malloc ( tmp_len );

			if ( tmp_buf == NULL ) {
				printf ( "malloc memory error!\n" );
				return 0;
			}

			spi_flash_read ( get_spi_flash_device ( 0 ), tmp_off, tmp_len, tmp_buf );

			for ( i = 0; i < tmp_len; i++ ) {
				if ( i % 16 == 0 ) {
					printf ( "\n%08X:", tmp_off + i );
				}

				printf ( " %02X", tmp_buf[i] );

				if ( i % 16 == 15 ) {
					printf ( "    " );

					for ( j = i - 15; j <= i; j++ ) {
						/*
						 if (!isprint(tmp_buf[j])
						 || tmp_buf[j] >=
						 0x80)
						 printf(".");
						 else
						 printf("%c",
						 tmp_buf
						 [j]);
						 */
						!isprint ( tmp_buf[j] ) || tmp_buf[j] >= 0x80 ? printf ( "." ) : printf ( "%c", tmp_buf[j] );
					}
				}
			}

			printf ( "\n" );
			free ( tmp_buf );
			tmp_buf = NULL;

		} else if ( strcmp ( cmd, "w" ) == 0 ) {
			if ( argc < 5 ) {
				goto usage;
			}

			tmp_off = strtoul ( argv[3], &endp, 16 );
			tmp_len = strlen ( argv[4] );
			printf ( "offset = 0x%08X, len = 0x%08X.\n", tmp_off, tmp_len / 2 );

			if ( ( tmp_len & 1 ) || tmp_len > 0x4000 ) {
				goto usage;
			}

			tmp_buf = ( unsigned char * ) malloc ( tmp_len );

			if ( tmp_buf != NULL ) {
				unsigned char tmp_chs[3] = { 0, 0, 0 };

				for ( i = 0; i < tmp_len / 2; i++ ) {
					memcpy ( tmp_chs, argv[4] + i * 2, 2 );
					tmp_buf[i] = strtoul ( ( const char * ) tmp_chs, &endp, 16 );
				}

				if ( tmp_len != spi_flash_random_write ( get_spi_flash_device ( 0 ), tmp_off, tmp_len / 2, tmp_buf ) * 2 ) {
					printf ( "spi flash write failed!\n" );

				} else {
					printf ( "spi flash write success.\n" );
				}

				free ( tmp_buf );
				tmp_buf = NULL;

			} else {
				printf ( "malloc memory error!\n" );
			}

			return 0;

		} else if ( strcmp ( cmd, "wb" ) == 0 ) {
			if ( argc < 6 ) {
				goto usage;
			}

			tmp_off = strtoul ( argv[3], &endp, 16 );
			tmp_len = strtoul ( argv[4], &endp, 16 );
			tmp_val = strtoul ( argv[5], &endp, 16 ) & 0xFF;
			printf ( "offset = 0x%08X, len = 0x%08X, val = 0x%08X.\n", tmp_off, tmp_len, tmp_val );

			if ( tmp_off < PQ_BINARY_START ) {
				printf ( "can't erase code area!!!\n" );
			}

			//return 0;
/*
			tmp_buf = ( unsigned char * ) malloc ( tmp_len );

			if ( tmp_buf != NULL ) {
				memset ( ( void * ) tmp_buf, tmp_val, tmp_len );

				if ( tmp_len != spi_flash_random_write ( get_spi_flash_device ( 0 ), tmp_off, tmp_len, tmp_buf ) ) {
					printf ( "spi wr 0x%02X(0x%08X,0x%08X) fail!\n", tmp_val, tmp_off, tmp_len );

				} else {
					printf ( "spi erase 0x%02X(0x%08X,0x%08X) ok\n", tmp_val, tmp_off, tmp_len );
				}

				free ( tmp_buf );
				tmp_buf = NULL;

			} else {
				printf ( "malloc memory error!\n" );
			}

			return 0;
*/
		}
	}

usage:
	return cmd_usage ( cmdtp );
}
