#include <serial.h>
#include <command.h>
#include <common.h>
#include <reboot.h>
#include <relocate.h>
#include <mini_console.h>
#include <inputdata.h>

#include <version.h>

#include <clock.h>
#include <sar_adc.h>
#include <board_config.h>

#define SF_UPGRADE_CMD    "ug"
#define RF_UPGRADE_RES    "ok"
#define RF_NOUPGRADE_RES    "ng"
#define CONFIG_UPDATE_KEY	16

//#define _ADCKEY_TEST_

void jump_0 ( void )
{
}

typedef void ( *main_entry ) ( void );

int __attribute__ ( ( section ( ".second.boot.entry" ) ) ) second_boot ( int boot_flag )
{
	/* copy_data_from_spi_to_sram2(BOOT_DATA_BASE, BOOT_DATA_SIZE>>2); */
	serial_init ( 0 );
	serial_init ( 2 );
	current_serial_device = default_serial_console();
#if (BOOT_DEBUG_VERSION == 1)
	printf ( "\nenter the second boot!\nbpflag = 0x%x\n", boot_flag );
#endif
	set_boot_stage ( SEC_BOOT_STAGE );
#if CONFIG_ENABLE_SARADC
	sar_adc_init();
#endif
#ifdef _ADCKEY_TEST_
	int i, j;
	INPUTDATA inputdata;
#if CONFIG_ENABLE_SARADC
	set_redetect_flag();
#endif
	if ( boot_flag == REBOOT_FLAG_NORMAL ) {
		for ( i = 0; i < 3; i++ ) {
#if  CONFIG_ENABLE_SARADC
			if ( !detect_adc_key ( 1, &inputdata ) ) {
				printf ( "data: 0x%x, type: %d\n", inputdata.input_data, inputdata.input_type );

				if ( inputdata.input_type == 0 && adc2key ( 1, &inputdata ) == CONFIG_UPDATE_KEY ) {
					char rec[10] = { 0, };
					int n;
					serial_init ( 2 );
					struct serial_device *sdev1 = get_serial_device ( 1 );

					while ( 1 ) {
						sdev1->puts ( SF_UPGRADE_CMD );

						if ( sdev1->tstc() >= 2 ) {
							for ( n = 0; n < 2; n++ ) {
								rec[n] = sdev1->getc();
							}

							rec[n] = 0;

							if ( !strcmp ( RF_UPGRADE_RES, rec ) ) {
								boot_flag = REBOOT_FLAG_UPGRADE2;
								break;

							} else if ( !strcmp ( RF_NOUPGRADE_RES, rec ) ) {
								boot_flag = REBOOT_FLAG_FROM_SUSPEND;
								break;
							}
						}
					}

					break;
				}
			}
#endif
			for ( j = 100 * 1000; j > 0; j-- )
				;
		}
	}

#endif
#if (BOOT_DEBUG_VERSION == 1)
	printf ( "fbc boot code version:\n" );
	print_build_version_info();
#endif
	reset_watchdog();

	if ( REBOOT_FLAG_SUSPEND == boot_flag ) {
		typedef void (*suspend)();
		if (copy_partition_to_ccm(SECTION_0, PARTITION_SUSPEND)) {
			printf("suspend crc error!\n");
			reboot(REBOOT_FLAG_FROM_WATCHDOG);
		}
		else {
			serial_puts ( "enter suspend!\n" );
			disable_watchdog();
			((suspend)ICCM_BASE)();
			return 0;
		}
	}

	if ( REBOOT_FLAG_UPGRADE1 == boot_flag || REBOOT_FLAG_UPGRADE2 == boot_flag ) {
		typedef int ( *update ) ();
		serial_puts("enter update!\n");
		if (copy_partition_to_ccm(SECTION_0, PARTITION_UPDATE)) {
			printf("update crc error!\n");
			reboot(REBOOT_FLAG_FROM_WATCHDOG);
		}
		else {
			printf ( "copy upgrade data done!\n" );
			disable_watchdog();
			set_boot_flag ( boot_flag );
			( ( update ) ICCM_BASE ) ();
		}
	}

	if ( bootdelay > 0 && abortboot ( bootdelay ) ) {
		disable_watchdog();
		do_wait_cmd();
		return 0;

	} else {
		if (copy_partition_to_ccm(SECTION_0, PARTITION_MAIN)) {
			printf("main crc error!\n");
			reboot(REBOOT_FLAG_FROM_WATCHDOG);
		}
		else {
			serial_puts("enter main!\n");
			set_boot_flag ( boot_flag );
			( ( main_entry ) ICCM_BASE ) ();
			return 0;
		}
	}
}
